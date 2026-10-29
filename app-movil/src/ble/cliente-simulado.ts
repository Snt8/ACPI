import type { ClienteBrazalete, EventosCliente } from '@/ble/cliente';
import { parsearTelemetria } from '@/protocolo/brazalete';

const INTERVALO_PAQUETES_MS = 500; // igual que INTERVALO_TELEMETRIA_BLE_MS del firmware
const ESPERA_CONEXION_MS = 2000;

// Un cruce de ejemplo: la persona llega mal orientada, se corrige, espera el rojo y
// cruza con el verde. Cada tramo dura "paquetes" × 500 ms.
const GUION = [
  { st: 2, paquetes: 8, desde: 25 },
  { st: 0, paquetes: 30, desde: 21 },
  { st: 1, paquetes: 50, desde: 25 },
] as const;

/** Paquetes JSON idénticos a los del firmware, en bucle */
export function* paquetesSimulados(): Generator<string> {
  let rumbo = 300;
  for (;;) {
    for (const tramo of GUION) {
      for (let i = 0; i < tramo.paquetes; i++) {
        // Mal orientada gira poco a poco hacia el cruce (0°); después queda quieta
        rumbo = tramo.st === 2 ? (rumbo + 8) % 360 : 0;
        const segundos = Math.max(tramo.desde - Math.floor(i / 2), 0);
        yield JSON.stringify({ hd: rumbo, st: tramo.st, ok: tramo.st !== 2, tr: segundos, bt: -1 });
      }
    }
  }
}

export function crearClienteSimulado(eventos: EventosCliente): ClienteBrazalete {
  let temporizador: ReturnType<typeof setTimeout> | null = null;
  let intervalo: ReturnType<typeof setInterval> | null = null;

  return {
    async iniciar() {
      eventos.alRegistrar('Modo simulado: sin Bluetooth');
      eventos.alCambiarConexion('buscando');
      temporizador = setTimeout(() => {
        eventos.alCambiarConexion('conectado');
        eventos.alRegistrar('Conectado al brazalete simulado');
        const paquetes = paquetesSimulados();
        intervalo = setInterval(() => {
          const texto = paquetes.next().value as string;
          const telemetria = parsearTelemetria(texto);
          if (telemetria) {
            eventos.alRecibirTelemetria(telemetria, texto);
          }
        }, INTERVALO_PAQUETES_MS);
      }, ESPERA_CONEXION_MS);
    },

    async detener() {
      if (temporizador) clearTimeout(temporizador);
      if (intervalo) clearInterval(intervalo);
      temporizador = intervalo = null;
      eventos.alCambiarConexion('apagado');
    },

    async enviarComando(comando) {
      eventos.alRegistrar(`Comando simulado enviado: ${comando}`);
    },
  };
}
