package com.example.acpigps

import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import android.os.Build
import androidx.core.app.NotificationCompat

class LocalNotificationManager(private val context: Context) {

    companion object {
        private const val ALERTS_CHANNEL_ID = "ACPI_ALERTS_CHANNEL"
        private const val ALERTS_CHANNEL_NAME = "Alertas de Seguridad ACPI"
        private const val NOTIFICATION_BASE_ID = 100
    }

    init {
        createNotificationChannel()
    }

    private fun createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val channel = NotificationChannel(
                ALERTS_CHANNEL_ID,
                ALERTS_CHANNEL_NAME,
                NotificationManager.IMPORTANCE_HIGH
            ).apply {
                description = "Canal para notificaciones críticas de seguridad del sistema ACPI"
                enableVibration(true)
            }
            val manager = context.getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
            manager.createNotificationChannel(channel)
        }
    }

    fun showNotification(title: String, message: String, notificationId: Int) {
        val intent = Intent(context, MainActivity::class.java).apply {
            flags = Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TASK
        }
        val pendingIntent = PendingIntent.getActivity(
            context,
            0,
            intent,
            PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT
        )

        val notification = NotificationCompat.Builder(context, ALERTS_CHANNEL_ID)
            .setContentTitle(title)
            .setContentText(message)
            .setSmallIcon(R.drawable.ic_launcher_foreground)
            .setAutoCancel(true)
            .setPriority(NotificationCompat.PRIORITY_HIGH)
            .setDefaults(NotificationCompat.DEFAULT_ALL)
            .setContentIntent(pendingIntent)
            .build()

        val manager = context.getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
        manager.notify(notificationId, notification)
    }

    fun alertGeofenceViolation(distance: Int) {
        showNotification(
            "⚠️ Salida de Zona Segura",
            "Has salido de tu zona segura por $distance metros.",
            NOTIFICATION_BASE_ID + 1
        )
    }

    fun alertSafeToCross() {
        showNotification(
            "✅ Cruce Permitido",
            "El semáforo está a tu favor. Puedes cruzar de forma segura.",
            NOTIFICATION_BASE_ID + 2
        )
    }

    fun alertLowBattery(batteryLevel: Int) {
        showNotification(
            "🔋 Batería de Pulsera Baja",
            "La batería de la pulsera está en $batteryLevel%. Favor de cargarla.",
            NOTIFICATION_BASE_ID + 3
        )
    }
}
