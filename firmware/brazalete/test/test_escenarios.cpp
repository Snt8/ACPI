// Pruebas de escenario: simulan minutos de uso con la MISMA lógica que la FSM
// (CoordinadorCruce: paquetes, brújula a 2 Hz sólo sin vibrar, orquestar y
// actualizar) y verifican las garantías de patrones.md:
//   - sin evento no vibra;
//   - cada evento vibra una sola vez;
//   - entre inicios de patrones pasan al menos 3 s, nunca se solapan y tras
//     cada patrón los motores descansan al menos 1 s.
// Lo único simulado es el mundo exterior: cuándo llegan paquetes, qué dicen y
// qué lee la brújula. El standby se modela como "sin paso() hasta que vuelva un
// paquete" (en la placa, además, sólo se escucha 1.2 s cada 5 s).
#include "minitest.h"
#include "../src/control/coordinador_cruce.h"
#include "../src/constantes.h"
#include <functional>
#include <vector>

namespace {

struct Inicio {
    uint32_t t;
    Patron   patron;
};

class Simulador {
public:
    // Mundo exterior en el instante t
    std::function<bool(uint32_t)>        luz;        // permitidoCruce
    std::function<Orientacion(uint32_t)> lectura;    // lo que lee la brújula
    std::function<bool(uint32_t)>        presente = [](uint32_t) { return true; };  // llegan paquetes
    std::function<uint8_t(uint32_t)>     id       = [](uint32_t) { return (uint8_t)1; };

    CoordinadorCruce    cruce;
    std::vector<Inicio> inicios;
    std::vector<uint32_t> standby;   // instantes en que se pasó a standby
    uint32_t ms_encendido = 0;       // tiempo con algún motor encendido
    bool motor_sin_patron = false;   // motor encendido fuera de un patrón registrado
    bool motor_equivocado = false;   // un giro encendió el motor del otro lado
    bool en_standby = true;          // arranca en standby, como el brazalete

    void correr(uint32_t desde, uint32_t hasta) {
        for (uint32_t t = desde; t < hasta; t += PASO_MS) {
            ahora_ = t;
            const estado_semaforo* paquete = nullptr;
            estado_semaforo p{};
            if (presente(t) && (!hubo_envio_ || t - ultimo_envio_ >= INTERVALO_ENVIO_SEMAFORO_MS)) {
                firmarPaqueteSemaforo(&p);
                p.id = id(t);
                p.grados = 90;
                p.permitidoCruce = static_cast<uint8_t>(luz(t) ? 1 : 0);
                paquete = &p;
                ultimo_envio_ = t;
                hubo_envio_ = true;
            }

            if (en_standby) {
                if (paquete == nullptr) continue;  // en standby no pasa nada
                en_standby = false;
                cruce.entrar(t);
            }

            VueltaCruce v = cruce.paso(t, paquete, &Simulador::leer, this);
            if (v.iniciado != Patron::NINGUNO) inicios.push_back({t, v.iniciado});
            revisarMotores(t, v.salida);

            if (cruce.sinPaquetes(t)) {   // igual que fsm.cpp
                cruce.salir();
                en_standby = true;
                standby.push_back(t);
            }
        }
    }

    int contar(Patron patron) const {
        int n = 0;
        for (const Inicio& i : inicios) n += (i.patron == patron);
        return n;
    }

    int iniciosEntre(uint32_t desde, uint32_t hasta) const {
        int n = 0;
        for (const Inicio& i : inicios) n += (i.t >= desde && i.t < hasta);
        return n;
    }

    // Verifica las reglas de protección de las soldaduras sobre todo el registro
    void verificarSeparacion() const {
        for (size_t i = 1; i < inicios.size(); ++i) {
            uint32_t entre = inicios[i].t - inicios[i - 1].t;
            uint32_t dur   = ControladorPatrones::duracion(inicios[i - 1].patron);
            VERIFICAR(entre >= SEPARACION_MINIMA_EVENTOS_MS);
            VERIFICAR(entre >= dur + SILENCIO_TRAS_PATRON_MS);  // sin solaparse y con descanso
        }
        VERIFICAR(!motor_sin_patron);
        VERIFICAR(!motor_equivocado);
    }

    static const uint32_t PASO_MS = 10;

private:
    uint32_t ahora_        = 0;
    uint32_t ultimo_envio_ = 0;
    bool     hubo_envio_   = false;

    static Orientacion leer(const estado_semaforo&, void* contexto) {
        Simulador* s = static_cast<Simulador*>(contexto);
        return s->lectura(s->ahora_);
    }

    // Comprobación independiente del controlador: un motor sólo puede estar
    // encendido dentro de la duración del último patrón registrado, y un
    // pulso de giro sólo enciende el motor de su lado
    void revisarMotores(uint32_t t, SalidaMotores s) {
        if (!s.derecho && !s.izquierdo) return;
        ms_encendido += PASO_MS;
        if (inicios.empty() ||
            t - inicios.back().t >= ControladorPatrones::duracion(inicios.back().patron)) {
            motor_sin_patron = true;
            return;
        }
        Patron p = inicios.back().patron;
        if ((p == Patron::GIRAR_DERECHA && s.izquierdo) ||
            (p == Patron::GIRAR_IZQUIERDA && s.derecho)) {
            motor_equivocado = true;
        }
    }
};

Orientacion deFrente(uint32_t)   { return {true, 2.0f, 2.0f}; }
Orientacion malDerecha(uint32_t) { return {true, 90.0f, 90.0f}; }
bool siemprePermitido(uint32_t)  { return true; }
bool siempreProhibido(uint32_t)  { return false; }

// Generador pseudoaleatorio determinista (el resultado no cambia entre corridas)
struct Lcg {
    uint32_t estado;
    uint32_t siguiente() { estado = estado * 1664525u + 1013904223u; return estado >> 8; }
    float entre(float a, float b) { return a + (b - a) * (float)(siguiente() % 10000) / 10000.0f; }
};

} // namespace

PRUEBA(escenario_quieta_de_frente_solo_dos_vibraciones) {
    // Llega al semáforo ya de frente y se queda quieta 20 s con la misma luz:
    // "detectado" + aviso de luz, y después nada
    Simulador s;
    s.luz = siemprePermitido;
    s.lectura = deFrente;
    s.correr(0, 20000);
    VERIFICAR_IGUAL(s.inicios.size(), 2);
    VERIFICAR_IGUAL(s.contar(Patron::SEMAFORO_DETECTADO), 1);
    VERIFICAR_IGUAL(s.contar(Patron::CRUCE_PERMITIDO), 1);
    VERIFICAR_IGUAL(s.iniciosEntre(3001, 20000), 0);
    s.verificarSeparacion();
    // Encendido total: 3×200 ms + 2×2000 ms
    VERIFICAR_IGUAL(s.ms_encendido, 600 + 4000);
}

PRUEBA(escenario_cada_cambio_de_luz_vibra_una_vez) {
    // Ciclo real del semáforo: 25 s por luz. De frente durante 2 minutos.
    Simulador s;
    s.luz = [](uint32_t t) { return (t / 25000) % 2 == 1; };
    s.lectura = deFrente;
    s.correr(0, 120000);
    // Cada aviso llega con el segundo paquete de la luz nueva (500 ms después)
    for (const Inicio& i : s.inicios) {
        if (i.t >= 25000) VERIFICAR_IGUAL(i.t % 25000, INTERVALO_ENVIO_SEMAFORO_MS);
    }
    // Cambios en 25, 50, 75 y 100 s
    VERIFICAR_IGUAL(s.contar(Patron::SEMAFORO_DETECTADO), 1);
    VERIFICAR_IGUAL(s.contar(Patron::CRUCE_PROHIBIDO), 3);  // inicial + 50 s + 100 s
    VERIFICAR_IGUAL(s.contar(Patron::CRUCE_PERMITIDO), 2);  // 25 s + 75 s
    VERIFICAR_IGUAL(s.inicios.size(), 6);
    s.verificarSeparacion();
}

PRUEBA(escenario_mal_orientada_pulso_exacto_cada_tres_segundos) {
    Simulador s;
    s.luz = siemprePermitido;
    s.lectura = malDerecha;
    s.correr(0, 30000);
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
    s.luz = siempreProhibido;
    s.lectura = [](uint32_t t) {
        return t < 10000 ? Orientacion{true, 90.0f, -90.0f} : Orientacion{true, 3.0f, 3.0f};
    };
    s.correr(0, 24000);
    VERIFICAR_IGUAL(s.contar(Patron::GIRAR_IZQUIERDA), 3);  // 3, 6 y 9 s
    VERIFICAR_IGUAL(s.contar(Patron::CRUCE_PROHIBIDO), 1);
    VERIFICAR_IGUAL(s.inicios.back().patron, Patron::CRUCE_PROHIBIDO);
    VERIFICAR(s.inicios.back().t < 13000);
    s.verificarSeparacion();
}

PRUEBA(escenario_ruido_en_el_umbral_no_repite_avisos) {
    // Ya de frente, la lectura oscila entre 3° y 9.5° (a ambos lados del umbral
    // de 5°, dentro de la histéresis): sigue "de frente" y la luz se avisa una
    // sola vez
    Simulador s;
    s.luz = siemprePermitido;
    s.lectura = [](uint32_t t) {
        float d = (t < 3000 || (t / 500) % 2 == 0) ? 3.0f : 9.5f;
        return Orientacion{true, d, d};
    };
    s.correr(0, 24000);
    VERIFICAR_IGUAL(s.contar(Patron::CRUCE_PERMITIDO), 1);
    VERIFICAR_IGUAL(s.contar(Patron::GIRAR_DERECHA) + s.contar(Patron::GIRAR_IZQUIERDA), 0);
    s.verificarSeparacion();
}

PRUEBA(escenario_cambio_de_luz_durante_el_verde_espera_y_descansa) {
    // El aviso de verde (4.5 s) está sonando cuando la luz pasa a rojo: el rojo
    // espera a que termine y a que los motores descansen 1 s
    Simulador s;
    s.luz = [](uint32_t t) { return t < 4000; };
    s.lectura = deFrente;
    s.correr(0, 20000);
    VERIFICAR_IGUAL(s.inicios.size(), 3);
    VERIFICAR_IGUAL(s.inicios[1].patron, Patron::CRUCE_PERMITIDO);
    VERIFICAR_IGUAL(s.inicios[1].t, 3000);
    VERIFICAR_IGUAL(s.inicios[2].patron, Patron::CRUCE_PROHIBIDO);
    VERIFICAR_IGUAL(s.inicios[2].t, 3000 + 4500 + SILENCIO_TRAS_PATRON_MS);
    s.verificarSeparacion();
}

PRUEBA(escenario_sin_paquetes_pasa_a_standby_y_no_vibra) {
    // El semáforo deja de transmitir a los 10 s. A los 30 s del último paquete
    // el brazalete pasa a standby; mientras tanto y después, sin eventos, no
    // vibra. Cuando el semáforo vuelve, avisa "detectado" otra vez.
    Simulador s;
    s.luz = siemprePermitido;
    s.lectura = deFrente;
    s.presente = [](uint32_t t) { return t < 10000 || t >= 60000; };
    s.correr(0, 70000);
    VERIFICAR_IGUAL(s.standby.size(), 1);
    VERIFICAR_IGUAL(s.standby[0], 9500 + TIEMPO_SIN_PAQUETES_STANDBY_MS);  // último paquete: 9.5 s
    VERIFICAR_IGUAL(s.iniciosEntre(3001, 60000), 0);
    VERIFICAR_IGUAL(s.contar(Patron::SEMAFORO_DETECTADO), 2);
    VERIFICAR_IGUAL(s.contar(Patron::CRUCE_PERMITIDO), 2);  // uno por cada detección
    s.verificarSeparacion();
}

PRUEBA(escenario_otro_semaforo_no_mantiene_despierto) {
    // A los 5 s sólo se oye otro semáforo (id 2): no cuenta como paquete del
    // semáforo seguido, así que a los 30 s se pasa a standby, y al salir se
    // detecta el nuevo semáforo como uno nuevo
    Simulador s;
    s.luz = siemprePermitido;
    s.lectura = deFrente;
    s.id = [](uint32_t t) { return (uint8_t)(t < 5000 ? 1 : 2); };
    s.correr(0, 40000);
    VERIFICAR_IGUAL(s.standby.size(), 1);
    VERIFICAR_IGUAL(s.standby[0], 4500 + TIEMPO_SIN_PAQUETES_STANDBY_MS);
    VERIFICAR_IGUAL(s.iniciosEntre(3001, s.standby[0]), 0);  // los paquetes ignorados no hacen vibrar
    VERIFICAR_IGUAL(s.contar(Patron::SEMAFORO_DETECTADO), 2);
    VERIFICAR_IGUAL(s.cruce.evaluador().ultimoPaquete().id, 2);
    s.verificarSeparacion();
}

PRUEBA(escenario_paquetes_sueltos_con_otra_luz_no_hacen_vibrar) {
    // Uno de cada 7 paquetes llega con la luz contraria (interferencia u otro
    // equipo), nunca dos seguidos: la persona está de frente y no debe recibir
    // ningún aviso más allá de "detectado" y la luz real
    Simulador s;
    s.lectura = deFrente;
    int envio = 0;
    s.luz = [&](uint32_t) { return (envio++ % 7) != 3; };  // la luz real es "permitido"
    s.correr(0, 60000);
    VERIFICAR_IGUAL(s.inicios.size(), 2);
    VERIFICAR_IGUAL(s.contar(Patron::CRUCE_PERMITIDO), 1);
    VERIFICAR_IGUAL(s.contar(Patron::CRUCE_PROHIBIDO), 0);
    s.verificarSeparacion();
}

PRUEBA(escenario_aleatorio_media_hora_respeta_las_garantias) {
    // Media hora con orientación, luz, lecturas inválidas y cortes de señal al
    // azar (semilla fija). Pase lo que pase, nunca dos inicios a menos de 3 s,
    // nunca sin descanso, ni motores encendidos fuera de un patrón.
    Simulador s;
    Lcg azar{12345};
    float desviacion = 90.0f, giro = 90.0f;
    bool invalida = false, permitido = false, presente = true;
    uint32_t cambio_lectura = 0, cambio_luz = 0, cambio_senal = 0;

    s.luz = [&](uint32_t t) {
        if (t >= cambio_luz) {
            permitido = !permitido;
            cambio_luz = t + 1000 + azar.siguiente() % 30000;
        }
        return permitido;
    };
    s.lectura = [&](uint32_t t) {
        if (t >= cambio_lectura) {
            desviacion = azar.entre(0.0f, 180.0f);
            giro = azar.entre(-180.0f, 180.0f);
            invalida = (azar.siguiente() % 8) == 0;
            cambio_lectura = t + azar.siguiente() % 4000;
        }
        return Orientacion{!invalida, desviacion, giro};
    };
    s.presente = [&](uint32_t t) {
        if (t >= cambio_senal) {
            presente = (azar.siguiente() % 5) != 0;   // 1 de cada 5 tramos sin señal
            cambio_senal = t + 5000 + azar.siguiente() % 60000;
        }
        return presente;
    };
    s.correr(0, 30u * 60u * 1000u);
    VERIFICAR(s.inicios.size() > 50);   // hubo actividad de verdad
    VERIFICAR(!s.standby.empty());      // y algún paso por standby
    s.verificarSeparacion();
    // Cota empírica de esfuerzo de los motores: con estas entradas queda muy
    // por debajo de la mitad del tiempo
    VERIFICAR(s.ms_encendido < 30u * 60u * 1000u / 2);
}

PRUEBA(escenario_prueba_de_vibracion_desde_la_app) {
    // TEST_ALERT suena con el patrón de "detectado" y respeta las mismas reglas:
    // pegado a otro patrón no suena; VIBRATE_STOP corta lo que esté sonando
    CoordinadorCruce cruce;
    VERIFICAR(cruce.probarVibracion(1000));
    VERIFICAR_IGUAL(cruce.patrones().patronActual(), Patron::SEMAFORO_DETECTADO);
    VERIFICAR(!cruce.probarVibracion(2000));             // menos de 3 s después
    cruce.detenerVibracion();
    VERIFICAR(!cruce.patrones().sonando());
    VERIFICAR(!cruce.probarVibracion(3000));             // cortar no adelanta los 3 s
    VERIFICAR(cruce.probarVibracion(1000 + ControladorPatrones::duracion(Patron::SEMAFORO_DETECTADO) +
                                    SEPARACION_MINIMA_EVENTOS_MS));
}
