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

PRUEBA(protocolo_tamano_y_firma) {
    VERIFICAR_IGUAL(sizeof(estado_semaforo), 9);
    estado_semaforo p{};
    firmarPaqueteSemaforo(&p);
    VERIFICAR(std::memcmp(p.firma, "ACPI", 4) == 0);
    VERIFICAR(esPaqueteSemaforo((const uint8_t*)&p, (int)sizeof(p)));
}

PRUEBA(protocolo_rechaza_paquetes_ajenos) {
    estado_semaforo p{};
    firmarPaqueteSemaforo(&p);
    const uint8_t* bytes = (const uint8_t*)&p;
    VERIFICAR(!esPaqueteSemaforo(nullptr, (int)sizeof(p)));
    VERIFICAR(!esPaqueteSemaforo(bytes, (int)sizeof(p) - 1));  // formato viejo / truncado
    VERIFICAR(!esPaqueteSemaforo(bytes, (int)sizeof(p) + 1));
    VERIFICAR(!esPaqueteSemaforo(bytes, 0));
    p.firma[3] = 'X';
    VERIFICAR(!esPaqueteSemaforo(bytes, (int)sizeof(p)));
    // El paquete viejo de 5 bytes (sin firma) no se acepta
    uint8_t viejo[5] = {1, 90, 0, 1, 10};
    VERIFICAR(!esPaqueteSemaforo(viejo, 5));
}

PRUEBA(protocolo_campos_en_su_posicion) {
    // El orden de bytes en el aire debe ser el mismo en semáforo y brazalete
    estado_semaforo p{};
    firmarPaqueteSemaforo(&p);
    p.id = 7;
    p.grados = 0x0102;
    p.permitidoCruce = 1;
    p.tiempo = 25;
    const uint8_t* b = (const uint8_t*)&p;
    VERIFICAR_IGUAL(b[4], 7);
    VERIFICAR_IGUAL(b[5], 0x02);  // little-endian, como el ESP32
    VERIFICAR_IGUAL(b[6], 0x01);
    VERIFICAR_IGUAL(b[7], 1);
    VERIFICAR_IGUAL(b[8], 25);
}
