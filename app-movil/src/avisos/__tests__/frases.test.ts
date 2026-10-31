import { detectarAviso, fraseEstado, tituloEstado, type Instantanea } from '@/avisos/frases';
import { paquetesSimulados } from '@/ble/cliente-simulado';
import { parsearTelemetria, type EstadoCruce, type Telemetria } from '@/protocolo/brazalete';

const telemetria = (estado: EstadoCruce, segundosRestantes = 10): Telemetria => ({
  rumbo: 0,
  estado,
  orientado: estado !== 'mal-orientado',
  segundosRestantes,
  bateria: null,
});
const conectado = (estado: EstadoCruce, segundos?: number): Instantanea => ({
  conexion: 'conectado',
  telemetria: telemetria(estado, segundos),
});

describe('fraseEstado', () => {
  it('describe el cruce sin dar órdenes', () => {
    expect(fraseEstado(conectado('permitido', 12))).toBe('Paso peatonal en verde, quedan 12 segundos');
    expect(fraseEstado(conectado('prohibido', 1))).toBe('Paso peatonal en rojo, quedan 1 segundo');
  });

  it('sin conexión habla de la conexión', () => {
    expect(fraseEstado({ conexion: 'buscando', telemetria: null })).toBe('Buscando el brazalete');
    expect(fraseEstado({ conexion: 'conectado', telemetria: null })).toMatch(/esperando datos/);
  });
});

describe('tituloEstado', () => {
  it('cada estado de conexión tiene su título, no siempre "Sin semáforo"', () => {
    expect(tituloEstado({ conexion: 'buscando', telemetria: null })).toBe('Sin semáforo cerca');
    expect(tituloEstado({ conexion: 'bluetooth-apagado', telemetria: null })).toBe('Bluetooth apagado');
    expect(tituloEstado({ conexion: 'sin-permisos', telemetria: null })).toBe('Falta un permiso');
    expect(tituloEstado({ conexion: 'conectando', telemetria: null })).toBe('Conectando');
    expect(tituloEstado({ conexion: 'conectado', telemetria: null })).toBe('Semáforo detectado');
  });

  it('con datos describe el cruce, sin órdenes', () => {
    expect(tituloEstado(conectado('permitido'))).toBe('Verde peatonal');
    expect(tituloEstado(conectado('mal-orientado'))).toBe('Mal orientado');
    const sinCalibrar = parsearTelemetria('{"v":2,"hd":0,"st":2,"ok":false,"tr":10,"bt":-1,"br":0}');
    expect(tituloEstado({ conexion: 'conectado', telemetria: sinCalibrar })).toBe('Brújula sin calibrar');
  });
});

describe('detectarAviso', () => {
  const buscando: Instantanea = { conexion: 'buscando', telemetria: null };

  it('avisa al conectar y al perder la conexión', () => {
    expect(detectarAviso(buscando, { conexion: 'conectado', telemetria: null })?.texto).toBe('Semáforo detectado');
    expect(detectarAviso(conectado('prohibido'), buscando)).toEqual({
      texto: 'Brazalete desconectado',
      prioridad: 'alta',
    });
  });

  it('alejarse del semáforo no se anuncia como un error', () => {
    // rx = 0: el brazalete ya no oía al semáforo y volvió a standby
    const lejos: Instantanea = {
      conexion: 'conectado',
      telemetria: parsearTelemetria('{"v":2,"hd":0,"st":0,"ok":true,"tr":3,"bt":-1,"rx":0}'),
    };
    expect(detectarAviso(lejos, buscando)).toEqual({ texto: 'Semáforo fuera de alcance', prioridad: 'normal' });
    const cerca: Instantanea = {
      conexion: 'conectado',
      telemetria: parsearTelemetria('{"v":2,"hd":0,"st":0,"ok":true,"tr":3,"bt":-1,"rx":18}'),
    };
    expect(detectarAviso(cerca, buscando)?.prioridad).toBe('alta');
  });

  it('avisa los problemas del teléfono una vez', () => {
    expect(detectarAviso(buscando, { conexion: 'no-compatible', telemetria: null })?.texto).toMatch(/no tiene Bluetooth/);
    expect(detectarAviso(buscando, { conexion: 'bluetooth-apagado', telemetria: null })?.texto).toMatch(/apagado/);
    expect(detectarAviso(buscando, { conexion: 'conectando', telemetria: null })).toBeNull();
  });

  it('avisa los cambios de luz una sola vez', () => {
    expect(detectarAviso(conectado('prohibido', 3), conectado('permitido', 25))?.texto).toBe(
      'Paso peatonal en verde, quedan 25 segundos',
    );
    // Mismo estado, sólo baja la cuenta regresiva: nada
    expect(detectarAviso(conectado('permitido', 25), conectado('permitido', 24))).toBeNull();
  });

  it('no habla por la orientación: de eso se encarga la vibración', () => {
    expect(detectarAviso(conectado('prohibido'), conectado('mal-orientado'))).toBeNull();
  });

  it('avisa una vez que la brújula no está calibrada', () => {
    const sinCalibrar: Instantanea = {
      conexion: 'conectado',
      telemetria: parsearTelemetria('{"v":2,"hd":0,"st":2,"ok":false,"tr":10,"bt":-1,"br":0}'),
    };
    expect(detectarAviso({ conexion: 'conectado', telemetria: null }, sinCalibrar)).toEqual({
      texto: 'La brújula del brazalete no está calibrada: no puede indicar hacia dónde girar',
      prioridad: 'alta',
    });
    expect(detectarAviso(sinCalibrar, sinCalibrar)).toBeNull();
    expect(fraseEstado(sinCalibrar)).toMatch(/no está calibrada/);
  });

  it('un cruce simulado completo produce los avisos esperados, sin repetir', () => {
    const paquetes = paquetesSimulados();
    let anterior: Instantanea = buscando;
    let actual: Instantanea = { conexion: 'conectado', telemetria: null };
    const avisos = [detectarAviso(anterior, actual)?.texto];
    for (let i = 0; i < 88; i++) {
      anterior = actual;
      actual = { conexion: 'conectado', telemetria: parsearTelemetria(paquetes.next().value as string) };
      const aviso = detectarAviso(anterior, actual);
      if (aviso) avisos.push(aviso.texto);
    }
    expect(avisos).toEqual([
      'Semáforo detectado',
      'Paso peatonal en rojo, quedan 21 segundos',
      'Paso peatonal en verde, quedan 25 segundos',
    ]);
  });
});
