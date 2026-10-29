import * as Location from 'expo-location';

import type { Coordenadas } from '@/semaforos/repositorio';

export interface Posicion extends Coordenadas {
  precisionMetros: number | null;
}

export class ErrorUbicacion extends Error {}

/** Posición actual con la mejor precisión disponible; pide el permiso si hace falta */
export async function obtenerPosicion(): Promise<Posicion> {
  const { granted } = await Location.requestForegroundPermissionsAsync();
  if (!granted) {
    throw new ErrorUbicacion('Falta el permiso de ubicación. Actívalo en los ajustes del teléfono');
  }
  const { coords } = await Location.getCurrentPositionAsync({ accuracy: Location.Accuracy.Highest });
  return { latitud: coords.latitude, longitud: coords.longitude, precisionMetros: coords.accuracy };
}

/**
 * Nombre sugerido para un semáforo a partir de la dirección más cercana, usando el
 * geocodificador del sistema (no consume la cuota de Google).
 */
export async function sugerirNombre({ latitud, longitud }: Coordenadas): Promise<string> {
  try {
    const [direccion] = await Location.reverseGeocodeAsync({ latitude: latitud, longitude: longitud });
    const calle = [direccion?.street, direccion?.streetNumber].filter(Boolean).join(' ');
    const nombre = calle || direccion?.name;
    if (nombre) {
      return `Semáforo en ${nombre}`;
    }
  } catch {
    // Sin red o sin resultado: se usa el nombre genérico
  }
  return `Semáforo del ${new Date().toLocaleDateString()}`;
}
