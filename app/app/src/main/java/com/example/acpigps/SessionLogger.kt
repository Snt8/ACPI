package com.example.acpigps

import android.content.Context
import android.location.Location
import android.util.Log
import org.json.JSONObject
import java.util.UUID

class SessionLogger(private val context: Context, private val reporter: GrafanaReporter) {

    private var sessionId: String? = null
    private var isSessionActive = false
    private var startTimestamp: Long = 0
    private var waitingStartTimestamp: Long = 0
    private var totalWaitingTimeMs: Long = 0
    private var panicCount = 0
    private var stepsAtStart = 0
    private var allowedCruce = false
    private var wasDisaligned = false

    fun startSession(currentSteps: Int) {
        sessionId = UUID.randomUUID().toString()
        isSessionActive = true
        startTimestamp = System.currentTimeMillis()
        waitingStartTimestamp = startTimestamp
        totalWaitingTimeMs = 0
        panicCount = 0
        stepsAtStart = currentSteps
        allowedCruce = false
        Log.d("SessionLogger", "Sesion de cruce iniciada: $sessionId")
    }

    fun logData(data: PulseraData, location: Location?, currentSteps: Int) {
        if (!isSessionActive) return

        // Incrementar el contador de pánicos
        if (data.st == 3) {
            panicCount++
        }

        // st=1: cruce permitido (orientado + semáforo rojo para autos = verde peatón)
        if (data.st == 1) {
            allowedCruce = true
        }

        // st=2: orientación incorrecta
        if (data.st == 2) {
            wasDisaligned = true
        }

        // Lógica de cronómetro para tiempo de espera
        val ahora = System.currentTimeMillis()
        if (data.st != 0 && waitingStartTimestamp != 0L) {
            totalWaitingTimeMs += ahora - waitingStartTimestamp
            waitingStartTimestamp = 0L
        } else if (data.st == 0 && waitingStartTimestamp == 0L) {
            waitingStartTimestamp = ahora
        }
    }

    fun endSession(currentSteps: Int, finalLocation: Location?) {
        if (!isSessionActive || sessionId == null) return

        val ahora = System.currentTimeMillis()
        val durationMs = ahora - startTimestamp
        val durationSec = durationMs / 1000L

        // Cerrar el último conteo de tiempo de espera
        if (waitingStartTimestamp != 0L) {
            totalWaitingTimeMs += ahora - waitingStartTimestamp
        }
        val waitingSec = totalWaitingTimeMs / 1000L
        val totalSteps = currentSteps - stepsAtStart

        // Construir el objeto de métricas
        val metrics = JSONObject().apply {
            put("duration_sec", durationSec)
            put("waiting_sec", waitingSec)
            put("steps", totalSteps)
            put("panics", panicCount)
            put("allowed_cruce", if (allowedCruce) 1 else 0)
            put("lat", finalLocation?.latitude ?: 0.0)
            put("lon", finalLocation?.longitude ?: 0.0)
        }

        // Enviar a Grafana Cloud
        reporter.sendSessionTelemetry(
            deviceId = "ACPI_Pulsera",
            sessionId = sessionId!!,
            status = when {
                panicCount > 0  -> 3
                wasDisaligned   -> 2
                allowedCruce    -> 1
                else            -> 0
            },
            metrics = metrics
        )

        Log.d("SessionLogger", "Sesion de cruce finalizada y enviada: $sessionId")
        
        isSessionActive = false
        sessionId = null
    }

    fun isSessionActive(): Boolean = isSessionActive
}
