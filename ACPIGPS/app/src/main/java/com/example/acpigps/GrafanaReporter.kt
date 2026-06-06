package com.example.acpigps

import android.content.Context
import android.util.Log
import okhttp3.*
import okhttp3.MediaType.Companion.toMediaType
import okhttp3.RequestBody.Companion.toRequestBody
import org.json.JSONObject
import java.io.IOException
import java.util.concurrent.ConcurrentLinkedQueue

class GrafanaReporter(private val context: Context) {

    companion object {
        private const val TAG = "GrafanaReporter"
        
        // URL y Token de configuración. Se cargan de SharedPreferences o por defecto.
        // Nota: Grafana Cloud InfluxDB o HTTP JSON collector URL se puede cambiar en SharedPreferences.
        private const val DEFAULT_URL = "https://influx-prod-13-prod-us-east-0.grafana.net/api/v1/push"
        private const val DEFAULT_TOKEN = "YOUR_GRAFANA_CLOUD_API_TOKEN"
    }

    private val client = OkHttpClient()
    private val bufferQueue = ConcurrentLinkedQueue<String>() // Ring-buffer en memoria para resiliencia

    fun sendSessionTelemetry(
        deviceId: String,
        sessionId: String,
        status: Int,
        metrics: JSONObject
    ) {
        val payload = JSONObject().apply {
            put("device_id", deviceId)
            put("session_id", sessionId)
            put("status", status)
            put("timestamp", System.currentTimeMillis())
            
            // Aplanar todas las métricas en llaves de primer nivel para simplificar series temporales
            val keys = metrics.keys()
            while (keys.hasNext()) {
                val key = keys.next()
                put(key, metrics.get(key))
            }
        }

        val payloadString = payload.toString()
        bufferQueue.add(payloadString)
        
        // Limitar tamaño del buffer a 100 elementos para evitar fugas de memoria
        while (bufferQueue.size > 100) {
            bufferQueue.poll()
        }

        enviarSiguiente()
    }

    private fun enviarSiguiente() {
        val payload = bufferQueue.peek() ?: return

        // Obtener credenciales dinámicas de SharedPreferences o usar las por defecto
        val prefs = context.getSharedPreferences("app_prefs", Context.MODE_PRIVATE)
        val url = prefs.getString("grafana_url", DEFAULT_URL) ?: DEFAULT_URL
        val token = prefs.getString("grafana_token", DEFAULT_TOKEN) ?: DEFAULT_TOKEN

        if (url == DEFAULT_URL && token == DEFAULT_TOKEN) {
            Log.w(TAG, "Telemetria almacenada pero no enviada: URL/Token de Grafana no configurados.")
            // Para la demo, simulamos éxito para no llenar el buffer indefinidamente
            bufferQueue.poll()
            return
        }

        val mediaType = "application/json; charset=utf-8".toMediaType()
        val requestBody = payload.toRequestBody(mediaType)

        val request = Request.Builder()
            .url(url)
            .post(requestBody)
            .addHeader("Authorization", "Bearer $token")
            .build()

        client.newCall(request).enqueue(object : Callback {
            override fun onFailure(call: Call, e: IOException) {
                Log.e(TAG, "Fallo conexion a Grafana Cloud: ${e.message}. Telemetría guardada para reintento.")
                // En caso de fallo de red, se queda en cola y se reintentará en el próximo evento.
            }

            override fun onResponse(call: Call, response: Response) {
                response.use {
                    if (response.isSuccessful) {
                        Log.d(TAG, "Sesión de telemetría reportada con éxito a Grafana.")
                        bufferQueue.poll() // Quitar de la cola
                        enviarSiguiente() // Procesar siguientes acumulados
                    } else {
                        Log.e(TAG, "Grafana rechazó payload con código ${response.code}. Descartando.")
                        bufferQueue.poll() // Quitar para evitar atasco
                    }
                }
            }
        })
    }
}
