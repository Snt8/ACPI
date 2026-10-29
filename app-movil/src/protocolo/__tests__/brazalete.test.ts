import { bytesATexto, parsearTelemetria, textoABytes } from '@/protocolo/brazalete';

describe('parsearTelemetria', () => {
  it('lee el paquete tal como lo arma movil.cpp', () => {
    expect(parsearTelemetria('{"hd":135,"st":1,"ok":true,"tr":18,"bt":-1}')).toEqual({
      rumbo: 135,
      estado: 'permitido',
      orientado: true,
      segundosRestantes: 18,
      bateria: null,
    });
  });

  it.each([
    [0, 'prohibido'],
    [1, 'permitido'],
    [2, 'mal-orientado'],
    [3, 'panico'],
    [9, 'desconocido'],
  ])('st = %i es "%s"', (st, estado) => {
    expect(parsearTelemetria(`{"hd":0,"st":${st},"ok":false,"tr":0,"bt":-1}`)?.estado).toBe(estado);
  });

  it('descarta paquetes cortados o sin estado', () => {
    expect(parsearTelemetria('{"hd":135,"st":1,"ok":tr')).toBeNull();
    expect(parsearTelemetria('{"hd":135}')).toBeNull();
    expect(parsearTelemetria('null')).toBeNull();
    expect(parsearTelemetria('')).toBeNull();
  });

  it('acota los valores fuera de rango', () => {
    const t = parsearTelemetria('{"hd":-90,"st":0,"ok":1,"tr":999,"bt":150}');
    expect(t).toEqual({ rumbo: 270, estado: 'prohibido', orientado: false, segundosRestantes: 255, bateria: null });
  });

  it('acepta la batería cuando el brazalete la mide', () => {
    expect(parsearTelemetria('{"st":0,"bt":42}')?.bateria).toBe(42);
  });
});

describe('conversión de bytes', () => {
  it('ida y vuelta en ASCII', () => {
    const json = '{"hd":7,"st":2}';
    expect(bytesATexto(textoABytes(json))).toBe(json);
    expect(textoABytes('TEST_ALERT')).toEqual([84, 69, 83, 84, 95, 65, 76, 69, 82, 84]);
  });
});
