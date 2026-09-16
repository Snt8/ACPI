package com.example.acpigps

import android.content.Context
import android.hardware.Sensor
import android.hardware.SensorEvent
import android.hardware.SensorEventListener
import android.hardware.SensorManager
import java.util.ArrayDeque

class WalkDirectionDetector(
    context: Context,
    private val onDirectionChanged: (Float, String) -> Unit
) : SensorEventListener {

    private val sensorManager = context.getSystemService(Context.SENSOR_SERVICE) as SensorManager
    private val accelerometer = sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER)
    private val magnetometer = sensorManager.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD)

    // --- Variables para la Brújula ---
    private val gravity = FloatArray(3)
    private val geomagnetic = FloatArray(3)
    private val rotationMatrix = FloatArray(9)
    private val orientationAngles = FloatArray(3)
    private var currentAngle = 0f
    private var currentQuadrant = 0

    // --- Variables para el Contador de Pasos ---
    private var stepThreshold = 10.5f
    private var lastStepTime: Long = 0
    private val stepDelay: Long = 300

    // --- Variables para el filtro de media móvil ---
    private val windowSize = 10 // Usaremos el promedio de las últimas 10 lecturas
    private val accelReadings = ArrayDeque<FloatArray>(windowSize)
    private val magReadings = ArrayDeque<FloatArray>(windowSize)

    // --- Variables para la robustez del cuadrante ---
    private var potentialQuadrant = 0
    private var quadrantConfirmCount = 0
    private val quadrantConfirmationThreshold = 5

    fun startDetection() {
        sensorManager.registerListener(this, accelerometer, SensorManager.SENSOR_DELAY_UI)
        sensorManager.registerListener(this, magnetometer, SensorManager.SENSOR_DELAY_UI)
    }

    fun stopDetection() {
        sensorManager.unregisterListener(this)
    }

    fun getCurrentQuadrant(): Int = currentQuadrant

    override fun onSensorChanged(event: SensorEvent) {
        if (event.sensor.type == Sensor.TYPE_ACCELEROMETER) {
            // Lógica del filtro de media móvil
            if (accelReadings.size >= windowSize) {
                accelReadings.removeFirst()
            }
            accelReadings.addLast(event.values.clone())

            val avg = floatArrayOf(0f, 0f, 0f)
            for (reading in accelReadings) {
                avg[0] += reading[0]
                avg[1] += reading[1]
                avg[2] += reading[2]
            }
            gravity[0] = avg[0] / accelReadings.size
            gravity[1] = avg[1] / accelReadings.size
            gravity[2] = avg[2] / accelReadings.size

            // Lógica del Contador de Pasos
            val magnitude = kotlin.math.sqrt(gravity[0] * gravity[0] + gravity[1] * gravity[1] + gravity[2] * gravity[2])
            val currentTime = System.currentTimeMillis()
            if (magnitude > stepThreshold && (currentTime - lastStepTime) > stepDelay) {
                lastStepTime = currentTime
                onDirectionChanged(currentAngle, getDirectionFromAngle(currentAngle))
            }
        }

        if (event.sensor.type == Sensor.TYPE_MAGNETIC_FIELD) {
            // Lógica del filtro de media móvil
            if (magReadings.size >= windowSize) {
                magReadings.removeFirst()
            }
            magReadings.addLast(event.values.clone())

            val avg = floatArrayOf(0f, 0f, 0f)
            for (reading in magReadings) {
                avg[0] += reading[0]
                avg[1] += reading[1]
                avg[2] += reading[2]
            }
            geomagnetic[0] = avg[0] / magReadings.size
            geomagnetic[1] = avg[1] / magReadings.size
            geomagnetic[2] = avg[2] / magReadings.size
        }

        val success = SensorManager.getRotationMatrix(rotationMatrix, null, gravity, geomagnetic)
        if (success) {
            SensorManager.getOrientation(rotationMatrix, orientationAngles)
            val azimuth = Math.toDegrees(orientationAngles[0].toDouble()).toFloat()
            currentAngle = (azimuth + 360) % 360

            val newQuadrant = getQuadrantFromAngle(currentAngle)
            if (newQuadrant == potentialQuadrant) {
                quadrantConfirmCount++
            } else {
                potentialQuadrant = newQuadrant
                quadrantConfirmCount = 1
            }

            if (quadrantConfirmCount >= quadrantConfirmationThreshold) {
                currentQuadrant = potentialQuadrant
            }
        }
    }

    private fun getDirectionFromAngle(angle: Float): String {
        // --- LÓGICA DE DIRECCIÓN MEJORADA ---
        // Se amplían los rangos de las direcciones cardinales (N, S, E, O)
        // para darles prioridad y hacer la detección más estable.
        return when (angle) {
            in 330.0..360.0, in 0.0..<30.0 -> "Norte"    // 60°
            in 30.0..<60.0 -> "Noreste"   // 30°
            in 60.0..<120.0 -> "Este"      // 60°
            in 120.0..<150.0 -> "Sureste"  // 30°
            in 150.0..<210.0 -> "Sur"       // 60°
            in 210.0..<240.0 -> "Suroeste" // 30°
            in 240.0..<300.0 -> "Oeste"     // 60°
            in 300.0..<330.0 -> "Noroeste"  // 30°
            else -> "Norte" // Fallback seguro
        }
    }

    private fun getQuadrantFromAngle(angle: Float): Int {
        // Los cuadrantes mantienen su rango amplio de 90° para la lógica de seguridad
        return when (angle) {
            in 45.0..135.0 -> 1 // Este
            in 135.0..225.0 -> 2 // Sur
            in 225.0..315.0 -> 3 // Oeste
            else -> 4 // Norte
        }
    }

    override fun onAccuracyChanged(sensor: Sensor?, accuracy: Int) {}
}
