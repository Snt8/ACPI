import * as Location from 'expo-location';
import { useSQLiteContext, type SQLiteDatabase } from 'expo-sqlite';
import { useEffect } from 'react';
import { AppState } from 'react-native';

import { registrar, useBrazalete } from '@/brazalete/estado';
import { distanciaMetros } from '@/semaforos/geo';
import { crearSemaforo, listarSemaforos } from '@/semaforos/repositorio';
import { sugerirNombre } from '@/semaforos/ubicacion';
import { alEncolar, contarPendientes, encolar, idAleatorio, vaciarCola } from '@/telemetria/cola';
import { configServidor, despertarServidor } from '@/telemetria/servidor';
import {
  conPaquete,
  conPosicion,
  nuevaSesion,
  PRECISION_MAXIMA_M,
  resumir,
  type ResumenCruce,
  type SesionCruce,
} from '@/telemetria/sesion';

// Dos detecciones a menos de esto son el mismo semáforo (el error típico del GPS en la
// ciudad es de 5 a 15 m)
const RADIO_MISMO_SEMAFORO_M = 25;
const INTERVALO_ENVIO_MS = 60_000;

/**
 * Semáforo guardado donde se detectó el cruce, o uno nuevo si no había ninguno cerca.
 * null si el GPS no dio una ubicación confiable.
 */
async function semaforoDelCruce(db: SQLiteDatabase, resumen: ResumenCruce): Promise<number | null> {
  if (resumen.lat === null || resumen.lon === null || (resumen.precision_m ?? Infinity) > PRECISION_MAXIMA_M) {
    return null;
  }
  const punto = { latitud: resumen.lat, longitud: resumen.lon };
  const cercanos = (await listarSemaforos(db))
    .map((s) => ({ id: s.id, distancia: distanciaMetros(punto, s) }))
    .filter((s) => s.distancia <= RADIO_MISMO_SEMAFORO_M)
    .sort((a, b) => a.distancia - b.distancia);
  if (cercanos.length > 0) {
    return cercanos[0].id;
  }
  const nombre = await sugerirNombre(punto);
  registrar(`Semáforo nuevo guardado: ${nombre}`);
  return crearSemaforo(
    db,
    { ...punto, nombre, notas: 'Detectado por el brazalete.', precisionMetros: resumen.precision_m },
    'brazalete',
  );
}

/**
 * Registra cada cruce (sesión de conexión con el brazalete, ver telemetria/sesion.ts) y
 * envía al servidor lo que esté en la cola. Va dentro del SQLiteProvider.
 */
export function useServiciosTelemetria() {
  const db = useSQLiteContext();

  // Envío: al abrir, al volver a la app, al encolar algo y cada minuto
  useEffect(() => {
    const vaciar = () => void vaciarCola(db);
    void contarPendientes(db);
    vaciar();
    const quitarOyente = alEncolar(vaciar);
    const intervalo = setInterval(vaciar, INTERVALO_ENVIO_MS);
    const suscripcion = AppState.addEventListener('change', (estado) => estado === 'active' && vaciar());
    return () => {
      quitarOyente();
      clearInterval(intervalo);
      suscripcion.remove();
    };
  }, [db]);

  // Sesiones de cruce
  useEffect(() => {
    let sesion: SesionCruce | null = null;
    let simulada = false;
    let gps: Location.LocationSubscription | null = null;

    // El permiso se pide al abrir la app y no en plena esquina, donde un diálogo estorba
    void Location.requestForegroundPermissionsAsync().catch(() => undefined);

    async function empezar() {
      const id = idAleatorio();
      sesion = nuevaSesion(id, Date.now());
      simulada = useBrazalete.getState().simulado;
      const servidor = configServidor();
      if (servidor) {
        despertarServidor(servidor);
      }
      try {
        const { granted } = await Location.getForegroundPermissionsAsync();
        if (!granted) {
          registrar('Cruce sin GPS: falta el permiso de ubicación');
          return;
        }
        const suscripcion = await Location.watchPositionAsync(
          { accuracy: Location.Accuracy.BestForNavigation, timeInterval: 1000, distanceInterval: 1 },
          ({ coords }) => {
            if (sesion?.id === id) {
              sesion = conPosicion(
                sesion,
                { latitud: coords.latitude, longitud: coords.longitude, precisionMetros: coords.accuracy },
                Date.now(),
              );
            }
          },
        );
        // La sesión pudo terminar mientras arrancaba el GPS
        if (sesion?.id === id) {
          gps = suscripcion;
        } else {
          suscripcion.remove();
        }
      } catch (error) {
        registrar(`No se pudo seguir el GPS: ${String(error)}`);
      }
    }

    async function terminar() {
      const terminada = sesion;
      sesion = null;
      gps?.remove();
      gps = null;
      if (!terminada) {
        return;
      }
      const resumen = resumir(terminada, Date.now());
      try {
        // En modo simulado el GPS es real pero el semáforo no: no se guarda en la lista
        const semaforoId = simulada ? null : await semaforoDelCruce(db, resumen);
        await encolar(db, 'cruce', { ...resumen, semaforo_id: semaforoId, simulado: simulada });
        registrar(
          resumen.cruzo
            ? `Cruce registrado: espera ${resumen.espera_s} s, cruce ${resumen.duracion_cruce_s} s`
            : 'Sesión registrada sin cruce',
        );
      } catch (error) {
        registrar(`No se pudo registrar el cruce: ${String(error)}`);
      }
    }

    const quitar = useBrazalete.subscribe((actual, anterior) => {
      if (actual.conexion === 'conectado' && anterior.conexion !== 'conectado') {
        void empezar();
      } else if (actual.conexion !== 'conectado' && anterior.conexion === 'conectado') {
        void terminar();
      }
      if (sesion && actual.telemetria && actual.telemetria !== anterior.telemetria) {
        sesion = conPaquete(sesion, actual.telemetria, Date.now());
      }
    });
    return () => {
      quitar();
      gps?.remove();
    };
  }, [db]);
}
