import type { EstadoCruce, Telemetria } from '@/protocolo/brazalete';
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
