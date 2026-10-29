import { Stack, router, useLocalSearchParams } from 'expo-router';
import { useSQLiteContext } from 'expo-sqlite';
import { useEffect, useState } from 'react';
import { Alert, ScrollView, StyleSheet, TextInput } from 'react-native';

import { anunciar } from '@/avisos/anunciar';
import { Boton } from '@/components/boton';
import { ThemedText } from '@/components/themed-text';
import { ThemedView } from '@/components/themed-view';
import { BottomTabInset, Spacing } from '@/constants/theme';
import { useTheme } from '@/hooks/use-theme';
import { MapaSemaforos } from '@/semaforos/mapa';
import {
  actualizarSemaforo,
  crearSemaforo,
  eliminarSemaforo,
  obtenerSemaforo,
  type Coordenadas,
  type Semaforo,
} from '@/semaforos/repositorio';
import { sugerirNombre } from '@/semaforos/ubicacion';

// Id del marcador provisional de un semáforo que aún no está en la base
const ID_NUEVO = -1;

/** Alta (id = "nuevo", con latitud y longitud) o edición de un semáforo */
export default function PantallaSemaforo() {
  const db = useSQLiteContext();
  const tema = useTheme();
  const params = useLocalSearchParams<{ id: string; latitud?: string; longitud?: string }>();
  const esNuevo = params.id === 'nuevo';

  const [nombre, setNombre] = useState('');
  const [notas, setNotas] = useState('');
  const [punto, setPunto] = useState<Coordenadas | null>(null);
  const [precision, setPrecision] = useState<number | null>(null);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    let vigente = true;
    async function cargar() {
      if (esNuevo) {
        const nuevo = { latitud: Number(params.latitud), longitud: Number(params.longitud) };
        setPunto(nuevo);
        const sugerido = await sugerirNombre(nuevo);
        if (vigente) setNombre((actual) => actual || sugerido);
        return;
      }
      const semaforo = await obtenerSemaforo(db, Number(params.id));
      if (!vigente) return;
      if (!semaforo) {
        setError('Este semáforo ya no existe');
        return;
      }
      setNombre(semaforo.nombre);
      setNotas(semaforo.notas);
      setPunto(semaforo);
      setPrecision(semaforo.precisionMetros);
    }
    void cargar();
    return () => {
      vigente = false;
    };
  }, [db, esNuevo, params.id, params.latitud, params.longitud]);

  async function guardar() {
    const limpio = nombre.trim();
    if (!limpio) {
      setError('Escribe un nombre para el semáforo');
      anunciar({ texto: 'Escribe un nombre para el semáforo', prioridad: 'normal' });
      return;
    }
    if (!punto) return;
    const datos = { ...punto, nombre: limpio, notas: notas.trim(), precisionMetros: precision };
    if (esNuevo) {
      await crearSemaforo(db, datos);
    } else {
      await actualizarSemaforo(db, Number(params.id), datos);
    }
    anunciar({ texto: `Guardado: ${limpio}`, prioridad: 'normal' });
    router.back();
  }

  function confirmarBorrado() {
    Alert.alert('Borrar semáforo', `¿Borrar “${nombre}”? No se puede deshacer.`, [
      { text: 'Cancelar', style: 'cancel' },
      {
        text: 'Borrar',
        style: 'destructive',
        onPress: async () => {
          await eliminarSemaforo(db, Number(params.id));
          anunciar({ texto: `Borrado: ${nombre}`, prioridad: 'normal' });
          router.back();
        },
      },
    ]);
  }

  // Al moverlo a mano en el mapa, la precisión del GPS ya no aplica
  function mover(nuevo: Coordenadas) {
    setPunto(nuevo);
    setPrecision(null);
  }

  const marcador: Semaforo | null = punto && {
    ...punto,
    id: esNuevo ? ID_NUEVO : Number(params.id),
    nombre,
    notas,
    precisionMetros: precision,
    creadoEn: 0,
  };
  const estiloCampo = [styles.campo, { color: tema.text, backgroundColor: tema.backgroundElement }];

  return (
    <ThemedView style={styles.pantalla}>
      <Stack.Screen options={{ title: esNuevo ? 'Nuevo semáforo' : 'Editar semáforo' }} />
      <ScrollView
        contentInsetAdjustmentBehavior="automatic"
        keyboardShouldPersistTaps="handled"
        contentContainerStyle={styles.contenido}>
        {error && (
          <ThemedText themeColor="peligro" accessibilityLiveRegion="assertive">
            {error}
          </ThemedText>
        )}

        <ThemedText type="smallBold" nativeID="etiqueta-nombre">
          Nombre
        </ThemedText>
        <TextInput
          style={estiloCampo}
          value={nombre}
          onChangeText={setNombre}
          accessibilityLabel="Nombre del semáforo"
          accessibilityLabelledBy="etiqueta-nombre"
          placeholder="Ej.: Esquina de la escuela"
          placeholderTextColor={tema.textSecondary}
          returnKeyType="done"
        />

        <ThemedText type="smallBold" nativeID="etiqueta-notas">
          Notas
        </ThemedText>
        <TextInput
          style={[estiloCampo, styles.notas]}
          value={notas}
          onChangeText={setNotas}
          accessibilityLabel="Notas"
          accessibilityHint="Por ejemplo, de qué lado de la calle está o si hay escalón"
          accessibilityLabelledBy="etiqueta-notas"
          placeholder="Ej.: cruce hacia el parque, hay escalón"
          placeholderTextColor={tema.textSecondary}
          multiline
        />

        {marcador && (
          <>
            <ThemedText themeColor="textSecondary">
              {precision !== null
                ? `Ubicación del GPS, precisión de ${Math.round(precision)} metros.`
                : 'Ubicación marcada en el mapa.'}{' '}
              Mantén presionado el marcador y arrástralo para corregirla.
            </ThemedText>
            <MapaSemaforos
              style={styles.mapa}
              centro={marcador}
              semaforos={[marcador]}
              editable={{ id: marcador.id, alMover: mover }}
            />
          </>
        )}

        <Boton texto="Guardar" onPress={guardar} disabled={!punto} />
        {!esNuevo && <Boton texto="Borrar semáforo" variante="peligro" onPress={confirmarBorrado} />}
      </ScrollView>
    </ThemedView>
  );
}

const styles = StyleSheet.create({
  pantalla: {
    flex: 1,
  },
  contenido: {
    padding: Spacing.three,
    paddingBottom: BottomTabInset + Spacing.four,
    gap: Spacing.three,
  },
  campo: {
    minHeight: 56,
    borderRadius: 12,
    paddingHorizontal: Spacing.three,
    fontSize: 18,
  },
  notas: {
    minHeight: 96,
    paddingTop: Spacing.three,
    textAlignVertical: 'top',
  },
  mapa: {
    height: 240,
  },
});
