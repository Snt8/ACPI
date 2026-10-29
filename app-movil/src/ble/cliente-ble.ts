import { PermissionsAndroid, Platform, type EventSubscription } from 'react-native';
import BleManager, { BleState } from 'react-native-ble-manager';

import type { ClienteBrazalete, EventosCliente } from '@/ble/cliente';
import {
  NOMBRE_BRAZALETE,
  UUID_RX,
  UUID_SERVICIO,
  UUID_TX,
  bytesATexto,
  parsearTelemetria,
  textoABytes,
} from '@/protocolo/brazalete';

// El brazalete sólo enciende el BLE cerca de un semáforo (fuera de eso está en
// standby), así que la app busca en tandas y reintenta con esperas crecientes
const SEGUNDOS_POR_BUSQUEDA = 10;
const ESPERAS_REINTENTO_MS = [2000, 5000, 10000, 30000];
// El JSON de telemetría ocupa hasta 160 bytes; con el MTU por defecto (23) se cortaría.
// iOS negocia el MTU solo; en Android hay que pedirlo.
const MTU_PEDIDO = 185;

const mismoUuid = (a: string, b: string) => a.toLowerCase() === b.toLowerCase();

async function pedirPermisosAndroid(): Promise<boolean> {
  if (Platform.OS !== 'android') {
    return true;
  }
  const permisos =
    Platform.Version >= 31
      ? [
          PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN,
          PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT,
          PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
        ]
      : [PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION];
  const resultado = await PermissionsAndroid.requestMultiple(permisos);
  return permisos.every((p) => resultado[p] === PermissionsAndroid.RESULTS.GRANTED);
}

export function crearClienteBle(eventos: EventosCliente): ClienteBrazalete {
  let suscripciones: EventSubscription[] = [];
  let reintento: ReturnType<typeof setTimeout> | null = null;
  let intentos = 0;
  let activo = false;
  let conectando = false;
  let periferico: string | null = null; // id del brazalete conectado

  function programarReintento() {
    if (!activo || reintento) {
      return;
    }
    const espera = ESPERAS_REINTENTO_MS[Math.min(intentos, ESPERAS_REINTENTO_MS.length - 1)];
    intentos++;
    reintento = setTimeout(() => {
      reintento = null;
      void buscar();
    }, espera);
  }

  async function buscar() {
    if (!activo || conectando || periferico) {
      return;
    }
    eventos.alCambiarConexion('buscando');
    try {
      // Filtrar por el UUID del servicio es lo que permite buscar en segundo plano en iOS
      await BleManager.scan({ serviceUUIDs: [UUID_SERVICIO], seconds: SEGUNDOS_POR_BUSQUEDA });
    } catch (error) {
      eventos.alRegistrar(`No se pudo buscar: ${String(error)}`);
      programarReintento();
    }
  }

  async function conectar(id: string) {
    conectando = true;
    eventos.alCambiarConexion('conectando');
    try {
      await BleManager.connect(id);
      if (Platform.OS === 'android') {
        const mtu = await BleManager.requestMTU(id, MTU_PEDIDO);
        eventos.alRegistrar(`MTU negociado: ${mtu}`);
      }
      await BleManager.retrieveServices(id);
      await BleManager.startNotification(id, UUID_SERVICIO, UUID_TX);
      periferico = id;
      intentos = 0;
      eventos.alCambiarConexion('conectado');
      eventos.alRegistrar(`Conectado a ${id}`);
    } catch (error) {
      eventos.alRegistrar(`Falló la conexión: ${String(error)}`);
      await BleManager.disconnect(id).catch(() => {});
      eventos.alCambiarConexion('buscando');
      programarReintento();
    } finally {
      conectando = false;
    }
  }

  function escuchar() {
    suscripciones = [
      BleManager.onDidUpdateState(({ state }) => {
        eventos.alRegistrar(`Bluetooth: ${state}`);
        if (state === BleState.On) {
          void buscar();
        } else if (state === BleState.Off) {
          periferico = null;
          eventos.alCambiarConexion('bluetooth-apagado');
        }
      }),
      BleManager.onDiscoverPeripheral((p) => {
        const nombre = p.name ?? p.advertising?.localName;
        // El filtro por servicio ya lo hace el sistema; el nombre evita conectar
        // con otro equipo que reutilice estos UUID de ejemplo
        if (nombre && nombre !== NOMBRE_BRAZALETE) {
          return;
        }
        if (!conectando && !periferico) {
          void BleManager.stopScan().catch(() => {});
          void conectar(p.id);
        }
      }),
      BleManager.onStopScan(() => {
        if (!conectando && !periferico) {
          programarReintento();
        }
      }),
      BleManager.onDisconnectPeripheral(({ peripheral }) => {
        if (peripheral !== periferico) {
          return;
        }
        periferico = null;
        eventos.alRegistrar('Brazalete desconectado');
        // Lo normal es que el brazalete haya vuelto a standby al alejarse del
        // semáforo; se vuelve a buscar enseguida por si fue un corte
        intentos = 0;
        eventos.alCambiarConexion('buscando');
        programarReintento();
      }),
      BleManager.onDidUpdateValueForCharacteristic(({ characteristic, value }) => {
        if (!mismoUuid(characteristic, UUID_TX)) {
          return;
        }
        const texto = bytesATexto(value);
        const telemetria = parsearTelemetria(texto);
        if (telemetria) {
          eventos.alRecibirTelemetria(telemetria, texto);
        } else {
          eventos.alRegistrar(`Paquete descartado: ${texto}`);
        }
      }),
    ];
  }

  return {
    async iniciar() {
      if (activo) {
        return;
      }
      if (!(await pedirPermisosAndroid())) {
        eventos.alCambiarConexion('sin-permisos');
        return;
      }
      activo = true;
      await BleManager.start({ showAlert: false });
      escuchar();
      const estado = await BleManager.checkState();
      if (estado === BleState.On) {
        await buscar();
      } else if (estado === BleState.Unauthorized) {
        eventos.alCambiarConexion('sin-permisos');
      } else {
        // En iOS el estado puede llegar "unknown" al arrancar; onDidUpdateState
        // avisa cuando el Bluetooth esté listo
        eventos.alCambiarConexion('bluetooth-apagado');
      }
    },

    async detener() {
      activo = false;
      if (reintento) {
        clearTimeout(reintento);
        reintento = null;
      }
      suscripciones.forEach((s) => s.remove());
      suscripciones = [];
      await BleManager.stopScan().catch(() => {});
      if (periferico) {
        await BleManager.disconnect(periferico).catch(() => {});
        periferico = null;
      }
      eventos.alCambiarConexion('apagado');
    },

    async enviarComando(comando) {
      if (!periferico) {
        throw new Error('El brazalete no está conectado');
      }
      await BleManager.write(periferico, UUID_SERVICIO, UUID_RX, textoABytes(comando));
      eventos.alRegistrar(`Comando enviado: ${comando}`);
    },
  };
}
