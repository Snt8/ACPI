import { parsearTelemetria, type EstadoCruce, type Telemetria } from '@/protocolo/brazalete';
import type { Posicion } from '@/semaforos/ubicacion';
import { conPaquete, conPosicion, nuevaSesion, resumir, type SesionCruce } from '@/telemetria/sesion';

const paquete = (estado: EstadoCruce, rumbo = 0, segundosRestantes = 20): Telemetria => ({
  rumbo,
  estado,
  orientado: estado !== 'mal-orientado',
  segundosRestantes,
  bateria: null,
});
// 0,00001° de latitud ≈ 1,11 m
const posicion = (metrosNorte: number, metrosEste: number, precisionMetros: number | null = 5): Posicion => ({
  latitud: 4.6 + metrosNorte / 111_195,
  longitud: -74.08 + metrosEste / (111_195 * Math.cos((4.6 * Math.PI) / 180)),
  precisionMetros,
});
const s = (segundo: number) => segundo * 1000;

describe('sesión de cruce', () => {
  it('ubica el semáforo con la lectura más precisa de la espera', () => {
    let sesion = nuevaSesion('a', 0);
    sesion = conPosicion(sesion, posicion(3, 0, 30), s(1));
    sesion = conPosicion(sesion, posicion(0, 0, 4), s(5));
    sesion = conPosicion(sesion, posicion(1, 1, 12), s(9));
    expect(sesion.esquina?.precisionMetros).toBe(4);
  });

  it('mide la espera y el cruce hasta llegar al otro lado', () => {
    let sesion: SesionCruce = nuevaSesion('a', 0);
    sesion = conPosicion(sesion, posicion(0, 0), s(1));
    sesion = conPaquete(sesion, paquete('prohibido'), s(2));
    sesion = conPaquete(sesion, paquete('permitido', 0, 25), s(20)); // verde, cruza hacia el norte
    sesion = conPaquete(sesion, paquete('permitido', 0, 24), s(21));
    sesion = conPosicion(sesion, posicion(6, 0), s(25)); // a mitad de calle
    sesion = conPosicion(sesion, posicion(13, 0), s(32)); // otro lado
    sesion = conPosicion(sesion, posicion(30, 0), s(40)); // sigue caminando: no cambia el fin

    expect(resumir(sesion, s(60))).toMatchObject({
      espera_s: 20,
      cruzo: true,
      duracion_cruce_s: 12,
      verde_restante_s: 25,
      rumbo_cruce: 0,
      duracion_sesion_s: 60,
      precision_m: 5,
    });
  });

  it('no cuenta como cruce alejarse por la misma acera', () => {
    let sesion = nuevaSesion('a', 0);
    sesion = conPosicion(sesion, posicion(0, 0), s(1));
    sesion = conPaquete(sesion, paquete('permitido', 0), s(10));
    sesion = conPosicion(sesion, posicion(0, 20), s(20)); // 20 m al este, cruzaba al norte
    expect(resumir(sesion, s(30)).cruzo).toBe(false);
    expect(resumir(sesion, s(30)).duracion_cruce_s).toBeNull();
  });

  it('ignora lecturas del GPS poco precisas', () => {
    let sesion = nuevaSesion('a', 0);
    sesion = conPosicion(sesion, posicion(0, 0), s(1));
    sesion = conPaquete(sesion, paquete('permitido', 0), s(10));
    sesion = conPosicion(sesion, posicion(15, 0, 40), s(12));
    expect(sesion.finCruce).toBeNull();
  });

  it('sin verde no hay espera ni cruce, y cuenta las veces mal orientado', () => {
    let sesion = nuevaSesion('a', 0);
    for (const [i, estado] of (['mal-orientado', 'mal-orientado', 'prohibido', 'mal-orientado'] as const).entries()) {
      sesion = conPaquete(sesion, paquete(estado), s(i));
    }
    expect(resumir(sesion, s(10))).toMatchObject({
      lat: null,
      espera_s: null,
      cruzo: false,
      veces_mal_orientado: 2,
    });
  });
});

describe('métricas de los enlaces (telemetría extendida)', () => {
  const v2 = (n: number, extra: Partial<Record<string, number>> = {}) =>
    parsearTelemetria(JSON.stringify({ v: 2, n, hd: 0, st: 0, ok: true, tr: 10, bt: -1, id: 3, br: 1, ...extra }))!;

  it('cuenta los paquetes BLE perdidos por los saltos de la secuencia', () => {
    let sesion = nuevaSesion('a', 0);
    for (const n of [10, 11, 12, 15, 16, 20]) {
      sesion = conPaquete(sesion, v2(n), s(n));
    }
    const r = resumir(sesion, s(30));
    expect(r.paquetes_ble).toBe(6);
    expect(r.paquetes_ble_perdidos).toBe(5); // faltaron 13, 14, 17, 18, 19
    expect(r.semaforo_ble_id).toBe(3);
  });

  it('la secuencia da la vuelta en 65535 sin contar perdidos', () => {
    let sesion = nuevaSesion('a', 0);
    sesion = conPaquete(sesion, v2(65534), 0);
    sesion = conPaquete(sesion, v2(65535), 1);
    sesion = conPaquete(sesion, v2(0), 2);
    expect(resumir(sesion, 3).paquetes_ble_perdidos).toBe(0);
  });

  it('un salto enorme es un reinicio del brazalete, no paquetes perdidos', () => {
    let sesion = nuevaSesion('a', 0);
    sesion = conPaquete(sesion, v2(30000), 0);
    sesion = conPaquete(sesion, v2(2), 1);
    expect(resumir(sesion, 2).paquetes_ble_perdidos).toBe(0);
  });

  it('promedia la recepción y la señal del semáforo', () => {
    let sesion = nuevaSesion('a', 0);
    sesion = conPaquete(sesion, v2(1, { rx: 20, rs: -60 }), 0);
    sesion = conPaquete(sesion, v2(2, { rx: 15, rs: -70 }), 1);
    sesion = conPaquete(sesion, v2(3, { rx: 19, rs: 0 }), 2); // 0 = sin dato de señal
    const r = resumir(sesion, 3);
    expect(r.recepcion_semaforo_pct).toBe(90); // (20 + 15 + 19) / 60
    expect(r.rssi_semaforo_medio_dbm).toBe(-65);
    expect(r.rssi_semaforo_min_dbm).toBe(-70);
    expect(r.brujula_sin_calibrar).toBe(false);
  });

  it('con firmware viejo las métricas de enlace quedan en null', () => {
    let sesion = nuevaSesion('a', 0);
    sesion = conPaquete(sesion, paquete('prohibido'), 0);
    expect(resumir(sesion, 1)).toMatchObject({
      paquetes_ble: 1,
      paquetes_ble_perdidos: null,
      recepcion_semaforo_pct: null,
      rssi_semaforo_medio_dbm: null,
      semaforo_ble_id: null,
    });
  });

  it('anota si el brazalete estaba sin calibrar', () => {
    let sesion = nuevaSesion('a', 0);
    sesion = conPaquete(sesion, v2(1, { br: 0, st: 2 }), 0);
    expect(resumir(sesion, 1).brujula_sin_calibrar).toBe(true);
  });
});
