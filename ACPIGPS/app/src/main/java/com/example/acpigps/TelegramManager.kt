package com.example.acpigps

import android.content.Context
import android.util.Log
import okhttp3.Call
import okhttp3.Callback
import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.Response
import java.io.IOException

class TelegramManager(private val context: Context) {

    // REEMPLAZA ESTOS VALORES CON LOS TUYOS
    private val botToken = ""
    private val chatId = ""

    private val client = OkHttpClient()

    // Método para enviar un mensaje de texto simple
    fun sendMessage(message: String, onLogUpdated: (String) -> Unit) {
        if (botToken.startsWith("TU_") || chatId.startsWith("TU_")) {
            val errorMsg = "ERROR: Configura el token y chat ID en TelegramManager.kt"
            Log.e("TelegramManager", errorMsg)
            onLogUpdated(errorMsg)
            return
        }

        val encodedMessage = java.net.URLEncoder.encode(message, "UTF-8")
        val url = "https://api.telegram.org/bot$botToken/sendMessage?chat_id=$chatId&text=$encodedMessage"
        val request = Request.Builder().url(url).build()

        client.newCall(request).enqueue(object : Callback {
            override fun onFailure(call: Call, e: IOException) {
                val errorMessage = "Error al enviar mensaje a Telegram: ${e.message}"
                Log.e("TelegramManager", errorMessage)
                onLogUpdated(errorMessage)
            }

            override fun onResponse(call: Call, response: Response) {
                // 👇 SOLUCIÓN 1: Usar 'response.use' para cerrar la conexión automáticamente
                response.use {
                    if (it.isSuccessful) {
                        val successMessage = "Mensaje enviado a Telegram con éxito."
                        Log.d("TelegramManager", successMessage)
                        onLogUpdated(successMessage)
                    } else {
                        val errorMessage = "Error en la respuesta de Telegram: ${it.code} - ${it.message}"
                        Log.e("TelegramManager", errorMessage)
                        onLogUpdated(errorMessage)
                    }
                }
            }
        })
    }

    // Método para enviar la ubicación en formato de enlace de Google Maps
    fun sendLocation(latitude: Double, longitude: Double, onLogUpdated: (String) -> Unit) {
        // 👇 SOLUCIÓN 2: Corregir el enlace de Google Maps
        val mapsLink = "https://www.google.com/maps?q=$latitude,$longitude"
        val message = "Mi ubicación actual: $mapsLink"
        sendMessage(message, onLogUpdated)
    }
}
