import { detectarAviso, fraseEstado, type Instantanea } from '@/avisos/frases';
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

describe('detectarAviso', () => {
  const buscando: Instantanea = { conexion: 'buscando', telemetria: null };

  it('avisa al conectar y al perder la conexión', () => {
    expect(detectarAviso(buscando, { conexion: 'conectado', telemetria: null })?.texto).toBe('Semáforo detectado');
    expect(detectarAviso(conectado('prohibido'), buscando)).toEqual({
      texto: 'Brazalete desconectado',
      prioridad: 'alta',
    });
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
