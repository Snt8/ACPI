import { AppState, PermissionsAndroid, Platform, type EventSubscription } from 'react-native';
import BleManager, { BleScanMode } from 'react-native-ble-manager';

import { entregarPaquete, type ClienteBrazalete, type EstadoConexion, type EventosCliente } from '@/ble/cliente';
import {
  anotarInicio,
  esperaPorLimite,
  esperaTrasFallo,
  evaluarConectado,
  evaluarEncontrado,
  permisosAndroid,
  planBusqueda,
  reaccionAlAdaptador,
} from '@/ble/decisiones';
import { UUID_RX, UUID_SERVICIO, UUID_TX, bytesATexto, mismoUuid, textoABytes } from '@/protocolo/brazalete';

// El brazalete sólo enciende el BLE cerca de un semáforo (fuera de eso está en
// standby), así que la app busca todo el tiempo que no está conectada. Qué buscar,
// cuánto esperar y a quién conectarse lo decide ble/decisiones.ts.

// El JSON de telemetría ocupa hasta 160 bytes; con el MTU por defecto (23) se cortaría.
// iOS negocia el MTU solo; en Android hay que pedirlo.
const MTU_PEDIDO = 185;
// iOS nunca da por fallida una conexión: sin un límite propio se quedaría "conectando"
const TIEMPO_CONEXION_MS = 10_000;

/** Lo que el cliente usa del teléfono; las pruebas lo reemplazan por dobles */
export interface EntornoBle {
  ble: Pick<
    typeof BleManager,
    | 'start'
    | 'checkState'
    | 'scan'
    | 'stopScan'
    | 'connect'
    | 'disconnect'
    | 'requestMTU'
    | 'retrieveServices'
    | 'startNotification'
    | 'write'
    | 'getConnectedPeripherals'
    | 'onDidUpdateState'
    | 'onDiscoverPeripheral'
    | 'onStopScan'
    | 'onDisconnectPeripheral'
    | 'onDidUpdateValueForCharacteristic'
  >;
  plataforma: string;
  /** Con `conDialogo` false sólo mira si ya están concedidos (al volver de Ajustes) */
  pedirPermisos(conDialogo: boolean): Promise<boolean>;
  enPrimerPlano(): boolean;
  alCambiarPrimerPlano(oyente: (primerPlano: boolean) => void): () => void;
  ahora(): number;
}

// Un intento de conexión que ya no sirve (se detuvo el cliente o se apagó el Bluetooth)
class Cancelado extends Error {}

const mensaje = (error: unknown) => (error instanceof Error ? error.message : String(error));

export function crearClienteBleCon(eventos: EventosCliente, entorno: EntornoBle): ClienteBrazalete {
  const { ble } = entorno;
  let activo = false;
  let suscripciones: EventSubscription[] = [];
  let quitarPrimerPlano: (() => void) | null = null;
  // Próximo intento programado (arranque o búsqueda); nunca hay más de uno
  let temporizador: ReturnType<typeof setTimeout> | null = null;
  let fallos = 0; // errores seguidos, para la espera creciente
  let arrancado = false; // BleManager.start hecho y oyentes puestos
  let arrancando = false;
  let adaptadorListo = false;
  let ultimoAdaptador: string | null = null;
  let revision: Promise<void> | null = null; // búsqueda de una conexión que quedó abierta
  let escaneando = false;
  let numeroBusqueda = 0; // invalida la promesa de una búsqueda vieja
  let iniciosBusqueda: number[] = [];
  let intento: { id: string; abortar(error: Error): void } | null = null; // conexión en curso
  let periferico: string | null = null; // id del brazalete conectado
  let ultimoEstado: EstadoConexion | null = null;
  const descartados = new Set<string>(); // para registrar cada descarte una sola vez

  // Un cliente detenido no emite nada: sus promesas pendientes pueden resolverse tarde
  function cambiar(estado: EstadoConexion) {
    if (activo && estado !== ultimoEstado) {
      ultimoEstado = estado;
      eventos.alCambiarConexion(estado);
    }
  }
  function registrar(texto: string) {
    if (activo) {
      eventos.alRegistrar(texto);
    }
  }

  function programar(espera: number, accion: () => void) {
    cancelarProgramado();
    temporizador = setTimeout(() => {
      temporizador = null;
      accion();
    }, espera);
  }
  function cancelarProgramado() {
    if (temporizador) {
      clearTimeout(temporizador);
      temporizador = null;
    }
  }
  function reintentarTrasFallo(accion: () => void) {
    fallos++;
    const espera = esperaTrasFallo(fallos);
    registrar(`Nuevo intento en ${espera / 1000} s`);
    programar(espera, accion);
  }

  async function arrancar(conDialogo: boolean) {
    if (!activo || arrancado || arrancando) {
      return;
    }
    arrancando = true;
    try {
      if (!(await entorno.pedirPermisos(conDialogo))) {
        // Se vuelve a mirar cuando la app vuelve al frente (por ejemplo, desde Ajustes)
        cambiar('sin-permisos');
        return;
      }
      if (!activo) {
        return;
      }
      await ble.start({ showAlert: false });
      if (!activo) {
        return;
      }
      escuchar();
      arrancado = true;
      fallos = 0;
      // checkState también emite onDidUpdateState; alCambiarAdaptador ignora el repetido
      alCambiarAdaptador(await ble.checkState());
    } catch (error) {
      registrar(`No se pudo iniciar el Bluetooth: ${mensaje(error)}`);
      if (!arrancado) {
        reintentarTrasFallo(() => void arrancar(false));
      }
    } finally {
      arrancando = false;
    }
  }

  function alCambiarAdaptador(estado: string) {
    if (!activo || estado === ultimoAdaptador) {
      return;
    }
    ultimoAdaptador = estado;
    registrar(`Bluetooth: ${estado}`);
    const reaccion = reaccionAlAdaptador(estado);
    adaptadorListo = reaccion === 'listo';
    if (reaccion === 'listo') {
      void alEstarListo();
      return;
    }
    // Con el adaptador fuera de servicio no hay búsqueda en curso (en Android la
    // promesa de scan quedaría colgada) ni nada que reintentar
    escaneando = false;
    numeroBusqueda++;
    cancelarProgramado();
    if (reaccion === 'esperar') {
      return;
    }
    olvidarConexion();
    cambiar(reaccion === 'apagado' ? 'bluetooth-apagado' : reaccion);
  }

  function olvidarConexion() {
    const pendiente = intento;
    intento = null;
    pendiente?.abortar(new Cancelado());
    periferico = null;
  }

  async function alEstarListo() {
    revision ??= recuperarConexionPrevia();
    await revision;
    buscar();
  }

  /**
   * Tras recargar el JS (o si la app murió con la conexión abierta) el sistema puede
   * seguir conectado al brazalete: como atiende a un solo teléfono, deja de anunciarse
   * y nunca aparecería en la búsqueda. Se reutiliza esa conexión.
   */
  async function recuperarConexionPrevia() {
    try {
      let conectados = await ble.getConnectedPeripherals([UUID_SERVICIO]);
      // En iOS la librería sólo devuelve en la segunda llamada los que aún no conocía
      if (conectados.length === 0 && entorno.plataforma === 'ios') {
        conectados = await ble.getConnectedPeripherals([UUID_SERVICIO]);
      }
      let elegido: string | null = null;
      for (const p of conectados) {
        if (!activo) {
          return;
        }
        if (!evaluarConectado(p, entorno.plataforma).aceptar) {
          continue; // otro equipo del teléfono: no se toca
        }
        if (elegido === null && !periferico && !intento) {
          elegido = p.id;
        } else {
          registrar(`Se cierra una conexión sobrante con ${p.id}`);
          await ble.disconnect(p.id).catch(() => {});
        }
      }
      if (elegido && activo) {
        registrar(`El brazalete seguía conectado (${elegido}): se reutiliza la conexión`);
        void conectar(elegido);
      }
    } catch (error) {
      registrar(`No se pudieron revisar las conexiones abiertas: ${mensaje(error)}`);
    }
  }

  /** Empieza una búsqueda; con `reiniciar` reemplaza la que esté en curso */
  function buscar(reiniciar = false) {
    if (!activo || !arrancado || !adaptadorListo || periferico || intento || temporizador) {
      return;
    }
    if (escaneando && !reiniciar) {
      return;
    }
    const ahora = entorno.ahora();
    const espera = entorno.plataforma === 'android' ? esperaPorLimite(iniciosBusqueda, ahora) : 0;
    if (espera > 0) {
      programar(espera, () => buscar(reiniciar));
      return;
    }
    const plan = planBusqueda(entorno.enPrimerPlano());
    const numero = ++numeroBusqueda;
    escaneando = true;
    iniciosBusqueda = anotarInicio(iniciosBusqueda, ahora);
    cambiar('buscando');
    // Filtrar por el UUID del servicio es lo que permite buscar en segundo plano en
    // iOS. Llamar a scan con otra en curso la reemplaza en las dos plataformas.
    ble
      .scan({
        serviceUUIDs: [UUID_SERVICIO],
        seconds: plan.segundos,
        scanMode: plan.rapida ? BleScanMode.LowLatency : BleScanMode.LowPower,
      })
      .catch((error: unknown) => {
        if (!activo || numero !== numeroBusqueda) {
          return;
        }
        escaneando = false;
        registrar(`No se pudo buscar: ${mensaje(error)}`);
        reintentarTrasFallo(() => buscar());
      });
  }

  async function conectar(id: string) {
    let abortar: (error: Error) => void = () => {};
    const abortado = new Promise<never>((_, rechazar) => {
      abortar = rechazar;
    });
    const actual = { id, abortar };
    intento = actual;
    const vigente = () => activo && intento === actual;
    const limite = setTimeout(
      () => abortar(new Error(`sin respuesta en ${TIEMPO_CONEXION_MS / 1000} s`)),
      TIEMPO_CONEXION_MS,
    );
    cambiar('conectando');
    if (escaneando) {
      escaneando = false;
      numeroBusqueda++;
      void ble.stopScan().catch(() => {});
    }
    try {
      await Promise.race([pasosConexion(id, vigente), abortado]);
      if (!vigente()) {
        throw new Cancelado();
      }
      periferico = id;
      fallos = 0;
      cambiar('conectado');
      registrar(`Conectado a ${id}`);
    } catch (error) {
      if (!vigente()) {
        return; // detener() o el Bluetooth apagado ya se ocuparon
      }
      intento = null; // los pasos que sigan en curso ven que ya no sirven
      registrar(`Falló la conexión: ${mensaje(error)}`);
      await ble.disconnect(id).catch(() => {});
      if (!activo) {
        return;
      }
      cambiar('buscando');
      reintentarTrasFallo(() => buscar());
    } finally {
      clearTimeout(limite);
      if (intento === actual) {
        intento = null;
      }
    }
  }

  async function pasosConexion(id: string, vigente: () => boolean) {
    const seguir = () => {
      if (!vigente()) {
        throw new Cancelado();
      }
    };
    await ble.connect(id);
    seguir();
    if (entorno.plataforma === 'android') {
      const mtu = await ble.requestMTU(id, MTU_PEDIDO);
      seguir();
      registrar(`MTU negociado: ${mtu}`);
    }
    await ble.retrieveServices(id);
    seguir();
    await ble.startNotification(id, UUID_SERVICIO, UUID_TX);
  }

  function alCambiarPrimerPlano(primerPlano: boolean) {
    if (!activo) {
      return;
    }
    if (!arrancado) {
      // Quizá volvió de Ajustes con el permiso concedido
      if (primerPlano) {
        cancelarProgramado();
        fallos = 0;
        void arrancar(false);
      }
      return;
    }
    // No se espera a un reintento programado: en iOS los temporizadores no corren en
    // segundo plano, y al volver al frente la persona espera que busque ya
    cancelarProgramado();
    fallos = 0;
    if (primerPlano) {
      ble
        .checkState()
        .then(alCambiarAdaptador)
        .catch(() => {});
    }
    buscar(true); // con la velocidad que corresponde al nuevo plano
  }

  function escuchar() {
    suscripciones = [
      ble.onDidUpdateState(({ state }) => alCambiarAdaptador(state)),
      ble.onDiscoverPeripheral((p) => {
        if (!activo || periferico || intento) {
          return;
        }
        const veredicto = evaluarEncontrado(p);
        if (!veredicto.aceptar) {
          // En Android llega en cada anuncio: se registra una vez
          if (!descartados.has(p.id)) {
            descartados.add(p.id);
            registrar(`Se descarta ${p.id}: ${veredicto.motivo}`);
          }
          return;
        }
        if (veredicto.nota) {
          registrar(`${p.id} ${veredicto.nota}; se acepta porque anuncia el servicio del brazalete`);
        }
        void conectar(p.id);
      }),
      ble.onStopScan(({ status }) => {
        if (!activo) {
          return;
        }
        escaneando = false;
        if (periferico || intento) {
          return;
        }
        // 10: se cumplió el tiempo; 0: stopScan en iOS. Otro código es un error de Android.
        if (status !== undefined && status !== 0 && status !== 10) {
          registrar(`La búsqueda se detuvo con el código ${status}`);
          reintentarTrasFallo(() => buscar());
          return;
        }
        buscar();
      }),
      ble.onDisconnectPeripheral(({ peripheral }) => {
        if (!activo) {
          return;
        }
        if (intento && peripheral === intento.id) {
          intento.abortar(new Error('el brazalete cortó la conexión'));
          return;
        }
        if (peripheral !== periferico) {
          return;
        }
        periferico = null;
        registrar('Brazalete desconectado');
        // Lo normal es que el brazalete haya vuelto a standby al alejarse del
        // semáforo; se vuelve a buscar enseguida por si fue un corte
        fallos = 0;
        cambiar('buscando');
        buscar();
      }),
      ble.onDidUpdateValueForCharacteristic(({ peripheral, characteristic, value }) => {
        if (!activo || peripheral !== periferico || !mismoUuid(characteristic, UUID_TX)) {
          return;
        }
        entregarPaquete(eventos, bytesATexto(value));
      }),
    ];
  }

  return {
    async iniciar() {
      if (activo) {
        return;
      }
      activo = true;
      ultimoEstado = null;
      quitarPrimerPlano = entorno.alCambiarPrimerPlano(alCambiarPrimerPlano);
      await arrancar(true);
    },

    async detener() {
      activo = false;
      cancelarProgramado();
      quitarPrimerPlano?.();
      quitarPrimerPlano = null;
      suscripciones.forEach((s) => s.remove());
      suscripciones = [];
      const pendiente = intento;
      const conectado = periferico;
      olvidarConexion();
      if (arrancado) {
        await ble.stopScan().catch(() => {});
        for (const id of new Set([pendiente?.id, conectado])) {
          if (id) {
            await ble.disconnect(id).catch(() => {});
          }
        }
      }
      escaneando = false;
      arrancado = false;
      adaptadorListo = false;
      ultimoAdaptador = null;
      revision = null;
      eventos.alCambiarConexion('apagado');
    },

    async enviarComando(comando) {
      if (!activo || !periferico) {
        throw new Error('El brazalete no está conectado');
      }
      await ble.write(periferico, UUID_SERVICIO, UUID_RX, textoABytes(comando));
      registrar(`Comando enviado: ${comando}`);
    },
  };
}

async function pedirPermisosAndroid(conDialogo: boolean): Promise<boolean> {
  if (Platform.OS !== 'android') {
    return true; // iOS pide el permiso al arrancar BleManager y avisa con el estado "unauthorized"
  }
  const permisos = permisosAndroid(Number(Platform.Version));
  if (!conDialogo) {
    const concedidos = await Promise.all(permisos.map((p) => PermissionsAndroid.check(p)));
    return concedidos.every(Boolean);
  }
  const resultado = await PermissionsAndroid.requestMultiple(permisos);
  return permisos.every((p) => resultado[p] === PermissionsAndroid.RESULTS.GRANTED);
}

export function crearClienteBle(eventos: EventosCliente): ClienteBrazalete {
  // "inactive" (iOS, por ejemplo con el centro de control abierto) cuenta como al frente
  const alFrente = (estado: string | null | undefined) => estado !== 'background';
  return crearClienteBleCon(eventos, {
    ble: BleManager,
    plataforma: Platform.OS,
    pedirPermisos: pedirPermisosAndroid,
    enPrimerPlano: () => alFrente(AppState.currentState),
    alCambiarPrimerPlano(oyente) {
      let anterior = alFrente(AppState.currentState);
      const suscripcion = AppState.addEventListener('change', (estado) => {
        if (alFrente(estado) !== anterior) {
          anterior = alFrente(estado);
          oyente(anterior);
        }
      });
      return () => suscripcion.remove();
    },
    ahora: Date.now,
  });
}
