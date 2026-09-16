package com.example.acpigps

import android.content.Context
import android.util.Log
import android.util.Base64
import okhttp3.*
import okhttp3.MediaType.Companion.toMediaType
import okhttp3.RequestBody.Companion.toRequestBody
import org.json.JSONArray
import org.json.JSONObject
import java.io.IOException
import java.util.concurrent.ConcurrentLinkedQueue

class GrafanaReporter(private val context: Context) {

    companion object {
        private const val TAG = "GrafanaReporter"
        private const val DEFAULT_URL = BuildConfig.GRAFANA_URL
        private const val DEFAULT_TOKEN = BuildConfig.GRAFANA_TOKEN
        private const val DEFAULT_USER = BuildConfig.GRAFANA_USER
    }

    private val client = OkHttpClient()
    private val bufferQueue = ConcurrentLinkedQueue<String>()

    fun sendSessionTelemetry(
        deviceId: String,
        sessionId: String,
        status: Int,
        metrics: JSONObject
    ) {
        // Construir el JSON plano de la sesión
        val data = JSONObject().apply {
            put("device_id", deviceId)
            put("session_id", sessionId)
            put("status", status)
            val keys = metrics.keys()
            while (keys.hasNext()) {
                val key = keys.next()
                put(key, metrics.get(key))
            }
        }

        // Formato Loki: { "streams": [{ "stream": {labels}, "values": [["<ns>", "<line>"]] }] }
        val timestampNs = (System.currentTimeMillis() * 1_000_000L).toString()
        val lokiPayload = JSONObject().apply {
            put("streams", JSONArray().apply {
                put(JSONObject().apply {
                    put("stream", JSONObject().apply {
                        put("app", "acpi")
                        put("device_id", deviceId)
                    })
                    put("values", JSONArray().apply {
                        put(JSONArray().apply {
                            put(timestampNs)
                            put(data.toString())
                        })
                    })
                })
            })
        }

        bufferQueue.add(lokiPayload.toString())
        while (bufferQueue.size > 100) bufferQueue.poll()
        enviarSiguiente()
    }

    private fun enviarSiguiente() {
        val payload = bufferQueue.peek() ?: return

        val prefs = context.getSharedPreferences("app_prefs", Context.MODE_PRIVATE)
        val url = prefs.getString("grafana_url", DEFAULT_URL) ?: DEFAULT_URL
        val token = prefs.getString("grafana_token", DEFAULT_TOKEN) ?: DEFAULT_TOKEN
        val user = prefs.getString("grafana_user", DEFAULT_USER) ?: DEFAULT_USER

        if (url.isBlank() || token.isBlank() || user.isBlank()) {
            Log.w(TAG, "Telemetria descartada: credenciales de Grafana incompletas.")
            bufferQueue.poll()
            return
        }

        val credentials = Base64.encodeToString("$user:$token".toByteArray(), Base64.NO_WRAP)
        val requestBody = payload.toRequestBody("application/json; charset=utf-8".toMediaType())
        val request = Request.Builder()
            .url(url)
            .post(requestBody)
            .addHeader("Authorization", "Basic $credentials")
            .build()

        client.newCall(request).enqueue(object : Callback {
            override fun onFailure(call: Call, e: IOException) {
                Log.e(TAG, "Fallo conexión a Loki: ${e.message}. Reintentando en próximo envío.")
            }

            override fun onResponse(call: Call, response: Response) {
                response.use {
                    if (response.isSuccessful) {
                        Log.d(TAG, "Sesión enviada a Loki correctamente.")
                        bufferQueue.poll()
                        enviarSiguiente()
                    } else {
                        Log.e(TAG, "Loki rechazó el payload. Código: ${response.code}. Body: ${response.body?.string()}")
                        bufferQueue.poll()
                    }
                }
            }
        })
    }
}
