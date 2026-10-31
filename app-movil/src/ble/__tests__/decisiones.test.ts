import type { Peripheral } from 'react-native-ble-manager';

import {
  LIMITE_BUSQUEDAS_ANDROID,
  VENTANA_LIMITE_MS,
  anotarInicio,
  esperaPorLimite,
  esperaTrasFallo,
  evaluarConectado,
  evaluarEncontrado,
  permisosAndroid,
  planBusqueda,
  reaccionAlAdaptador,
} from '@/ble/decisiones';
import { UUID_SERVICIO } from '@/protocolo/brazalete';

const periferico = (datos: { name?: string; localName?: string; serviceUUIDs?: string[] }): Peripheral => ({
  id: 'AA:BB',
  rssi: -60,
  name: datos.name,
  advertising: { localName: datos.localName, serviceUUIDs: datos.serviceUUIDs },
});

describe('reaccionAlAdaptador', () => {
  it('sólo "on" deja buscar', () => {
    expect(reaccionAlAdaptador('on')).toBe('listo');
    expect(reaccionAlAdaptador('off')).toBe('apagado');
    expect(reaccionAlAdaptador('unauthorized')).toBe('sin-permisos');
    expect(reaccionAlAdaptador('unsupported')).toBe('no-compatible');
  });

  it('los estados de paso no se anuncian como apagado', () => {
    for (const estado of ['unknown', 'resetting', 'turning_on', 'turning_off', 'algo-nuevo']) {
      expect(reaccionAlAdaptador(estado)).toBe('esperar');
    }
  });
});

describe('permisosAndroid', () => {
  it('desde Android 12 no pide la ubicación', () => {
    expect(permisosAndroid(31)).toEqual(['android.permission.BLUETOOTH_SCAN', 'android.permission.BLUETOOTH_CONNECT']);
    expect(permisosAndroid(35)).not.toContain('android.permission.ACCESS_FINE_LOCATION');
  });

  it('antes de Android 12 la búsqueda BLE exige la ubicación', () => {
    expect(permisosAndroid(30)).toEqual(['android.permission.ACCESS_FINE_LOCATION']);
  });
});

describe('evaluarEncontrado', () => {
  it('acepta el brazalete sin notas', () => {
    expect(evaluarEncontrado(periferico({ localName: 'ACPI_Brazalete', serviceUUIDs: [UUID_SERVICIO] }))).toEqual({
      aceptar: true,
      nota: null,
    });
  });

  it('prefiere el nombre del advertising al guardado por iOS', () => {
    const p = periferico({ name: 'ACPI_Pulsera', localName: 'ACPI_Brazalete' });
    expect(evaluarEncontrado(p)).toEqual({ aceptar: true, nota: null });
  });

  it('acepta otro nombre si anuncia el servicio, y lo anota', () => {
    const p = periferico({ name: 'ESP32', serviceUUIDs: [UUID_SERVICIO.toUpperCase()] });
    expect(evaluarEncontrado(p)).toEqual({ aceptar: true, nota: 'se anuncia como "ESP32"' });
  });

  it('acepta sin nombre ni servicios: el sistema ya filtró por el UUID', () => {
    expect(evaluarEncontrado(periferico({}))).toEqual({ aceptar: true, nota: 'se anuncia como sin nombre' });
  });

  it('descarta un equipo que anuncia otros servicios y otro nombre', () => {
    const v = evaluarEncontrado(periferico({ name: 'Reloj', serviceUUIDs: ['180d'] }));
    expect(v).toEqual({ aceptar: false, motivo: '"Reloj" no anuncia el servicio del brazalete' });
  });
});

describe('evaluarConectado', () => {
  it('en iOS confía en el filtro por servicio', () => {
    expect(evaluarConectado(periferico({ name: 'Cualquiera' }), 'ios').aceptar).toBe(true);
  });

  it('en Android sólo acepta los nombres del brazalete', () => {
    expect(evaluarConectado(periferico({ name: 'ACPI_Brazalete' }), 'android').aceptar).toBe(true);
    expect(evaluarConectado(periferico({ name: 'ACPI_Pulsera' }), 'android').aceptar).toBe(true);
    expect(evaluarConectado(periferico({ name: 'Galaxy Watch' }), 'android').aceptar).toBe(false);
    expect(evaluarConectado(periferico({}), 'android').aceptar).toBe(false);
  });
});

describe('planBusqueda', () => {
  it('al frente busca rápido y en tandas cortas; en segundo plano ahorra batería', () => {
    expect(planBusqueda(true)).toEqual({ segundos: 30, rapida: true });
    expect(planBusqueda(false)).toEqual({ segundos: 300, rapida: false });
  });
});

describe('límite de búsquedas de Android', () => {
  it('deja empezar 5 en la ventana y hace esperar a la sexta', () => {
    let inicios: number[] = [];
    for (let i = 0; i < LIMITE_BUSQUEDAS_ANDROID; i++) {
      expect(esperaPorLimite(inicios, 1000 * i)).toBe(0);
      inicios = anotarInicio(inicios, 1000 * i);
    }
    // La más vieja (t = 0) sale de la ventana en VENTANA_LIMITE_MS
    expect(esperaPorLimite(inicios, 5000)).toBe(VENTANA_LIMITE_MS - 5000);
    expect(esperaPorLimite(inicios, VENTANA_LIMITE_MS)).toBe(0);
  });

  it('anotarInicio descarta lo viejo', () => {
    expect(anotarInicio([0, 1000], VENTANA_LIMITE_MS + 500)).toEqual([1000, VENTANA_LIMITE_MS + 500]);
  });
});

describe('esperaTrasFallo', () => {
  it('crece y se queda en 30 s', () => {
    expect([1, 2, 3, 4, 5, 50].map(esperaTrasFallo)).toEqual([2000, 5000, 10000, 30000, 30000, 30000]);
    expect(esperaTrasFallo(0)).toBe(2000);
  });
});
