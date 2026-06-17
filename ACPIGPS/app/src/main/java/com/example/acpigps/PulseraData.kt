package com.example.acpigps

data class PulseraData(
    val hd: Int,      // Heading de la pulsera (0-359°)
    val st: Int,      // Estado: 0=cruce_prohibido(rojo_peatón), 1=cruce_permitido(verde_peatón), 2=orientacion_mal, 3=panico
    val ok: Boolean,  // true si la orientación es correcta frente al semáforo
    val tr: Int,      // Tiempo restante del semáforo en segundos
    val bt: Int       // Batería estimada de la pulsera (%) o -1 si no disponible
)