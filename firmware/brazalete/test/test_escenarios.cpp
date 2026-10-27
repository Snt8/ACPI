// Pruebas de escenario: simulan minutos de uso con el mismo cableado que la FSM
// (paquetes cada 500 ms, brújula a 2 Hz sólo cuando no vibra, orquestar() y
// actualizar() en cada vuelta) y verifican las garantías de patrones.md:
//   - sin evento no vibra;
//   - cada evento vibra una sola vez;
//   - entre inicios de patrones pasan al menos 3 s y nunca se solapan.
#include "minitest.h"
#include "../src/control/evaluador_semaforo.h"
#include "../src/constantes.h"
#include <vector>

namespace {

struct Inicio {
    uint32_t t;
    Patron   patron;
};

// Reproduce el estado SENSING_CROSSING de fsm.cpp
class Simulador {
public:
    ControladorPatrones patrones;
    EvaluadorSemaforo   evaluador{patrones};
    std::vector<Inicio> inicios;
    uint32_t ms_encendido = 0;      // tiempo con algún motor encendido
    bool motor_sin_patron = false;  // un motor encendido sin patrón sonando

    // Avanza la simulación de 'desde' a 'hasta' en pasos de 10 ms.
    // luz(t): permitidoCruce en el instante t; orientacion(t): lectura de brújula.
    template <typename Luz, typename Lectura>
    void correr(uint32_t desde, uint32_t hasta, Luz luz, Lectura orientacion) {
        for (uint32_t t = desde; t < hasta; t += PASO_MS) {
            if (t - ultimo_paquete_ >= 500 || !hubo_paquete_) {
                estado_semaforo p{};
                firmarPaqueteSemaforo(&p);
                p.id = 1;
                p.grados = 90;
                p.permitidoCruce = luz(t) ? 1 : 0;
                evaluador.registrarPaquete(p);
                ultimo_paquete_ = t;
                hubo_paquete_ = true;
            }
            if (t - ultimo_muestreo_ >= INTERVALO_MUESTREO_MS && !patrones.sonando()) {
                ultimo_muestreo_ = t;
                evaluador.registrarOrientacion(orientacion(t));
            }
            Patron p = evaluador.orquestar(t);
            if (p != Patron::NINGUNO) inicios.push_back({t, p});

            SalidaMotores s = patrones.actualizar(t);
            if (s.derecho || s.izquierdo) {
                ms_encendido += PASO_MS;
                if (!patrones.sonando()) motor_sin_patron = true;
            }
        }
    }

    int contar(Patron patron) const {
        int n = 0;
        for (const Inicio& i : inicios) n += (i.patron == patron);
        return n;
    }

    // Verifica la regla de protección de las soldaduras sobre todo el registro
    void verificarSeparacion() const {
        for (size_t i = 1; i < inicios.size(); ++i) {
            uint32_t entre = inicios[i].t - inicios[i - 1].t;
            VERIFICAR(entre >= SEPARACION_MINIMA_EVENTOS_MS);
            VERIFICAR(entre >= ControladorPatrones::duracion(inicios[i - 1].patron));  // sin solaparse
        }
        VERIFICAR(!motor_sin_patron);
    }

    static const uint32_t PASO_MS = 10;

private:
    uint32_t ultimo_paquete_  = 0;
    bool     hubo_paquete_    = false;
    uint32_t ultimo_muestreo_ = 0;
};

Orientacion deFrente(uint32_t)  { return {true, 5.0f, 5.0f}; }
Orientacion malDerecha(uint32_t) { return {true, 90.0f, 90.0f}; }
bool siemprePermitido(uint32_t) { return true; }

// Generador pseudoaleatorio determinista (el resultado no cambia entre corridas)
struct Lcg {
    uint32_t estado;
    uint32_t siguiente() { estado = estado * 1664525u + 1013904223u; return estado >> 8; }
    float entre(float a, float b) { return a + (b - a) * (siguiente() % 10000) / 10000.0f; }
};

} // namespace

PRUEBA(escenario_quieta_de_frente_solo_dos_vibraciones) {
    // Llega al semáforo ya de frente y se queda quieta 20 s con la misma luz:
    // "detectado" + aviso de luz, y después nada
    Simulador s;
    s.correr(0, 20000, siemprePermitido, deFrente);
    VERIFICAR_IGUAL(s.inicios.size(), 2);
    VERIFICAR_IGUAL(s.contar(Patron::SEMAFORO_DETECTADO), 1);
    VERIFICAR_IGUAL(s.contar(Patron::CRUCE_PERMITIDO), 1);
    s.verificarSeparacion();
    // Encendido total: 3×200 ms + 2×2000 ms
    VERIFICAR_IGUAL(s.ms_encendido, 600 + 4000);
}

PRUEBA(escenario_cada_cambio_de_luz_vibra_una_vez) {
    // Ciclo real del semáforo: 25 s por luz. De frente durante 2 minutos.
    Simulador s;
    auto luz = [](uint32_t t) { return (t / 25000) % 2 == 1; };
    s.correr(0, 120000, luz, deFrente);
    // Cambios en 25, 50, 75 y 100 s
    VERIFICAR_IGUAL(s.contar(Patron::SEMAFORO_DETECTADO), 1);
    VERIFICAR_IGUAL(s.contar(Patron::CRUCE_PROHIBIDO), 3);  // inicial + 50 s + 100 s
    VERIFICAR_IGUAL(s.contar(Patron::CRUCE_PERMITIDO), 2);  // 25 s + 75 s
    VERIFICAR_IGUAL(s.inicios.size(), 6);
    s.verificarSeparacion();
}

PRUEBA(escenario_mal_orientada_pulso_exacto_cada_tres_segundos) {
    Simulador s;
    s.correr(0, 30000, siemprePermitido, malDerecha);
    VERIFICAR_IGUAL(s.contar(Patron::SEMAFORO_DETECTADO), 1);
    int giros = 0;
    uint32_t anterior = 0;
    for (const Inicio& i : s.inicios) {
        if (i.patron != Patron::GIRAR_DERECHA) continue;
        if (giros > 0) VERIFICAR_IGUAL(i.t - anterior, INTERVALO_GIRO_MS);
        anterior = i.t;
        ++giros;
    }
    // Primer pulso a los 3 s (tras el aviso de detección) y luego cada 3 s hasta 27 s
    VERIFICAR_IGUAL(giros, 9);
    VERIFICAR_IGUAL(s.contar(Patron::GIRAR_IZQUIERDA), 0);
    VERIFICAR_IGUAL(s.contar(Patron::CRUCE_PERMITIDO), 0);  // nunca quedó de frente
    s.verificarSeparacion();
}

PRUEBA(escenario_se_orienta_y_luego_silencio) {
    // Mal orientada 10 s, gira y queda de frente: pulsos, un aviso de luz y silencio
    Simulador s;
    auto lectura = [](uint32_t t) {
        return t < 10000 ? Orientacion{true, 90.0f, -90.0f} : Orientacion{true, 3.0f, 3.0f};
    };
    s.correr(0, 24000, [](uint32_t) { return false; }, lectura);
    VERIFICAR_IGUAL(s.contar(Patron::GIRAR_IZQUIERDA), 3);  // 3, 6 y 9 s
    VERIFICAR_IGUAL(s.contar(Patron::CRUCE_PROHIBIDO), 1);
    VERIFICAR_IGUAL(s.inicios.back().patron, Patron::CRUCE_PROHIBIDO);
    VERIFICAR(s.inicios.back().t < 13000);
    s.verificarSeparacion();
}

PRUEBA(escenario_ruido_en_el_umbral_no_repite_avisos) {
    // La lectura oscila entre 25° y 38° (alrededor del umbral de 30°): con la
    // histéresis queda "de frente" y la luz se avisa una sola vez
    Simulador s;
    auto lectura = [](uint32_t t) {
        float d = ((t / 500) % 2 == 0) ? 25.0f : 38.0f;
        return Orientacion{true, d, d};
    };
    s.correr(0, 24000, siemprePermitido, lectura);
    VERIFICAR_IGUAL(s.contar(Patron::CRUCE_PERMITIDO), 1);
    VERIFICAR_IGUAL(s.contar(Patron::GIRAR_DERECHA) + s.contar(Patron::GIRAR_IZQUIERDA), 0);
    s.verificarSeparacion();
}

PRUEBA(escenario_aleatorio_media_hora_respeta_las_garantias) {
    // Media hora con orientación, luz y lecturas inválidas al azar (semilla fija).
    // Pase lo que pase, nunca dos inicios a menos de 3 s ni motores sin patrón.
    Simulador s;
    Lcg azar{12345};
    float desviacion = 90.0f, giro = 90.0f;
    bool invalida = false;
    uint32_t proximo_cambio = 0;
    bool permitido = false;
    uint32_t proximo_cambio_luz = 0;

    auto luz = [&](uint32_t t) {
        if (t >= proximo_cambio_luz) {
            permitido = !permitido;
            proximo_cambio_luz = t + 1000 + azar.siguiente() % 30000;
        }
        return permitido;
    };
    auto lectura = [&](uint32_t t) {
        if (t >= proximo_cambio) {
            desviacion = azar.entre(0.0f, 180.0f);
            giro = azar.entre(-180.0f, 180.0f);
            invalida = (azar.siguiente() % 8) == 0;
            proximo_cambio = t + azar.siguiente() % 4000;
        }
        return Orientacion{!invalida, desviacion, giro};
    };
    s.correr(0, 30u * 60u * 1000u, luz, lectura);
    VERIFICAR(s.inicios.size() > 50);  // hubo actividad de verdad
    s.verificarSeparacion();
    // Cota de esfuerzo: con 3 s de separación el peor caso es un patrón de
    // 4 s encendido cada 4.5 s; en la práctica debe quedar muy por debajo
    VERIFICAR(s.ms_encendido < 30u * 60u * 1000u / 2);
}
