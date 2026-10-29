import type { Coordenadas } from '@/semaforos/repositorio';

const RADIO_TIERRA_M = 6_371_000;
const rad = (grados: number) => (grados * Math.PI) / 180;

/** Distancia en metros entre dos puntos (fórmula del haversine) */
export function distanciaMetros(a: Coordenadas, b: Coordenadas): number {
  const dLat = rad(b.latitud - a.latitud);
  const dLon = rad(b.longitud - a.longitud);
  const h =
    Math.sin(dLat / 2) ** 2 + Math.cos(rad(a.latitud)) * Math.cos(rad(b.latitud)) * Math.sin(dLon / 2) ** 2;
  return 2 * RADIO_TIERRA_M * Math.asin(Math.min(1, Math.sqrt(h)));
}

/** Rumbo inicial de a hacia b, en grados desde el norte (0-359) */
export function rumboGrados(a: Coordenadas, b: Coordenadas): number {
  const dLon = rad(b.longitud - a.longitud);
  const y = Math.sin(dLon) * Math.cos(rad(b.latitud));
  const x =
    Math.cos(rad(a.latitud)) * Math.sin(rad(b.latitud)) -
    Math.sin(rad(a.latitud)) * Math.cos(rad(b.latitud)) * Math.cos(dLon);
  return ((Math.atan2(y, x) * 180) / Math.PI + 360) % 360;
}

const PUNTOS = ['norte', 'noreste', 'este', 'sureste', 'sur', 'suroeste', 'oeste', 'noroeste'];

export function puntoCardinal(grados: number): string {
  return PUNTOS[Math.round((((grados % 360) + 360) % 360) / 45) % 8];
}

/** "a 120 metros al norte", "a 1,5 kilómetros al sureste", "aquí mismo" */
export function describirUbicacion(desde: Coordenadas, hasta: Coordenadas): string {
  const metros = distanciaMetros(desde, hasta);
  if (metros < 10) {
    return 'aquí mismo';
  }
  const distancia =
    metros < 1000
      ? `${Math.round(metros / 10) * 10} metros`
      : `${(metros / 1000).toFixed(1).replace('.', ',')} kilómetros`;
  return `a ${distancia} al ${puntoCardinal(rumboGrados(desde, hasta))}`;
}
