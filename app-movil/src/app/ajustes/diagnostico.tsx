import { useState } from 'react';
import { FlatList, StyleSheet, View } from 'react-native';

import { enviarComando, reiniciarConexion, useBrazalete } from '@/brazalete/estado';
import { Boton } from '@/components/boton';
import { ThemedText } from '@/components/themed-text';
import { ThemedView } from '@/components/themed-view';
import { BottomTabInset, Spacing } from '@/constants/theme';
import type { Comando } from '@/protocolo/brazalete';

/** Equivalente al BLEDebugFragment de la app Kotlin: para el equipo, no para el uso diario */
export default function PantallaDiagnostico() {
  const conexion = useBrazalete((s) => s.conexion);
  const telemetria = useBrazalete((s) => s.telemetria);
  const textoCrudo = useBrazalete((s) => s.textoCrudo);
  const registro = useBrazalete((s) => s.registro);
  const [error, setError] = useState<string | null>(null);

  async function enviar(comando: Comando) {
    setError(null);
    try {
      await enviarComando(comando);
    } catch (e) {
      setError(e instanceof Error ? e.message : String(e));
    }
  }

  const conectado = conexion === 'conectado';
  const datos = telemetria
    ? `Rumbo ${telemetria.rumbo}°, estado ${telemetria.estado}, ` +
      `${telemetria.orientado ? 'orientado' : 'no orientado'}, ${telemetria.segundosRestantes} s, ` +
      `batería ${telemetria.bateria === null ? 'no disponible' : `${telemetria.bateria} %`}`
    : 'Sin datos';

  return (
    <ThemedView style={styles.pantalla}>
      <FlatList
        contentInsetAdjustmentBehavior="automatic"
        contentContainerStyle={styles.contenido}
        data={registro}
        keyExtractor={(linea, i) => `${i}-${linea}`}
        ListHeaderComponent={
          <View style={styles.cabecera}>
            <ThemedText accessibilityLiveRegion="polite">Conexión: {conexion}</ThemedText>
            <ThemedText>{datos}</ThemedText>
            <ThemedText type="code" accessibilityLabel={`Último paquete: ${textoCrudo ?? 'ninguno'}`}>
              {textoCrudo ?? '—'}
            </ThemedText>
            {error && (
              <ThemedText themeColor="peligro" accessibilityLiveRegion="assertive">
                {error}
              </ThemedText>
            )}
            <Boton
              texto="Probar vibración"
              pista="Envía TEST_ALERT al brazalete"
              disabled={!conectado}
              onPress={() => enviar('TEST_ALERT')}
            />
            <Boton
              texto="Detener vibración"
              pista="Envía VIBRATE_STOP al brazalete"
              variante="secundario"
              disabled={!conectado}
              onPress={() => enviar('VIBRATE_STOP')}
            />
            <Boton texto="Reiniciar conexión" variante="secundario" onPress={() => void reiniciarConexion()} />
            <ThemedText type="smallBold" accessibilityRole="header">
              Registro
            </ThemedText>
          </View>
        }
        renderItem={({ item }) => (
          <ThemedText type="small" themeColor="textSecondary">
            {item}
          </ThemedText>
        )}
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
    gap: Spacing.one,
  },
  cabecera: {
    gap: Spacing.three,
    marginBottom: Spacing.two,
  },
});
