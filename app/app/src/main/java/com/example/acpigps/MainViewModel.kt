package com.example.acpigps

import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel

// Data classes para representar el estado de la UI
data class BleConnectionState(
    val isConnected: Boolean = false,
    val isScanning: Boolean = false,
    val deviceName: String? = null
)

data class CompassData(
    val angle: Float = 0f,
    val direction: String = "N",
    val steps: Int = 0
)

class MainViewModel : ViewModel() {

    private val _bleConnectionState = MutableLiveData(BleConnectionState())
    val bleConnectionState: LiveData<BleConnectionState> = _bleConnectionState

    private val _logMessages = MutableLiveData<List<String>>(emptyList())
    val logMessages: LiveData<List<String>> = _logMessages

    private val _compassData = MutableLiveData(CompassData())
    val compassData: LiveData<CompassData> = _compassData

    private val _isMonitoring = MutableLiveData<Boolean>(false)
    val isMonitoring: LiveData<Boolean> = _isMonitoring

    private val _brazaleteData = MutableLiveData<BrazaleteData?>(null)
    val brazaleteData: LiveData<BrazaleteData?> = _brazaleteData

    fun setMonitoringState(isMonitoring: Boolean) {
        _isMonitoring.value = isMonitoring
    }

    // --- Métodos para que la MainActivity actualice el ViewModel ---
    // Estos métodos reciben el estado completo desde el servicio
    fun onConnectionStateChanged(newState: BleConnectionState) {
        _bleConnectionState.value = newState
    }

    fun updateCompassData(newData: CompassData) {
        _compassData.value = newData
    }

    fun updateLogMessages(newLogs: List<String>) {
        _logMessages.value = newLogs
    }

    fun updateBrazaleteData(newData: BrazaleteData?) {
        _brazaleteData.value = newData
    }
}

