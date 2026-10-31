import type { Peripheral } from 'react-native-ble-manager';

import type { EstadoConexion, EventosCliente } from '@/ble/cliente';
import { crearClienteBleCon, type EntornoBle } from '@/ble/cliente-ble';
import { UUID_RX, UUID_SERVICIO, UUID_TX, textoABytes } from '@/protocolo/brazalete';

// La librería nativa no existe en jest: el cliente recibe un doble por el entorno y
// de la librería sólo usa las constantes de scanMode
jest.mock('react-native-ble-manager', () => ({
  __esModule: true,
  default: {},
  BleScanMode: { Opportunistic: -1, LowPower: 0, Balanced: 1, LowLatency: 2 },
}));

type Oyente = (evento: any) => void;

/** Doble de BleManager: cada método es un jest.fn y `emitir` simula los eventos nativos */
function crearBleFalso() {
  const oyentes = new Map<string, Set<Oyente>>();
  const escuchar = (nombre: string) =>
    jest.fn((oyente: Oyente) => {
      if (!oyentes.has(nombre)) oyentes.set(nombre, new Set());
      oyentes.get(nombre)!.add(oyente);
      return { remove: () => oyentes.get(nombre)!.delete(oyente) };
    });
  const emitir = (nombre: string, evento: unknown) => [...(oyentes.get(nombre) ?? [])].forEach((o) => o(evento));
  const falso = {
    estado: 'on',
    oyentes,
    emitir,
    ble: {
      start: jest.fn(async () => {}),
      // Como la librería real: además de devolverlo, emite onDidUpdateState
      checkState: jest.fn(async (): Promise<string> => {
        emitir('estado', { state: falso.estado });
        return falso.estado;
      }),
      scan: jest.fn(async () => {}),
      stopScan: jest.fn(async () => {}),
      connect: jest.fn(async () => {}),
      disconnect: jest.fn(async () => {}),
      requestMTU: jest.fn(async () => 185),
      retrieveServices: jest.fn(async () => ({})),
      startNotification: jest.fn(async () => {}),
      write: jest.fn(async () => {}),
      getConnectedPeripherals: jest.fn(async (): Promise<Peripheral[]> => []),
      onDidUpdateState: escuchar('estado'),
      onDiscoverPeripheral: escuchar('encontrado'),
      onStopScan: escuchar('fin-busqueda'),
      onDisconnectPeripheral: escuchar('desconectado'),
      onDidUpdateValueForCharacteristic: escuchar('valor'),
    },
  };
  return falso;
}

function preparar({ plataforma = 'ios', permisos = true }: { plataforma?: string; permisos?: boolean } = {}) {
  const falso = crearBleFalso();
  const estados: EstadoConexion[] = [];
  const registro: string[] = [];
  const paquetes: string[] = [];
  const eventos: EventosCliente = {
    alCambiarConexion: (e) => estados.push(e),
    alRecibirTelemetria: (_t, texto) => paquetes.push(texto),
    alRegistrar: (m) => registro.push(m),
  };
  let primerPlano = true;
  let oyentePlano: ((primerPlano: boolean) => void) | null = null;
  const entorno = {
    ble: falso.ble as unknown as EntornoBle['ble'],
    plataforma,
    pedirPermisos: jest.fn(async (_conDialogo: boolean) => permisos),
    enPrimerPlano: () => primerPlano,
    alCambiarPrimerPlano: jest.fn((oyente: (primerPlano: boolean) => void) => {
      oyentePlano = oyente;
      return () => {
        oyentePlano = null;
      };
    }),
    ahora: () => Date.now(),
  };
  const cliente = crearClienteBleCon(eventos, entorno);
  const cambiarPlano = (valor: boolean) => {
    primerPlano = valor;
    oyentePlano?.(valor);
  };
  return { falso, ble: falso.ble, cliente, entorno, estados, registro, paquetes, cambiarPlano, hayOyentePlano: () => !!oyentePlano };
}

const brazalete = (id = 'B1', datos: Partial<Peripheral['advertising']> & { name?: string } = {}): Peripheral => ({
  id,
  rssi: -60,
  name: datos.name,
  advertising: { localName: 'ACPI_Brazalete', serviceUUIDs: [UUID_SERVICIO], ...datos },
});

// Deja correr las promesas pendientes (y los temporizadores de `ms`)
const esperar = (ms = 0) => jest.advanceTimersByTimeAsync(ms);

beforeEach(() => {
  jest.useFakeTimers();
});
afterEach(() => {
  jest.useRealTimers();
});

describe('arranque', () => {
  it('busca una sola vez por el UUID del servicio, rápido y con la app al frente', async () => {
    const { ble, cliente, estados } = preparar({ plataforma: 'android' });
    await cliente.iniciar();
    await esperar();
    expect(ble.start).toHaveBeenCalledTimes(1);
    // checkState devuelve "on" y además emite el evento: no debe buscar dos veces
    expect(ble.scan).toHaveBeenCalledTimes(1);
    expect(ble.scan).toHaveBeenCalledWith({ serviceUUIDs: [UUID_SERVICIO], seconds: 30, scanMode: 2 });
    expect(estados).toEqual(['buscando']);
  });

  it('con el estado "unknown" de iOS no anuncia el Bluetooth apagado y espera', async () => {
    const { falso, ble, cliente, estados } = preparar();
    falso.estado = 'unknown';
    await cliente.iniciar();
    await esperar();
    expect(estados).toEqual([]);
    expect(ble.scan).not.toHaveBeenCalled();
    falso.emitir('estado', { state: 'on' });
    await esperar();
    expect(ble.scan).toHaveBeenCalledTimes(1);
    expect(estados).toEqual(['buscando']);
  });

  it('no busca con el Bluetooth apagado y busca al encenderlo', async () => {
    const { falso, ble, cliente, estados } = preparar();
    falso.estado = 'off';
    await cliente.iniciar();
    await esperar(60_000);
    expect(estados).toEqual(['bluetooth-apagado']);
    expect(ble.scan).not.toHaveBeenCalled();
    falso.emitir('estado', { state: 'on' });
    await esperar();
    expect(ble.scan).toHaveBeenCalledTimes(1);
  });

  it('distingue sin permiso y sin Bluetooth compatible', async () => {
    const a = preparar();
    a.falso.estado = 'unauthorized';
    await a.cliente.iniciar();
    await esperar();
    expect(a.estados).toEqual(['sin-permisos']);

    const b = preparar();
    b.falso.estado = 'unsupported';
    await b.cliente.iniciar();
    await esperar();
    expect(b.estados).toEqual(['no-compatible']);
  });

  it('si BleManager.start falla lo reintenta', async () => {
    const { ble, cliente, registro } = preparar();
    ble.start.mockRejectedValueOnce(new Error('sin módulo'));
    await cliente.iniciar();
    expect(ble.scan).not.toHaveBeenCalled();
    expect(registro).toContain('No se pudo iniciar el Bluetooth: sin módulo');
    await esperar(2000);
    expect(ble.start).toHaveBeenCalledTimes(2);
    expect(ble.scan).toHaveBeenCalledTimes(1);
  });

  it('sin permisos vuelve a mirar al volver la app al frente, sin otro diálogo', async () => {
    const { entorno, ble, cliente, estados, cambiarPlano } = preparar({ plataforma: 'android', permisos: false });
    await cliente.iniciar();
    expect(estados).toEqual(['sin-permisos']);
    expect(entorno.pedirPermisos).toHaveBeenLastCalledWith(true);
    entorno.pedirPermisos.mockResolvedValue(true);
    cambiarPlano(false);
    cambiarPlano(true);
    await esperar();
    expect(entorno.pedirPermisos).toHaveBeenLastCalledWith(false);
    expect(ble.scan).toHaveBeenCalledTimes(1);
    expect(estados).toEqual(['sin-permisos', 'buscando']);
  });
});

describe('conexión', () => {
  it('conecta, pide el MTU sólo en Android y se suscribe a TX', async () => {
    for (const plataforma of ['ios', 'android']) {
      const { falso, ble, cliente, estados } = preparar({ plataforma });
      await cliente.iniciar();
      await esperar();
      falso.emitir('encontrado', brazalete());
      await esperar();
      expect(ble.stopScan).toHaveBeenCalled();
      expect(ble.connect).toHaveBeenCalledWith('B1');
      expect(ble.requestMTU).toHaveBeenCalledTimes(plataforma === 'android' ? 1 : 0);
      if (plataforma === 'android') expect(ble.requestMTU).toHaveBeenCalledWith('B1', 185);
      expect(ble.startNotification).toHaveBeenCalledWith('B1', UUID_SERVICIO, UUID_TX);
      expect(estados).toEqual(['buscando', 'conectando', 'conectado']);
    }
  });

  it('acepta el brazalete con un nombre viejo guardado por iOS', async () => {
    const { falso, ble, cliente, registro } = preparar();
    await cliente.iniciar();
    await esperar();
    falso.emitir('encontrado', brazalete('B1', { name: 'ACPI_Pulsera', localName: undefined }));
    await esperar();
    expect(ble.connect).toHaveBeenCalledWith('B1');
    expect(registro.some((l) => l.includes('se anuncia como "ACPI_Pulsera"'))).toBe(true);
  });

  it('registra una sola vez cada equipo descartado', async () => {
    const { falso, ble, cliente, registro } = preparar({ plataforma: 'android' });
    await cliente.iniciar();
    await esperar();
    const otro = brazalete('X', { localName: 'Reloj', serviceUUIDs: ['180d'] });
    falso.emitir('encontrado', otro);
    falso.emitir('encontrado', otro);
    await esperar();
    expect(ble.connect).not.toHaveBeenCalled();
    expect(registro.filter((l) => l.startsWith('Se descarta X'))).toHaveLength(1);
  });

  it('no abre dos conexiones si el brazalete aparece dos veces', async () => {
    const { falso, ble, cliente } = preparar();
    ble.connect.mockImplementation(() => new Promise(() => {}));
    await cliente.iniciar();
    await esperar();
    falso.emitir('encontrado', brazalete('B1'));
    falso.emitir('encontrado', brazalete('B2'));
    await esperar();
    expect(ble.connect).toHaveBeenCalledTimes(1);
  });

  it('una conexión que no responde se corta a los 10 s y se vuelve a buscar', async () => {
    const { falso, ble, cliente, estados } = preparar();
    ble.connect.mockImplementationOnce(() => new Promise(() => {})); // iOS nunca la da por fallida
    await cliente.iniciar();
    await esperar();
    falso.emitir('encontrado', brazalete());
    await esperar(9_999);
    expect(estados.at(-1)).toBe('conectando');
    await esperar(1);
    expect(ble.disconnect).toHaveBeenCalledWith('B1');
    expect(estados.at(-1)).toBe('buscando');
    expect(ble.scan).toHaveBeenCalledTimes(1);
    await esperar(2000); // primer reintento tras un fallo
    expect(ble.scan).toHaveBeenCalledTimes(2);
    falso.emitir('encontrado', brazalete());
    await esperar();
    expect(estados.at(-1)).toBe('conectado');
  });

  it('una desconexión durante la conexión cuenta como fallo, sin esperar los 10 s', async () => {
    const { falso, ble, cliente, estados, registro } = preparar();
    ble.retrieveServices.mockImplementationOnce(() => new Promise(() => {}));
    await cliente.iniciar();
    await esperar();
    falso.emitir('encontrado', brazalete());
    await esperar();
    falso.emitir('desconectado', { peripheral: 'B1' });
    await esperar();
    expect(registro).toContain('Falló la conexión: el brazalete cortó la conexión');
    expect(estados.at(-1)).toBe('buscando');
  });

  it('al perder la conexión vuelve a buscar enseguida', async () => {
    const { falso, ble, cliente, estados } = preparar();
    await cliente.iniciar();
    await esperar();
    falso.emitir('encontrado', brazalete());
    await esperar();
    falso.emitir('desconectado', { peripheral: 'B1' });
    await esperar();
    expect(estados.slice(-2)).toEqual(['conectado', 'buscando']);
    expect(ble.scan).toHaveBeenCalledTimes(2);
  });

  it('ignora desconexiones de otros equipos', async () => {
    const { falso, cliente, estados } = preparar();
    await cliente.iniciar();
    await esperar();
    falso.emitir('encontrado', brazalete());
    await esperar();
    falso.emitir('desconectado', { peripheral: 'OTRO' });
    expect(estados.at(-1)).toBe('conectado');
  });

  it('con el Bluetooth apagado olvida la conexión', async () => {
    const { falso, ble, cliente, estados } = preparar();
    await cliente.iniciar();
    await esperar();
    falso.emitir('encontrado', brazalete());
    await esperar();
    falso.emitir('estado', { state: 'off' });
    expect(estados.at(-1)).toBe('bluetooth-apagado');
    await expect(cliente.enviarComando('TEST_ALERT')).rejects.toThrow('no está conectado');
    falso.emitir('estado', { state: 'on' });
    await esperar();
    expect(ble.scan).toHaveBeenCalledTimes(2);
  });
});

describe('conexión que quedó abierta', () => {
  it('reutiliza el brazalete que el sistema sigue teniendo conectado, sin buscar', async () => {
    const { ble, cliente, estados } = preparar();
    ble.getConnectedPeripherals.mockResolvedValue([brazalete('Z')]);
    await cliente.iniciar();
    await esperar();
    expect(ble.getConnectedPeripherals).toHaveBeenCalledWith([UUID_SERVICIO]);
    expect(ble.connect).toHaveBeenCalledWith('Z');
    expect(ble.scan).not.toHaveBeenCalled();
    expect(estados).toEqual(['conectando', 'conectado']);
  });

  it('en iOS pregunta otra vez: la librería sólo los devuelve en la segunda llamada', async () => {
    const { ble, cliente } = preparar();
    ble.getConnectedPeripherals.mockResolvedValueOnce([]).mockResolvedValueOnce([brazalete('Z')]);
    await cliente.iniciar();
    await esperar();
    expect(ble.connect).toHaveBeenCalledWith('Z');
  });

  it('en Android no toca otros equipos conectados (reloj, auriculares)', async () => {
    const { ble, cliente } = preparar({ plataforma: 'android' });
    ble.getConnectedPeripherals.mockResolvedValue([brazalete('W', { name: 'Galaxy Watch', localName: undefined })]);
    await cliente.iniciar();
    await esperar();
    expect(ble.connect).not.toHaveBeenCalled();
    expect(ble.disconnect).not.toHaveBeenCalled();
    expect(ble.scan).toHaveBeenCalledTimes(1);
  });

  it('cierra las conexiones sobrantes', async () => {
    const { ble, cliente } = preparar();
    ble.getConnectedPeripherals.mockResolvedValue([brazalete('Z1'), brazalete('Z2')]);
    await cliente.iniciar();
    await esperar();
    expect(ble.disconnect).toHaveBeenCalledWith('Z2');
    expect(ble.connect).toHaveBeenCalledTimes(1);
    expect(ble.connect).toHaveBeenCalledWith('Z1');
  });
});

describe('búsqueda', () => {
  it('encadena búsquedas sin esperar cuando terminan sin encontrar nada', async () => {
    const { falso, ble, cliente } = preparar();
    await cliente.iniciar();
    await esperar();
    for (let i = 0; i < 3; i++) {
      falso.emitir('fin-busqueda', { status: 10 });
      await esperar();
    }
    expect(ble.scan).toHaveBeenCalledTimes(4);
  });

  it('en Android respeta 5 búsquedas cada 30 s', async () => {
    const { falso, ble, cliente } = preparar({ plataforma: 'android' });
    await cliente.iniciar();
    await esperar();
    for (let i = 0; i < 6; i++) {
      falso.emitir('fin-busqueda', { status: 10 });
      await esperar(100);
    }
    expect(ble.scan).toHaveBeenCalledTimes(5);
    await esperar(31_000);
    expect(ble.scan).toHaveBeenCalledTimes(6);
  });

  it('si la búsqueda falla, reintenta con espera creciente', async () => {
    const { ble, cliente, registro } = preparar();
    ble.scan.mockRejectedValueOnce(new Error('ocupado')).mockRejectedValueOnce(new Error('ocupado'));
    await cliente.iniciar();
    await esperar();
    expect(registro).toContain('No se pudo buscar: ocupado');
    await esperar(2000);
    expect(ble.scan).toHaveBeenCalledTimes(2);
    await esperar(4999);
    expect(ble.scan).toHaveBeenCalledTimes(2);
    await esperar(1);
    expect(ble.scan).toHaveBeenCalledTimes(3);
  });

  it('un error de Android al buscar también espera antes de reintentar', async () => {
    const { falso, ble, cliente } = preparar({ plataforma: 'android' });
    await cliente.iniciar();
    await esperar();
    falso.emitir('fin-busqueda', { status: 2 }); // SCAN_FAILED_APPLICATION_REGISTRATION_FAILED
    await esperar();
    expect(ble.scan).toHaveBeenCalledTimes(1);
    await esperar(2000);
    expect(ble.scan).toHaveBeenCalledTimes(2);
  });

  it('cambia a búsqueda de bajo consumo en segundo plano y vuelve a la rápida al frente', async () => {
    const { ble, cliente, cambiarPlano } = preparar({ plataforma: 'android' });
    await cliente.iniciar();
    await esperar();
    cambiarPlano(false);
    await esperar();
    expect(ble.scan).toHaveBeenLastCalledWith({ serviceUUIDs: [UUID_SERVICIO], seconds: 300, scanMode: 0 });
    cambiarPlano(true);
    await esperar();
    expect(ble.scan).toHaveBeenLastCalledWith({ serviceUUIDs: [UUID_SERVICIO], seconds: 30, scanMode: 2 });
  });

  it('al volver al frente no espera un reintento programado', async () => {
    const { ble, cliente, cambiarPlano } = preparar();
    ble.scan.mockRejectedValueOnce(new Error('ocupado'));
    await cliente.iniciar();
    await esperar();
    expect(ble.scan).toHaveBeenCalledTimes(1);
    cambiarPlano(true);
    await esperar();
    expect(ble.scan).toHaveBeenCalledTimes(2);
  });
});

describe('telemetría y comandos', () => {
  it('entrega los paquetes del brazalete conectado y descarta los inválidos sin cortar', async () => {
    const { falso, ble, cliente, paquetes, registro } = preparar();
    await cliente.iniciar();
    await esperar();
    falso.emitir('encontrado', brazalete());
    await esperar();
    const valor = (texto: string, peripheral = 'B1', characteristic = UUID_TX.toUpperCase()) =>
      falso.emitir('valor', { peripheral, characteristic, service: UUID_SERVICIO, value: textoABytes(texto) });
    valor('{"hd":10,"st":1,"ok":true,"tr":5,"bt":-1}');
    valor('{"hd":10,"st":1'); // cortado
    valor('{"hd":10,"st":1,"ok":true,"tr":5,"bt":-1}', 'OTRO');
    valor('{"hd":10,"st":1,"ok":true,"tr":5,"bt":-1}', 'B1', UUID_RX);
    expect(paquetes).toEqual(['{"hd":10,"st":1,"ok":true,"tr":5,"bt":-1}']);
    expect(registro).toContain('Paquete descartado: {"hd":10,"st":1');
    expect(ble.disconnect).not.toHaveBeenCalled();
  });

  it('escribe los comandos en RX como ASCII', async () => {
    const { falso, ble, cliente } = preparar();
    await cliente.iniciar();
    await esperar();
    await expect(cliente.enviarComando('TEST_ALERT')).rejects.toThrow();
    falso.emitir('encontrado', brazalete());
    await esperar();
    await cliente.enviarComando('VIBRATE_STOP');
    expect(ble.write).toHaveBeenCalledWith('B1', UUID_SERVICIO, UUID_RX, textoABytes('VIBRATE_STOP'));
  });
});

describe('detener', () => {
  it('cancela una conexión en curso: el cliente detenido no emite "conectado"', async () => {
    const { falso, ble, cliente, estados, registro } = preparar();
    let terminarConexion: () => void = () => {};
    ble.connect.mockImplementationOnce(
      () =>
        new Promise<void>((resolver) => {
          terminarConexion = resolver;
        }),
    );
    await cliente.iniciar();
    await esperar();
    falso.emitir('encontrado', brazalete());
    await esperar();
    await cliente.detener();
    expect(ble.disconnect).toHaveBeenCalledWith('B1');
    const lineas = registro.length;
    terminarConexion(); // la conexión nativa termina tarde
    await esperar(60_000);
    expect(estados).toEqual(['buscando', 'conectando', 'apagado']);
    expect(ble.startNotification).not.toHaveBeenCalled();
    expect(registro).toHaveLength(lineas);
  });

  it('quita todos los oyentes y no programa nada más', async () => {
    const { falso, ble, cliente, estados, hayOyentePlano } = preparar();
    ble.scan.mockRejectedValueOnce(new Error('ocupado')); // deja un reintento programado
    await cliente.iniciar();
    await esperar();
    await cliente.detener();
    expect([...falso.oyentes.values()].every((s) => s.size === 0)).toBe(true);
    expect(hayOyentePlano()).toBe(false);
    await esperar(120_000);
    expect(ble.scan).toHaveBeenCalledTimes(1);
    expect(estados.at(-1)).toBe('apagado');
  });

  it('cierra la conexión abierta', async () => {
    const { falso, ble, cliente } = preparar();
    await cliente.iniciar();
    await esperar();
    falso.emitir('encontrado', brazalete());
    await esperar();
    await cliente.detener();
    expect(ble.stopScan).toHaveBeenCalled();
    expect(ble.disconnect).toHaveBeenCalledWith('B1');
  });
});
