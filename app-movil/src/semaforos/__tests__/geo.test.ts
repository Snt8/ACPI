import { describirUbicacion, distanciaMetros, puntoCardinal, rumboGrados } from '@/semaforos/geo';

const origen = { latitud: 19.4326, longitud: -99.1332 };
// 0,001° de latitud ≈ 111 m
const alNorte = { latitud: 19.4336, longitud: -99.1332 };
const alEste = { latitud: 19.4326, longitud: -99.1322 };

describe('geo', () => {
  it('mide distancias cortas con error de pocos metros', () => {
    expect(distanciaMetros(origen, alNorte)).toBeCloseTo(111.2, 0);
    expect(distanciaMetros(origen, origen)).toBe(0);
  });

  it('calcula el rumbo hacia el destino', () => {
    expect(rumboGrados(origen, alNorte)).toBeCloseTo(0, 0);
    expect(rumboGrados(origen, alEste)).toBeCloseTo(90, 0);
    expect(rumboGrados(alNorte, origen)).toBeCloseTo(180, 0);
  });

  it.each([
    [0, 'norte'],
    [44, 'noreste'],
    [90, 'este'],
    [200, 'sur'],
    [337, 'noroeste'],
    [359, 'norte'],
    [-90, 'oeste'],
  ])('%i° es %s', (grados, punto) => {
    expect(puntoCardinal(grados)).toBe(punto);
  });

  it('describe la ubicación en palabras', () => {
    expect(describirUbicacion(origen, alNorte)).toBe('a 110 metros al norte');
    expect(describirUbicacion(origen, { latitud: 19.4326, longitud: -99.1190 })).toBe('a 1,5 kilómetros al este');
    expect(describirUbicacion(origen, { latitud: 19.43263, longitud: -99.1332 })).toBe('aquí mismo');
  });
});
