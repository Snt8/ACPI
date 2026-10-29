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

  it('un paquete de la versión 1 no trae los campos extendidos', () => {
    expect(parsearTelemetria('{"hd":135,"st":1,"ok":true,"tr":18,"bt":-1}')?.extendida).toBeUndefined();
  });
});

describe('parsearTelemetria, versión 2', () => {
  // Mismo ejemplo que docs/ble-protocol.md y firmware/brazalete/test/test_telemetria.cpp
  const EJEMPLO =
    '{"v":2,"n":412,"hd":21,"st":1,"ok":true,"tr":18,"bt":-1,' +
    '"id":1,"lz":1,"hs":200,"gr":-1,"br":1,"pt":4,"rx":20,"rs":-67}';

  it('lee el paquete tal como lo arma paquete_telemetria.cpp', () => {
    expect(parsearTelemetria(EJEMPLO)).toEqual({
      rumbo: 21,
      estado: 'permitido',
      orientado: true,
      segundosRestantes: 18,
      bateria: null,
      extendida: {
        secuencia: 412,
        semaforoId: 1,
        verde: true,
        rumboSemaforo: 200,
        giro: -1,
        brujula: 'valida',
        patron: 'permitido',
        paquetesSemaforo: 20,
        rssi: -67,
      },
    });
  });

  it.each([
    [0, 'sin-calibrar'],
    [1, 'valida'],
    [2, 'descartada'],
    [7, null],
  ])('br = %i es %p', (br, brujula) => {
    expect(parsearTelemetria(`{"v":2,"st":2,"br":${br}}`)?.extendida?.brujula).toBe(brujula);
  });

  it.each([
    [0, 'ninguno'],
    [1, 'detectado'],
    [2, 'girar-derecha'],
    [3, 'girar-izquierda'],
    [4, 'permitido'],
    [5, 'prohibido'],
  ])('pt = %i es "%s"', (pt, patron) => {
    expect(parsearTelemetria(`{"v":2,"st":2,"pt":${pt}}`)?.extendida?.patron).toBe(patron);
  });

  it('campos fuera de rango o sin dato quedan en null sin descartar el paquete', () => {
    const t = parsearTelemetria('{"v":2,"st":2,"n":70000,"lz":5,"hs":400,"gr":-300,"rs":0,"rx":"x"}');
    expect(t?.estado).toBe('mal-orientado');
    expect(t?.extendida).toEqual({
      secuencia: null,
      semaforoId: null,
      verde: null,
      rumboSemaforo: null,
      giro: null,
      brujula: null,
      patron: null,
      paquetesSemaforo: null,
      rssi: null,
    });
  });

  it('la luz llega aunque la persona esté mal orientada', () => {
    expect(parsearTelemetria('{"v":2,"st":2,"ok":false,"lz":0}')?.extendida?.verde).toBe(false);
  });
});

describe('conversión de bytes', () => {
  it('ida y vuelta en ASCII', () => {
    const json = '{"hd":7,"st":2}';
    expect(bytesATexto(textoABytes(json))).toBe(json);
    expect(textoABytes('TEST_ALERT')).toEqual([84, 69, 83, 84, 95, 65, 76, 69, 82, 84]);
  });
});
