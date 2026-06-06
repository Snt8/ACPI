package com.example.acpigps

import android.annotation.SuppressLint
import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Context
import android.content.Intent
import android.content.SharedPreferences
import android.location.Location
import android.os.Binder
import android.os.Build
import android.os.IBinder
import android.os.Looper
import android.util.Log
import android.widget.Toast
import androidx.core.app.NotificationCompat
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import com.google.android.gms.location.*
import org.json.JSONObject
import java.util.UUID
import kotlin.math.roundToInt

class MonitoringService : Service(), ConnectionCallback {

    companion object {
        private const val NOTIFICATION_ID = 1
        private const val CHANNEL_ID = "MonitoringServiceChannel"
        const val ACTION_STOP_MONITORING = "ACTION_STOP_MONITORING"
    }

    inner class LocalBinder : Binder() {
        fun getService(): MonitoringService = this@MonitoringService
    }
    private val binder = LocalBinder()

    // --- LiveData para la UI ---
    private val _bleConnectionState = MutableLiveData(BleConnectionState())
    val bleConnectionState: LiveData<BleConnectionState> = _bleConnectionState
    private val _compassData = MutableLiveData(CompassData())
    val compassData: LiveData<CompassData> = _compassData
    private val _logMessages = MutableLiveData<List<String>>(emptyList())
    val logMessages: LiveData<List<String>> = _logMessages
    private val _pulseraData = MutableLiveData<PulseraData?>(null)
    val pulseraData: LiveData<PulseraData?> = _pulseraData

    // --- Managers y Clientes ---
    private lateinit var bleManager: BLEManager
    private lateinit var directionDetector: WalkDirectionDetector
    private lateinit var fusedLocationClient: FusedLocationProviderClient
    private lateinit var locationCallback: LocationCallback
    private lateinit var notificationManager: LocalNotificationManager
    private lateinit var grafanaReporter: GrafanaReporter
    private lateinit var sessionLogger: SessionLogger
    private lateinit var sharedPreferences: SharedPreferences

    // --- Variables de Estado ---
    private var isServiceRunning = false
    private var stepCount = 0
    private var homeLocation: Location? = null
    private val safeRadius = 50.0

    override fun onCreate() {
        super.onCreate()
        
        // Inicializar componentes locales y de telemetría
        bleManager = BLEManager(this)
        bleManager.connectionCallback = this
        directionDetector = WalkDirectionDetector(this) { angle, direction ->
            stepCount++
            _compassData.postValue(CompassData(angle, direction, stepCount))
        }
        fusedLocationClient = LocationServices.getFusedLocationProviderClient(this)
        
        notificationManager = LocalNotificationManager(this)
        grafanaReporter = GrafanaReporter(this)
        sessionLogger = SessionLogger(this, grafanaReporter)
        sharedPreferences = getSharedPreferences("app_prefs", Context.MODE_PRIVATE)

        setupLocationCallback()
        loadHomeLocation()
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        if (intent?.action == ACTION_STOP_MONITORING) {
            stopMonitoring()
            return START_NOT_STICKY
        }
        startForeground(NOTIFICATION_ID, createNotification("Iniciando monitoreo..."))
        startMonitoring()
        return START_STICKY
    }

    @SuppressLint("MissingPermission")
    private fun startMonitoring() {
        if (isServiceRunning) return
        addLog("Servicio de monitoreo iniciado.")

        // Iniciar sesión de telemetría
        sessionLogger.startSession(stepCount)
        _pulseraData.postValue(null)

        stepCount = 0
        directionDetector.startDetection()
        bleManager.startScan()
        fusedLocationClient.requestLocationUpdates(
            LocationRequest.create().apply {
                priority = LocationRequest.PRIORITY_HIGH_ACCURACY
                interval = 5000
            },
            locationCallback,
            Looper.getMainLooper()
        )
        isServiceRunning = true
    }

    private fun stopMonitoring() {
        if (!isServiceRunning) return
        addLog("Servicio de monitoreo detenido.")
        
        // Detener sensores y BLE
        directionDetector.stopDetection()
        bleManager.disconnect()
        fusedLocationClient.removeLocationUpdates(locationCallback)

        // Registrar ubicación final y subir telemetría
        fusedLocationClient.lastLocation.addOnSuccessListener { location ->
            sessionLogger.endSession(stepCount, location)
            isServiceRunning = false
            stopForeground(true)
            stopSelf()
        }.addOnFailureListener {
            sessionLogger.endSession(stepCount, null)
            isServiceRunning = false
            stopForeground(true)
            stopSelf()
        }
    }

    override fun onDestroy() {
        stopMonitoring()
        super.onDestroy()
    }

    override fun onBind(intent: Intent): IBinder = binder

    // --- Lógica de Geofence ---

    private fun loadHomeLocation() {
        val lat = sharedPreferences.getString("home_lat", null)?.toDouble()
        val lon = sharedPreferences.getString("home_lon", null)?.toDouble()
        if (lat != null && lon != null) {
            homeLocation = Location("").apply {
                latitude = lat
                longitude = lon
            }
            addLog("Ubicación segura cargada.")
        } else {
            addLog("No hay ubicación segura guardada.")
        }
    }

    private fun checkGeofence(currentLocation: Location) {
        homeLocation?.let { home ->
            val distance = currentLocation.distanceTo(home)
            if (distance > safeRadius) {
                addLog("⚠️ ALERTA: Fuera de zona segura! Distancia: ${distance.roundToInt()}m")
                notificationManager.alertGeofenceViolation(distance.roundToInt())
            }
        }
    }

    @SuppressLint("MissingPermission")
    fun saveCurrentLocationAsHome() {
        fusedLocationClient.lastLocation.addOnSuccessListener { location ->
            if (location != null) {
                homeLocation = location
                with(sharedPreferences.edit()) {
                    putString("home_lat", location.latitude.toString())
                    putString("home_lon", location.longitude.toString())
                    apply()
                }
                val logMessage = "Ubicación segura guardada: Lat: ${location.latitude}, Lon: ${location.longitude}"
                addLog(logMessage)
                Handler(Looper.getMainLooper()).post {
                    Toast.makeText(this, "Ubicación segura guardada.", Toast.LENGTH_SHORT).show()
                }
            } else {
                addLog("Error: No se pudo obtener la ubicación actual para guardarla.")
                Handler(Looper.getMainLooper()).post {
                    Toast.makeText(this, "No se pudo obtener la ubicación actual.", Toast.LENGTH_SHORT).show()
                }
            }
        }
    }

    private fun setupLocationCallback() {
        locationCallback = object : LocationCallback() {
            override fun onLocationResult(locationResult: LocationResult) {
                locationResult.lastLocation?.let { location ->
                    checkGeofence(location)
                }
            }
        }
    }

    // --- Callbacks de Conectividad y Datos BLE ---

    override fun onPulseraDataReceived(data: PulseraData) {
        val stPrevio = _pulseraData.value?.st
        val btPrevio = _pulseraData.value?.bt
        _pulseraData.postValue(data)

        // 1. Notificación local si cambia a st=1 (Cruce permitido)
        if (data.st == 1 && stPrevio != 1) {
            notificationManager.alertSafeToCross()
        }

        // 2. Notificación local si la batería baja de 15%
        if (data.bt in 1..15 && data.bt != btPrevio) {
            notificationManager.alertLowBattery(data.bt)
        }

        // Loguear datos en el buffer de la sesión
        fusedLocationClient.lastLocation.addOnSuccessListener { location ->
            sessionLogger.logData(data, location, stepCount)
        }
    }

    override fun onConnectionStateChanged(isConnected: Boolean, deviceName: String?) {
        val currentState = _bleConnectionState.value ?: BleConnectionState()
        _bleConnectionState.postValue(currentState.copy(isConnected = isConnected, deviceName = deviceName, isScanning = false))
        updateNotification(if (isConnected) "Conectado a ${deviceName ?: "dispositivo"}" else "Buscando dispositivo...")
    }

    override fun onScanningStateChanged(isScanning: Boolean) {
        val currentState = _bleConnectionState.value ?: BleConnectionState()
        _bleConnectionState.postValue(currentState.copy(isScanning = isScanning))
    }

    override fun logMessage(message: String) { addLog(message) }

    private fun createNotification(text: String): Notification {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val channel = NotificationChannel(CHANNEL_ID, "Canal de Monitoreo", NotificationManager.IMPORTANCE_LOW)
            val notificationManager = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
            notificationManager.createNotificationChannel(channel)
        }
        val stopIntent = Intent(this, MonitoringService::class.java).apply { action = ACTION_STOP_MONITORING }
        val stopPendingIntent = PendingIntent.getService(this, 0, stopIntent, PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT)
        return NotificationCompat.Builder(this, CHANNEL_ID)
            .setContentTitle("ACPI GPS Monitoreando")
            .setContentText(text)
            .setSmallIcon(R.drawable.ic_launcher_foreground)
            .setOngoing(true)
            .addAction(android.R.drawable.ic_media_pause, "Detener", stopPendingIntent)
            .build()
    }

    private fun updateNotification(text: String) {
        val notification = createNotification(text)
        (getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager).notify(NOTIFICATION_ID, notification)
    }

    fun sendTestToEsp32() {
        bleManager.sendCommandToEsp32("TEST_ALERT")
        addLog("Enviando comando de prueba 'TEST_ALERT' al dispositivo BLE.")
    }

    private fun addLog(message: String) {
        val currentLogs = _logMessages.value?.toMutableList() ?: mutableListOf()
        currentLogs.add(0, message)
        _logMessages.postValue(currentLogs)
    }
}
