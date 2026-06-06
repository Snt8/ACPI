package com.example.acpigps

import android.annotation.SuppressLint
import android.bluetooth.*
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanFilter
import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import android.content.Context
import android.os.Handler
import android.os.Looper
import android.util.Log
import java.util.UUID
import org.json.JSONObject

// El objeto BLEConstants se mantiene igual
object BLEConstants {
    val SERVICE_UUID: UUID = UUID.fromString("12345678-1234-1234-1234-123456789abc")
    val TX_CHAR_UUID: UUID = UUID.fromString("87654321-4321-4321-4321-cba987654321")
    val RX_CHAR_UUID: UUID = UUID.fromString("11111111-2222-3333-4444-555555555555")
}


interface ConnectionCallback {
    fun onConnectionStateChanged(isConnected: Boolean, deviceName: String?)
    fun onScanningStateChanged(isScanning: Boolean)
    fun onPulseraDataReceived(data: PulseraData)
    fun logMessage(message: String) // Callback para logs centralizados
}

class BLEManager(private val context: Context) {

    private val bluetoothManager: BluetoothManager =
        context.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
    private val bluetoothAdapter: BluetoothAdapter = bluetoothManager.adapter
    private val scanner = bluetoothAdapter.bluetoothLeScanner
    private val handler = Handler(Looper.getMainLooper()) // Handler para el timeout

    private var bluetoothGatt: BluetoothGatt? = null
    private var rxCharacteristic: BluetoothGattCharacteristic? = null

    var connectionCallback: ConnectionCallback? = null
    private var isScanning = false

    companion object {
        private const val SCAN_PERIOD: Long = 15000 // 15 segundos de escaneo
        private const val DEVICE_NAME = "ACPI_Pulsera" // Nombre del dispositivo BLE
    }

    @SuppressLint("MissingPermission")
    private val scanCallback = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            val device = result.device
            if (device.name == DEVICE_NAME) {
                connectionCallback?.logMessage("Dispositivo '$DEVICE_NAME' encontrado.")
                stopScan()
                device.connectGatt(context, false, gattCallback)
            }
        }

        override fun onScanFailed(errorCode: Int) {
            connectionCallback?.logMessage("Error de escaneo BLE: $errorCode")
            isScanning = false
            connectionCallback?.onScanningStateChanged(false)
        }
    }

    private val gattCallback = object : BluetoothGattCallback() {
        @SuppressLint("MissingPermission")
        override fun onConnectionStateChange(gatt: BluetoothGatt, status: Int, newState: Int) {
            val deviceName = gatt.device?.name ?: "Dispositivo"
            if (status == BluetoothGatt.GATT_SUCCESS) {
                if (newState == BluetoothProfile.STATE_CONNECTED) {
                    bluetoothGatt = gatt
                    connectionCallback?.logMessage("Conectado a $deviceName. Descubriendo servicios...")
                    connectionCallback?.onConnectionStateChanged(true, deviceName)
                    gatt.discoverServices()
                } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                    connectionCallback?.logMessage("Desconectado de $deviceName.")
                    connectionCallback?.onConnectionStateChanged(false, null)
                    // No llames a disconnect() aquí para evitar bucles, el estado ya se ha notificado.
                }
            } else {
                connectionCallback?.logMessage("Error de conexión GATT. Status: $status")
                connectionCallback?.onConnectionStateChanged(false, null)
                disconnect()
            }
        }

        @SuppressLint("MissingPermission")
        override fun onServicesDiscovered(gatt: BluetoothGatt, status: Int) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                val service = gatt.getService(BLEConstants.SERVICE_UUID)
                if (service == null) {
                    connectionCallback?.logMessage("ERROR: Servicio BLE no encontrado.")
                    disconnect()
                    return
                }

                val txCharacteristic = service.getCharacteristic(BLEConstants.TX_CHAR_UUID)
                rxCharacteristic = service.getCharacteristic(BLEConstants.RX_CHAR_UUID)

                if (txCharacteristic == null || rxCharacteristic == null) {
                    connectionCallback?.logMessage("ERROR: Una o más características no encontradas.")
                    disconnect()
                    return
                }

                gatt.setCharacteristicNotification(txCharacteristic, true)
                val descriptor = txCharacteristic.getDescriptor(UUID.fromString("00002902-0000-1000-8000-00805f9b34fb"))
                descriptor.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
                gatt.writeDescriptor(descriptor)
                connectionCallback?.logMessage("Suscrito a notificaciones. Listo.")

            } else {
                connectionCallback?.logMessage("ERROR: No se pudieron descubrir los servicios. Status: $status")
                disconnect()
            }
        }

        @Deprecated("Deprecated in Java")
        override fun onCharacteristicChanged(gatt: BluetoothGatt, characteristic: BluetoothGattCharacteristic) {
            if (characteristic.uuid == BLEConstants.TX_CHAR_UUID) {
                val value = characteristic.value ?: return
                val message = String(value, Charsets.UTF_8)
                connectionCallback?.logMessage("Notificación recibida: \"$message\"")

                try {
                    val json = JSONObject(message)
                    val data = PulseraData(
                        hd = json.getInt("hd"),
                        st = json.getInt("st"),
                        ok = json.getBoolean("ok"),
                        tr = json.getInt("tr"),
                        bt = json.getInt("bt")
                    )
                    connectionCallback?.onPulseraDataReceived(data)
                } catch (e: Exception) {
                    connectionCallback?.logMessage("Error al parsear el mensaje JSON del ESP32: ${e.message}")
                }
            }
        }


        @SuppressLint("MissingPermission")
        override fun onCharacteristicWrite(gatt: BluetoothGatt?, characteristic: BluetoothGattCharacteristic?, status: Int) {
            super.onCharacteristicWrite(gatt, characteristic, status)
            if (status == BluetoothGatt.GATT_SUCCESS) {
                Log.d("BLEManager", "Datos enviados exitosamente.")
            } else {
                Log.e("BLEManager", "Error al escribir en la característica. Estado: $status")
            }
        }
    }

    @SuppressLint("MissingPermission")
    fun startScan() {
        if (isScanning) return
        if (!bluetoothAdapter.isEnabled) {
            connectionCallback?.logMessage("Error: Bluetooth no está activado.")
            return
        }

        val scanFilter = ScanFilter.Builder().setDeviceName(DEVICE_NAME).build()
        val settings = ScanSettings.Builder().setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY).build()

        handler.postDelayed({
            if (isScanning) {
                connectionCallback?.logMessage("Tiempo de escaneo agotado. Dispositivo no encontrado.")
                stopScan()
            }
        }, SCAN_PERIOD)

        isScanning = true
        scanner.startScan(listOf(scanFilter), settings, scanCallback)
        connectionCallback?.onScanningStateChanged(true)
        connectionCallback?.logMessage("Escaneando para '$DEVICE_NAME'...")
    }

    @SuppressLint("MissingPermission")
    fun stopScan() {
        if (isScanning) {
            handler.removeCallbacksAndMessages(null)
            scanner.stopScan(scanCallback)
            isScanning = false
            connectionCallback?.onScanningStateChanged(false)
            connectionCallback?.logMessage("Escaneo detenido.")
        }
    }

    @SuppressLint("MissingPermission")
    fun sendQuadrantToEsp32(cuadrante: Int) {
        val gatt = bluetoothGatt
        val characteristic = rxCharacteristic
        if (gatt != null && characteristic != null) {
            val data = byteArrayOf(cuadrante.toByte())
            characteristic.value = data
            val success = gatt.writeCharacteristic(characteristic)
            Log.d("BLEManager", "Intentando enviar cuadrante: $cuadrante. Éxito: $success")
        } else {
            Log.e("BLEManager", "No se pueden enviar datos. GATT o characteristic es nulo.")
        }
    }

    @SuppressLint("MissingPermission")
    fun sendCommandToEsp32(command: String) {
        val gatt = bluetoothGatt
        val characteristic = rxCharacteristic
        if (gatt != null && characteristic != null) {
            characteristic.value = command.toByteArray(Charsets.UTF_8)
            val success = gatt.writeCharacteristic(characteristic)
            Log.d("BLEManager", "Intentando enviar comando: '$command'. Éxito: $success")
        } else {
            Log.e("BLEManager", "No se pueden enviar comandos. GATT o characteristic es nulo.")
        }
    }

    @SuppressLint("MissingPermission")
    fun disconnect() {
        if (bluetoothGatt != null) {
            stopScan()
            bluetoothGatt?.close()
            bluetoothGatt = null
            rxCharacteristic = null
            connectionCallback?.onConnectionStateChanged(false, null)
            Log.d("BLEManager", "Desconectando BLE.")
        }
    }
}
