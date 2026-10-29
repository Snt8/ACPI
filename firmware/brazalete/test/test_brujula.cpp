// Pruebas de la matemática de la brújula: rumbo con tilt compensation
// (rumboCompensado) y calibración hard-iron (CalibracionMagnetica). Generan el
// campo que mediría el sensor en una pose conocida y comparan con el rumbo real.
#include "minitest.h"
#include "../src/brujula/calibracion_magnetica.h"
#include "../src/brujula/ejes_acelerometro.h"
#include "../src/utils/calcular_direccion.h"
#include "../src/constantes.h"
#include <math.h>

namespace {

struct V3 { float x, y, z; };
float dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

// Campo de Colombia en el marco Norte-Oeste-Arriba: 0.30 G horizontal hacia el
// norte magnético y 0.15 G hacia abajo, en LSB del QMC5883L a ±8 G (3000 LSB/G)
const V3 CAMPO  = {900.0f, 0.0f, -450.0f};
const V3 ARRIBA = {0.0f, 0.0f, 1.0f};

// Ejes del sensor (x adelante, y izquierda, z arriba) en el marco del mundo
// para un rumbo horario desde el norte, cabeceo y alabeo en grados
struct Ejes { V3 f, l, u; };
Ejes ejes(float rumbo, float pitch, float roll) {
    float y = rumbo * GRADOS_A_RAD, t = pitch * GRADOS_A_RAD, r = roll * GRADOS_A_RAD;
    V3 f0 = {cosf(y), -sinf(y), 0}, l0 = {sinf(y), cosf(y), 0}, u0 = {0, 0, 1};
    V3 f1 = {f0.x * cosf(t) + u0.x * sinf(t), f0.y * cosf(t) + u0.y * sinf(t), f0.z * cosf(t) + u0.z * sinf(t)};
    V3 u1 = {u0.x * cosf(t) - f0.x * sinf(t), u0.y * cosf(t) - f0.y * sinf(t), u0.z * cosf(t) - f0.z * sinf(t)};
    V3 l2 = {l0.x * cosf(r) + u1.x * sinf(r), l0.y * cosf(r) + u1.y * sinf(r), l0.z * cosf(r) + u1.z * sinf(r)};
    V3 u2 = {u1.x * cosf(r) - l0.x * sinf(r), u1.y * cosf(r) - l0.y * sinf(r), u1.z * cosf(r) - l0.z * sinf(r)};
    return {f1, l2, u2};
}
V3 enSensor(const Ejes& e, V3 v) { return {dot(v, e.f), dot(v, e.l), dot(v, e.u)}; }

float errorAngular(float medido, float real) {
    float d = fmodf(medido - real + 540.0f, 360.0f) - 180.0f;
    return fabsf(d);
}

} // namespace

PRUEBA(brujula_rumbo_plano) {
    for (int r = 0; r < 360; r += 15) {
        Ejes e = ejes((float)r, 0, 0);
        V3 m = enSensor(e, CAMPO), g = enSensor(e, ARRIBA);
        VERIFICAR(errorAngular(rumboCompensado(m.x, m.y, m.z, g.x, g.y, g.z), (float)r) < 0.05f);
    }
}

PRUEBA(brujula_rumbo_este_es_noventa) {
    // Sentido horario: mirando al este, el norte queda a la izquierda (+y)
    VERIFICAR(errorAngular(rumboCompensado(0, 900, -450, 0, 0, 1), 90.0f) < 0.01f);
    VERIFICAR(errorAngular(rumboCompensado(0, -900, -450, 0, 0, 1), 270.0f) < 0.01f);
}

PRUEBA(brujula_tilt_compensation_con_la_muneca_inclinada) {
    // La fórmula anterior fallaba 4° a 30° de cabeceo y 16° a 30°/30°
    const float inclinaciones[][2] = {{30, 0}, {0, 30}, {30, 30}, {-20, 45}, {45, -30}, {60, 0}};
    for (const auto& inc : inclinaciones) {
        for (int r = 0; r < 360; r += 10) {
            Ejes e = ejes((float)r, inc[0], inc[1]);
            V3 m = enSensor(e, CAMPO), g = enSensor(e, ARRIBA);
            VERIFICAR(errorAngular(rumboCompensado(m.x, m.y, m.z, g.x, g.y, g.z), (float)r) < 0.1f);
        }
    }
}

PRUEBA(brujula_gravedad_sin_normalizar) {
    // El módulo de g no importa (el acelerómetro nunca mide exactamente 1 g)
    Ejes e = ejes(123.0f, 20, -25);
    V3 m = enSensor(e, CAMPO), g = enSensor(e, ARRIBA);
    float a = rumboCompensado(m.x, m.y, m.z, g.x, g.y, g.z);
    float b = rumboCompensado(m.x, m.y, m.z, g.x * 0.9f, g.y * 0.9f, g.z * 0.9f);
    VERIFICAR(errorAngular(a, 123.0f) < 0.1f);
    VERIFICAR(errorAngular(a, b) < 0.01f);
}

PRUEBA(brujula_rumbo_siempre_en_rango) {
    for (int i = -50; i <= 50; i++) {
        float h = rumboCompensado(900.0f, (float)i * 1e-6f, 0, 0, 0, 1);
        VERIFICAR(h >= 0.0f && h < 360.0f);
    }
}

// ── Calibración ──────────────────────────────────────────────────────────────

namespace {

// Gira el sensor en todas direcciones con un campo fijo propio (hard-iron) y
// alimenta la calibración
void calibrarConGiros(CalibracionMagnetica& cal, V3 hard_iron) {
    for (int r = 0; r < 360; r += 10) {
        for (int p = -90; p <= 90; p += 15) {
            for (int roll = 0; roll < 360; roll += 30) {
                V3 m = enSensor(ejes((float)r, (float)p, (float)roll), CAMPO);
                float crudo[3] = {m.x + hard_iron.x, m.y + hard_iron.y, m.z + hard_iron.z};
                cal.agregar(crudo);
            }
        }
    }
}

} // namespace

PRUEBA(calibracion_sin_datos_no_esta_lista) {
    CalibracionMagnetica cal(CALIBRACION_SPAN_MINIMO_LSB, true);
    VERIFICAR(!cal.lista());
    float crudo[3] = {100, 200, 300}, out[3];
    cal.corregir(crudo, out);
    VERIFICAR_IGUAL(out[0], 100.0f);  // sin calibración devuelve lo crudo
}

PRUEBA(calibracion_quita_el_campo_de_los_motores) {
    // Imán de los motores: desplaza el campo ~600 LSB, más de la mitad del de la Tierra
    const V3 hard_iron = {450, -300, 200};
    CalibracionMagnetica cal(CALIBRACION_SPAN_MINIMO_LSB, true);
    calibrarConGiros(cal, hard_iron);
    VERIFICAR(cal.lista());

    float peor = 0, peor_sin = 0;
    for (int r = 0; r < 360; r += 5) {
        for (int p = -30; p <= 30; p += 30) {
            Ejes e = ejes((float)r, (float)p, 20.0f);
            V3 m = enSensor(e, CAMPO), g = enSensor(e, ARRIBA);
            float crudo[3] = {m.x + hard_iron.x, m.y + hard_iron.y, m.z + hard_iron.z};
            float c[3];
            cal.corregir(crudo, c);
            float e1 = errorAngular(rumboCompensado(c[0], c[1], c[2], g.x, g.y, g.z), (float)r);
            float e2 = errorAngular(rumboCompensado(crudo[0], crudo[1], crudo[2], g.x, g.y, g.z), (float)r);
            if (e1 > peor) peor = e1;
            if (e2 > peor_sin) peor_sin = e2;
        }
    }
    VERIFICAR(peor < 1.0f);       // con calibración: menos de 1°
    VERIFICAR(peor_sin > 20.0f);  // sin ella el error era enorme
}

PRUEBA(calibracion_un_pico_suelto_no_la_desplaza) {
    const V3 cero = {0, 0, 0};
    CalibracionMagnetica cal(CALIBRACION_SPAN_MINIMO_LSB, true);
    calibrarConGiros(cal, cero);
    DatosCalibracion antes = cal.datos();
    float pico[3] = {20000, -20000, 20000};
    float normal[3] = {100, 100, 100};
    cal.agregar(normal);
    cal.agregar(pico);      // la mediana de tres lo descarta
    cal.agregar(normal);
    for (int i = 0; i < 3; i++) {
        VERIFICAR_IGUAL(cal.datos().span[i], antes.span[i]);
        VERIFICAR(fabsf(cal.datos().centro[i] - antes.centro[i]) < 5.0f);
    }
}

PRUEBA(calibracion_girar_solo_en_el_plano_no_basta_en_el_brazalete) {
    // El brazalete exige recorrer también Z (hay que voltearlo); el semáforo no
    CalibracionMagnetica brazalete(CALIBRACION_SPAN_MINIMO_LSB, true);
    CalibracionMagnetica semaforo(CALIBRACION_SPAN_MINIMO_LSB, false);
    for (int r = 0; r < 720; r += 5) {
        V3 m = enSensor(ejes((float)r, 0, 0), CAMPO);
        float crudo[3] = {m.x, m.y, m.z};
        brazalete.agregar(crudo);
        semaforo.agregar(crudo);
    }
    VERIFICAR(!brazalete.lista());
    VERIFICAR(semaforo.lista());
}

PRUEBA(calibracion_cargar_rechaza_datos_incompletos) {
    CalibracionMagnetica cal(CALIBRACION_SPAN_MINIMO_LSB, true);
    DatosCalibracion corta = {{0, -100, 100}, {200, 200, 200}};
    VERIFICAR(!cal.cargar(corta));
    VERIFICAR(!cal.lista());
    DatosCalibracion buena = {{0, -100, 100}, {2000, 2000, 2000}};
    VERIFICAR(cal.cargar(buena));
    VERIFICAR(cal.lista());
    float crudo[3] = {0, -100, 100}, c[3];
    cal.corregir(crudo, c);
    VERIFICAR(fabsf(c[0]) < 0.01f && fabsf(c[1]) < 0.01f && fabsf(c[2]) < 0.01f);
}

PRUEBA(calibracion_con_giro_incompleto_sigue_precisa) {
    // Girándolo a mano ningún eje llega justo a su extremo: 40 s de un "8"
    // continuo a 20 muestras/s con el cabeceo hasta ±60° y el alabeo hasta
    // ±150°. El ajuste de esfera usa todas las muestras y el centro sigue
    // saliendo bien.
    const V3 hard_iron = {450, -300, 200};
    CalibracionMagnetica cal(CALIBRACION_SPAN_MINIMO_LSB, true);
    for (int i = 0; i < 800; i++) {
        float t = i * 0.05f;
        Ejes e = ejes(fmodf(t * 40.0f, 360.0f), 60.0f * sinf(t * 0.7f), 150.0f * sinf(t * 0.45f));
        V3 m = enSensor(e, CAMPO);
        float crudo[3] = {m.x + hard_iron.x, m.y + hard_iron.y, m.z + hard_iron.z};
        cal.agregar(crudo);
    }
    VERIFICAR(cal.lista());
    VERIFICAR(fabsf(cal.datos().centro[0] - hard_iron.x) < 2.0f);
    VERIFICAR(fabsf(cal.datos().centro[1] - hard_iron.y) < 2.0f);
    VERIFICAR(fabsf(cal.datos().centro[2] - hard_iron.z) < 2.0f);
}

PRUEBA(calibracion_del_semaforo_en_el_plano) {
    CalibracionMagnetica cal(400.0f, false);
    const V3 hard_iron = {40, -25, 10};
    for (int r = 0; r < 400; r += 3) {
        V3 m = enSensor(ejes((float)r, 0, 0), CAMPO);
        float crudo[3] = {m.x / 3 + hard_iron.x, m.y / 3 + hard_iron.y, m.z / 3 + hard_iron.z};
        cal.agregar(crudo);   // ~1000 LSB/G: un tercio de lo del brazalete
    }
    VERIFICAR(cal.lista());
    VERIFICAR(fabsf(cal.datos().centro[0] - hard_iron.x) < 1.0f);
    VERIFICAR(fabsf(cal.datos().centro[1] - hard_iron.y) < 1.0f);
}

// ── Orientación del acelerómetro ─────────────────────────────────────────────


PRUEBA(ejes_detecta_como_esta_montado_el_acelerometro) {
    for (int montaje = 0; montaje < NUM_ORIENTACIONES_ACEL; ++montaje) {
        // Orientación inversa: lo que mide un acelerómetro montado así
        int inversa = -1;
        for (int k = 0; k < NUM_ORIENTACIONES_ACEL && inversa < 0; ++k) {
            float v[3] = {0.1f, 0.2f, 0.97f};
            aplicarOrientacionAcel(k, v);
            aplicarOrientacionAcel(montaje, v);
            if (fabsf(v[0] - 0.1f) < 1e-5f && fabsf(v[1] - 0.2f) < 1e-5f && fabsf(v[2] - 0.97f) < 1e-5f) inversa = k;
        }
        VERIFICAR(inversa >= 0);

        // 30 s de calibración a mano (un "8"), con algo de aceleración de la mano
        static float g[600][3], m[600][3];
        uint32_t azar = 12345u;
        for (int i = 0; i < 600; ++i) {
            float t = i * 0.05f;
            Ejes e = ejes(fmodf(t * 40.0f, 360.0f), 70.0f * sinf(t * 0.7f), 160.0f * sinf(t * 0.45f));
            V3 mm = enSensor(e, CAMPO), gg = enSensor(e, ARRIBA);
            azar = azar * 1664525u + 1013904223u;
            float ruido = ((float)(azar >> 8) / 16777216.0f - 0.5f) * 0.1f;
            float leido[3] = {gg.x + ruido, gg.y - ruido, gg.z + ruido / 2};
            aplicarOrientacionAcel(inversa, leido);
            for (int k = 0; k < 3; ++k) g[i][k] = leido[k];
            m[i][0] = mm.x; m[i][1] = mm.y; m[i][2] = mm.z;
        }
        float disp[NUM_ORIENTACIONES_ACEL];
        VERIFICAR_IGUAL(detectarOrientacionAcel(g, m, 600, disp), montaje);
    }
}

PRUEBA(ejes_sin_muestras_quietas_no_decide) {
    float g[10][3] = {}, m[10][3] = {};
    for (int i = 0; i < 10; ++i) { g[i][2] = 1.0f; m[i][0] = 900.0f; }
    VERIFICAR_IGUAL(detectarOrientacionAcel(g, m, 10, nullptr), -1);
}
