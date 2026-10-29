import { Camera, Map, NativeUserLocation, ViewAnnotation } from '@maplibre/maplibre-react-native';
import { StyleSheet, View, type StyleProp, type ViewStyle } from 'react-native';

import { Colors } from '@/constants/theme';
import type { Coordenadas, Semaforo } from '@/semaforos/repositorio';

// MapLibre (open source) con el estilo "liberty" de OpenFreeMap: datos de
// OpenStreetMap, sin clave ni cuenta. Para cambiar de servidor basta otra URL de estilo
const ESTILO_MAPA = 'https://tiles.openfreemap.org/styles/liberty';

const ZOOM_CALLE = 16; // ~400 m de alto: se distinguen las esquinas

interface Props {
  centro: Coordenadas | null;
  semaforos: Semaforo[];
  style?: StyleProp<ViewStyle>;
  // Toque largo en el mapa: marcar un semáforo en ese punto
  alMantenerPresionado?: (punto: Coordenadas) => void;
  alTocarSemaforo?: (semaforo: Semaforo) => void;
  // Si se pasa, ese semáforo se puede arrastrar para corregir su posición
  editable?: { id: number; alMover: (punto: Coordenadas) => void };
}

/**
 * Mapa para quien ve (familia, equipo, personas con baja visión). Se oculta al
 * lector de pantalla: la lista de semáforos es su equivalente accesible.
 */
export function MapaSemaforos({ centro, semaforos, style, alMantenerPresionado, alTocarSemaforo, editable }: Props) {
  return (
    <View
      style={[styles.contenedor, style]}
      accessibilityElementsHidden
      importantForAccessibility="no-hide-descendants">
      <Map
        style={StyleSheet.absoluteFill}
        mapStyle={ESTILO_MAPA}
        touchRotate={false}
        touchPitch={false}
        onLongPress={({ nativeEvent: { lngLat } }) =>
          alMantenerPresionado?.({ latitud: lngLat[1], longitud: lngLat[0] })
        }>
        <Camera
          initialViewState={centro ? { center: [centro.longitud, centro.latitud], zoom: ZOOM_CALLE } : undefined}
        />
        <NativeUserLocation />
        {semaforos.map((s) => (
          <ViewAnnotation
            key={s.id}
            id={String(s.id)}
            lngLat={[s.longitud, s.latitud]}
            title={s.nombre}
            draggable={editable?.id === s.id}
            onDragEnd={({ nativeEvent: { lngLat } }) =>
              editable?.alMover({ latitud: lngLat[1], longitud: lngLat[0] })
            }
            onPress={() => alTocarSemaforo?.(s)}>
            <View style={styles.marcador} />
          </ViewAnnotation>
        ))}
      </Map>
    </View>
  );
}

const styles = StyleSheet.create({
  contenedor: {
    overflow: 'hidden',
    borderRadius: 16,
  },
  marcador: {
    width: 24,
    height: 24,
    borderRadius: 12,
    borderWidth: 3,
    borderColor: '#ffffff',
    backgroundColor: Colors.light.prohibido,
  },
});
