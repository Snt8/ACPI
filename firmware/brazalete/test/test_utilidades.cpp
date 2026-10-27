// Pruebas de la geometría de orientación y del protocolo ESP-NOW.
#include "minitest.h"
#include "../src/utils/calcular_direccion.h"
#include "../src/protocolo/protocolo_esp.h"
#include <cstring>

// ── calcularDireccion: giro más corto, + derecha / − izquierda ───────────────

PRUEBA(direccion_giro_mas_corto) {
    VERIFICAR_CERCA(calcularDireccion(90.0f, 0.0f), 90.0f, 0.01);     // objetivo a la derecha
    VERIFICAR_CERCA(calcularDireccion(0.0f, 90.0f), -90.0f, 0.01);    // a la izquierda
    VERIFICAR_CERCA(calcularDireccion(10.0f, 350.0f), 20.0f, 0.01);   // cruza el norte
    VERIFICAR_CERCA(calcularDireccion(350.0f, 10.0f), -20.0f, 0.01);
    VERIFICAR_CERCA(calcularDireccion(45.0f, 45.0f), 0.0f, 0.01);
    VERIFICAR_CERCA(calcularDireccion(180.0f, 0.0f), 180.0f, 0.01);   // empate: derecha
}

PRUEBA(direccion_fuera_de_rango) {
    // 400° equivale a 40°: el giro hasta 10° es de 30° a la izquierda
    VERIFICAR_CERCA(calcularDireccion(10.0f, 400.0f), -30.0f, 0.01);
    VERIFICAR_CERCA(calcularDireccion(10.0f, 760.0f), -30.0f, 0.01);
}

PRUEBA(direccion_resultado_siempre_en_rango) {
    for (int objetivo = 0; objetivo < 360; objetivo += 7) {
        for (int actual = 0; actual < 360; actual += 11) {
            float g = calcularDireccion((float)objetivo, (float)actual);
            VERIFICAR(g > -180.0f - 0.01f && g <= 180.0f + 0.01f);
        }
    }
}

// ── desviacionEnfrentamiento: 0 = de frente al semáforo ──────────────────────

PRUEBA(desviacion_enfrentados_es_cero) {
    VERIFICAR_CERCA(desviacionEnfrentamiento(270.0f, 90.0f), 0.0f, 0.01);
    VERIFICAR_CERCA(desviacionEnfrentamiento(0.0f, 180.0f), 0.0f, 0.01);
    VERIFICAR_CERCA(desviacionEnfrentamiento(10.0f, 190.0f), 0.0f, 0.01);
}

PRUEBA(desviacion_mismo_sentido_es_180) {
    VERIFICAR_CERCA(desviacionEnfrentamiento(90.0f, 90.0f), 180.0f, 0.01);
}

PRUEBA(desviacion_cruza_el_norte) {
    // 350° y 200°: arco menor 150°, le faltan 30° para enfrentarse
    VERIFICAR_CERCA(desviacionEnfrentamiento(350.0f, 200.0f), 30.0f, 0.01);
    VERIFICAR_CERCA(desviacionEnfrentamiento(200.0f, 350.0f), 30.0f, 0.01);
    VERIFICAR_CERCA(desviacionEnfrentamiento(5.0f, 160.0f), 25.0f, 0.01);
}

PRUEBA(arco_menor) {
    VERIFICAR_CERCA(arcoMenor(350.0f, 10.0f), 20.0f, 0.01);
    VERIFICAR_CERCA(arcoMenor(10.0f, 350.0f), 20.0f, 0.01);
    VERIFICAR_CERCA(arcoMenor(0.0f, 180.0f), 180.0f, 0.01);
    VERIFICAR_CERCA(arcoMenor(90.0f, 90.0f), 0.0f, 0.01);
}

PRUEBA(desviacion_y_giro_son_coherentes) {
    // Girar lo que indica giroParaEnfrentar (lo que usa la FSM) deja la desviación en 0
    for (int hs = 0; hs < 360; hs += 13) {
        for (int hp = 0; hp < 360; hp += 17) {
            float giro = giroParaEnfrentar((float)hp, (float)hs);
            float nuevo = (float)hp + giro;
            while (nuevo < 0.0f) nuevo += 360.0f;
            while (nuevo >= 360.0f) nuevo -= 360.0f;
            VERIFICAR_CERCA(desviacionEnfrentamiento(nuevo, (float)hs), 0.0f, 0.05);
            // y la magnitud del giro es la desviación
            VERIFICAR_CERCA(giro < 0 ? -giro : giro,
                            desviacionEnfrentamiento((float)hp, (float)hs), 0.05);
        }
    }
}

// ── Protocolo ────────────────────────────────────────────────────────────────

namespace {
estado_semaforo paqueteValido() {
    estado_semaforo p{};
    firmarPaqueteSemaforo(&p);
    p.id = 1;
    p.grados = 270;
    p.permitidoCruce = 1;
    p.tiempo = 12;
    sellarPaqueteSemaforo(&p);
    return p;
}
} // namespace

PRUEBA(protocolo_tamano_firma_y_crc) {
    VERIFICAR_IGUAL(sizeof(estado_semaforo), 10);
    estado_semaforo p = paqueteValido();
    VERIFICAR(std::memcmp(p.firma, "ACPI", 4) == 0);
    VERIFICAR(esPaqueteSemaforo((const uint8_t*)&p, (int)sizeof(p)));
}

PRUEBA(protocolo_crc8_valores_conocidos) {
    // CRC-8/SMBUS (poly 0x07, init 0): "123456789" -> 0xF4 (valor de referencia)
    const uint8_t texto[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    VERIFICAR_IGUAL(crc8Semaforo(texto, 9), 0xF4);
    VERIFICAR_IGUAL(crc8Semaforo(texto, 0), 0x00);
}

PRUEBA(protocolo_rechaza_paquetes_ajenos) {
    estado_semaforo p = paqueteValido();
    const uint8_t* bytes = (const uint8_t*)&p;
    VERIFICAR(!esPaqueteSemaforo(nullptr, (int)sizeof(p)));
    VERIFICAR(!esPaqueteSemaforo(bytes, (int)sizeof(p) - 1));  // truncado / formato de 9 bytes
    VERIFICAR(!esPaqueteSemaforo(bytes, (int)sizeof(p) + 1));
    VERIFICAR(!esPaqueteSemaforo(bytes, 0));
    p.firma[3] = 'X';
    VERIFICAR(!esPaqueteSemaforo(bytes, (int)sizeof(p)));
    // El paquete viejo de 5 bytes (sin firma) no se acepta
    uint8_t viejo[5] = {1, 90, 0, 1, 10};
    VERIFICAR(!esPaqueteSemaforo(viejo, 5));
}

PRUEBA(protocolo_rechaza_cualquier_byte_alterado) {
    // Interferencia: cambiar un solo bit de cualquier byte invalida el paquete
    estado_semaforo original = paqueteValido();
    for (int byte = 0; byte < (int)sizeof(original); ++byte) {
        for (int bit = 0; bit < 8; ++bit) {
            estado_semaforo p = original;
            ((uint8_t*)&p)[byte] ^= (uint8_t)(1u << bit);
            VERIFICAR(!esPaqueteSemaforo((const uint8_t*)&p, (int)sizeof(p)));
        }
    }
}

PRUEBA(protocolo_rechaza_campos_fuera_de_rango) {
    // Aunque el CRC sea correcto, grados >= 360 o permitidoCruce > 1 no son válidos
    estado_semaforo p = paqueteValido();
    p.grados = 360;
    sellarPaqueteSemaforo(&p);
    VERIFICAR(!esPaqueteSemaforo((const uint8_t*)&p, (int)sizeof(p)));
    p = paqueteValido();
    p.permitidoCruce = 2;
    sellarPaqueteSemaforo(&p);
    VERIFICAR(!esPaqueteSemaforo((const uint8_t*)&p, (int)sizeof(p)));
    p = paqueteValido();
    p.grados = 359;
    p.permitidoCruce = 0;
    sellarPaqueteSemaforo(&p);
    VERIFICAR(esPaqueteSemaforo((const uint8_t*)&p, (int)sizeof(p)));
}

PRUEBA(protocolo_rafaga_de_ruido_no_pasa) {
    // Diez mil paquetes de bytes al azar con la firma correcta: ninguno debe
    // pasar con un campo fuera de rango, y casi todos deben fallar el CRC
    uint32_t estado = 2024;
    int aceptados = 0;
    for (int i = 0; i < 10000; ++i) {
        uint8_t ruido[sizeof(estado_semaforo)];
        for (size_t k = 0; k < sizeof(ruido); ++k) {
            estado = estado * 1664525u + 1013904223u;
            ruido[k] = (uint8_t)(estado >> 24);
        }
        std::memcpy(ruido, "ACPI", 4);
        if (esPaqueteSemaforo(ruido, (int)sizeof(ruido))) {
            ++aceptados;
            estado_semaforo p;
            std::memcpy(&p, ruido, sizeof(p));
            VERIFICAR(p.grados < 360 && p.permitidoCruce <= 1);
        }
    }
    // Con CRC-8 pasa ~1 de cada 256, y de esos casi ninguno tiene campos en rango
    VERIFICAR(aceptados < 5);
}

PRUEBA(protocolo_campos_en_su_posicion) {
    // El orden de bytes en el aire debe ser el mismo en semáforo y brazalete
    estado_semaforo p{};
    firmarPaqueteSemaforo(&p);
    p.id = 7;
    p.grados = 0x0102;
    p.permitidoCruce = 1;
    p.tiempo = 25;
    sellarPaqueteSemaforo(&p);
    const uint8_t* b = (const uint8_t*)&p;
    VERIFICAR_IGUAL(b[4], 7);
    VERIFICAR_IGUAL(b[5], 0x02);  // little-endian, como el ESP32
    VERIFICAR_IGUAL(b[6], 0x01);
    VERIFICAR_IGUAL(b[7], 1);
    VERIFICAR_IGUAL(b[8], 25);
    VERIFICAR_IGUAL(b[9], crc8Semaforo(b, 9));
}
