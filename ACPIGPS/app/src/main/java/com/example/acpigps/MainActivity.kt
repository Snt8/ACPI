package com.example.acpigps

import android.Manifest
import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.content.ServiceConnection
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.os.IBinder
import android.widget.Toast
import androidx.activity.viewModels
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat

class MainActivity : AppCompatActivity(), CompassFragment.CompassListener, BLEDebugFragment.BLEDebugListener {

    private val viewModel: MainViewModel by viewModels()
    private var monitoringService: MonitoringService? = null
    private var isBound = false
    private var isMonitoring = false

    private val connection = object : ServiceConnection {
        override fun onServiceConnected(className: ComponentName, service: IBinder) {
            val binder = service as MonitoringService.LocalBinder
            monitoringService = binder.getService()
            isBound = true
            observeServiceData()
        }

        override fun onServiceDisconnected(arg0: ComponentName) {
            isBound = false
            monitoringService = null
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        val prefs = getSharedPreferences("app_prefs", Context.MODE_PRIVATE)
        prefs.edit()
            .putString("grafana_url", BuildConfig.GRAFANA_URL)
            .putString("grafana_token", BuildConfig.GRAFANA_TOKEN)
            .putString("grafana_user", BuildConfig.GRAFANA_USER)
            .apply()

        val viewPager: androidx.viewpager2.widget.ViewPager2 = findViewById(R.id.view_pager)
        viewPager.adapter = ViewPagerAdapter(this)

        viewModel.isMonitoring.observe(this) { monitoringState ->
            isMonitoring = monitoringState
            if (isMonitoring && !isBound) {
                bindToService()
            }
        }
    }

    private fun observeServiceData() {
        monitoringService?.bleConnectionState?.observe(this) { viewModel.onConnectionStateChanged(it) }
        monitoringService?.compassData?.observe(this) { viewModel.updateCompassData(it) }
        monitoringService?.logMessages?.observe(this) { viewModel.updateLogMessages(it) }
        monitoringService?.pulseraData?.observe(this) { viewModel.updatePulseraData(it) }
    }

    private fun startMonitoringService() {
        val serviceIntent = Intent(this, MonitoringService::class.java)
        ContextCompat.startForegroundService(this, serviceIntent)
        bindToService()
        viewModel.setMonitoringState(true)
    }

    private fun stopMonitoringService() {
        if (isBound) {
            unbindService(connection)
            isBound = false
        }
        val serviceIntent = Intent(this, MonitoringService::class.java)
        stopService(serviceIntent)
        viewModel.setMonitoringState(false)
    }

    private fun bindToService() {
        Intent(this, MonitoringService::class.java).also { intent ->
            bindService(intent, connection, Context.BIND_AUTO_CREATE)
        }
    }

    // --- Listeners de los Fragments ---
    override fun onToggleWalkClicked() {
        if (isMonitoring) {
            stopMonitoringService()
        } else {
            checkAndRequestPermissions()
        }
    }

    override fun onSendTestClicked() {
        monitoringService?.sendTestToEsp32()
    }

    override fun onDisconnectClicked() {
        stopMonitoringService()
    }

    override fun onSaveLocationClicked() {
        // Ahora le pedimos al servicio que guarde la ubicación.
        if (isBound && isMonitoring) {
            monitoringService?.saveCurrentLocationAsHome()
        } else {
            Toast.makeText(this, "Debe iniciar el monitoreo para guardar la ubicación.", Toast.LENGTH_SHORT).show()
        }
    }

    // --- Lógica de Permisos ---
    private val PERMISSION_REQUEST_CODE = 101
    private fun checkAndRequestPermissions() {
        val permissionsToRequest = mutableListOf<String>()
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            permissionsToRequest.add(Manifest.permission.POST_NOTIFICATIONS)
        }
        // Añadir otros permisos
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.ACTIVITY_RECOGNITION) != PackageManager.PERMISSION_GRANTED) {
            permissionsToRequest.add(Manifest.permission.ACTIVITY_RECOGNITION)
        }
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
            permissionsToRequest.add(Manifest.permission.ACCESS_FINE_LOCATION)
        }
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED) {
            permissionsToRequest.add(Manifest.permission.BLUETOOTH_SCAN)
        }
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
            permissionsToRequest.add(Manifest.permission.BLUETOOTH_CONNECT)
        }

        if (permissionsToRequest.isNotEmpty()) {
            ActivityCompat.requestPermissions(this, permissionsToRequest.toTypedArray(), PERMISSION_REQUEST_CODE)
        } else {
            startMonitoringService()
        }
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == PERMISSION_REQUEST_CODE && grantResults.all { it == PackageManager.PERMISSION_GRANTED }) {
            startMonitoringService()
        } else {
            Toast.makeText(this, "Se requieren permisos para iniciar el monitoreo.", Toast.LENGTH_LONG).show()
        }
    }
}
