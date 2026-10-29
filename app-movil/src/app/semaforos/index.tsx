import { router, useFocusEffect } from 'expo-router';
import { useSQLiteContext } from 'expo-sqlite';
import { useCallback, useMemo, useState } from 'react';
import { FlatList, Pressable, StyleSheet } from 'react-native';

import { anunciar } from '@/avisos/anunciar';
import { Boton } from '@/components/boton';
import { ThemedText } from '@/components/themed-text';
import { ThemedView } from '@/components/themed-view';
import { BottomTabInset, Spacing } from '@/constants/theme';
import { useTheme } from '@/hooks/use-theme';
import { describirUbicacion, distanciaMetros } from '@/semaforos/geo';
import { MapaSemaforos } from '@/semaforos/mapa';
import { crearSemaforo, listarSemaforos, type Coordenadas, type Semaforo } from '@/semaforos/repositorio';
import { obtenerPosicion, sugerirNombre, type Posicion } from '@/semaforos/ubicacion';

// Con más error que esto conviene corregir la posición a mano en el mapa
const PRECISION_ACEPTABLE_M = 20;

export default function PantallaSemaforos() {
  const db = useSQLiteContext();
  const tema = useTheme();
  const [semaforos, setSemaforos] = useState<Semaforo[]>([]);
  const [posicion, setPosicion] = useState<Posicion | null>(null);
  const [guardando, setGuardando] = useState(false);
  const [mensaje, setMensaje] = useState<string | null>(null);

  const cargar = useCallback(async () => {
    setSemaforos(await listarSemaforos(db));
    try {
      setPosicion(await obtenerPosicion());
    } catch {
      // Sin permiso de ubicación la lista se muestra igual, sin distancias
    }
  }, [db]);

  useFocusEffect(
    useCallback(() => {
      void cargar();
    }, [cargar]),
  );

  const ordenados = useMemo(
    () =>
      posicion
        ? [...semaforos].sort((a, b) => distanciaMetros(posicion, a) - distanciaMetros(posicion, b))
        : semaforos,
    [semaforos, posicion],
  );

  function avisar(texto: string) {
    setMensaje(texto);
    anunciar({ texto, prioridad: 'normal' });
  }

  async function guardarAqui() {
    setGuardando(true);
    try {
      const aqui = await obtenerPosicion();
      setPosicion(aqui);
      const nombre = await sugerirNombre(aqui);
      await crearSemaforo(db, { ...aqui, nombre, notas: '' });
      setSemaforos(await listarSemaforos(db));
      const precision = aqui.precisionMetros;
      avisar(
        precision !== null && precision > PRECISION_ACEPTABLE_M
          ? `Guardado: ${nombre}. La precisión del GPS fue baja, ${Math.round(precision)} metros`
          : `Guardado: ${nombre}`,
      );
    } catch (error) {
      avisar(error instanceof Error ? error.message : 'No se pudo guardar el semáforo');
    } finally {
      setGuardando(false);
    }
  }

  function abrir(semaforo: Semaforo) {
    router.push({ pathname: '/semaforos/[id]', params: { id: String(semaforo.id) } });
  }

  function marcarEnMapa({ latitud, longitud }: Coordenadas) {
    router.push({
      pathname: '/semaforos/[id]',
      params: { id: 'nuevo', latitud: String(latitud), longitud: String(longitud) },
    });
  }

  return (
    <ThemedView style={styles.pantalla}>
      <FlatList
        contentInsetAdjustmentBehavior="automatic"
        contentContainerStyle={styles.contenido}
        data={ordenados}
        keyExtractor={(s) => String(s.id)}
        ListHeaderComponent={
          <ThemedView style={styles.cabecera}>
            <Boton
              texto={guardando ? 'Guardando…' : 'Guardar semáforo aquí'}
              pista="Guarda un semáforo en tu ubicación actual. Hazlo parado junto al semáforo"
              disabled={guardando}
              onPress={guardarAqui}
            />
            {mensaje && (
              <ThemedText accessibilityLiveRegion="polite" style={styles.mensaje}>
                {mensaje}
              </ThemedText>
            )}
            {posicion && (
              <MapaSemaforos
                style={styles.mapa}
                centro={posicion}
                semaforos={semaforos}
                alMantenerPresionado={marcarEnMapa}
                alTocarSemaforo={abrir}
              />
            )}
            <ThemedText type="smallBold" accessibilityRole="header" style={styles.titulo}>
              {semaforos.length === 1 ? '1 semáforo guardado' : `${semaforos.length} semáforos guardados`}
              {posicion ? ', del más cercano al más lejano' : ''}
            </ThemedText>
          </ThemedView>
        }
        ListEmptyComponent={
          <ThemedText themeColor="textSecondary">
            Todavía no hay semáforos. Párate junto a uno y toca “Guardar semáforo aquí”, o mantén
            presionado un punto del mapa.
          </ThemedText>
        }
        renderItem={({ item }) => {
          const ubicacion = posicion ? describirUbicacion(posicion, item) : null;
          return (
            <Pressable
              accessibilityRole="button"
              accessibilityLabel={ubicacion ? `${item.nombre}, ${ubicacion}` : item.nombre}
              accessibilityHint="Abre el semáforo para cambiar su nombre o borrarlo"
              onPress={() => abrir(item)}
              style={({ pressed }) => [
                styles.fila,
                { backgroundColor: pressed ? tema.backgroundSelected : tema.backgroundElement },
              ]}>
              <ThemedText style={styles.nombre}>{item.nombre}</ThemedText>
              {ubicacion && <ThemedText themeColor="textSecondary">{ubicacion}</ThemedText>}
            </Pressable>
          );
        }}
      />
    </ThemedView>
  );
}

const styles = StyleSheet.create({
  pantalla: {
    flex: 1,
  },
  contenido: {
    padding: Spacing.three,
    paddingBottom: BottomTabInset + Spacing.three,
    gap: Spacing.two,
  },
  cabecera: {
    gap: Spacing.three,
    marginBottom: Spacing.two,
  },
  mensaje: {
    fontSize: 18,
  },
  mapa: {
    height: 260,
  },
  titulo: {
    marginTop: Spacing.two,
  },
  fila: {
    minHeight: 64,
    borderRadius: 12,
    padding: Spacing.three,
    gap: Spacing.one,
    justifyContent: 'center',
  },
  nombre: {
    fontSize: 18,
    fontWeight: 700,
  },
});
