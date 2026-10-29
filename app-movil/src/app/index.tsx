import { StyleSheet, View } from 'react-native';
import { SafeAreaView } from 'react-native-safe-area-context';

import { anunciar } from '@/avisos/anunciar';
import { fraseEstado } from '@/avisos/frases';
import { useBrazalete } from '@/brazalete/estado';
import { Boton } from '@/components/boton';
import { ThemedText } from '@/components/themed-text';
import { ThemedView } from '@/components/themed-view';
import { BottomTabInset, Spacing, type ThemeColor } from '@/constants/theme';
import type { EstadoCruce } from '@/protocolo/brazalete';

const TITULOS: Record<EstadoCruce, string> = {
  permitido: 'Verde peatonal',
  prohibido: 'Rojo peatonal',
  'mal-orientado': 'Gira hacia el cruce',
  panico: 'Pánico',
  desconocido: 'Estado desconocido',
};

const COLORES: Partial<Record<EstadoCruce, ThemeColor>> = {
  permitido: 'permitido',
  prohibido: 'prohibido',
  panico: 'peligro',
};

export default function PantallaEstado() {
  const conexion = useBrazalete((s) => s.conexion);
  const telemetria = useBrazalete((s) => s.telemetria);
  const simulado = useBrazalete((s) => s.simulado);
  const frase = fraseEstado({ conexion, telemetria });

  const titulo = conexion !== 'conectado' ? 'Sin semáforo' : telemetria ? TITULOS[telemetria.estado] : 'Conectado';
  const colorTitulo = (telemetria && COLORES[telemetria.estado]) || 'text';
  const repetir = () => anunciar({ texto: frase, prioridad: 'alta' });

  return (
    // Toque mágico (doble toque con dos dedos, VoiceOver): repetir el estado
    <ThemedView style={styles.pantalla} onMagicTap={repetir}>
      <SafeAreaView style={styles.contenido}>
        {/* Un solo elemento para el lector: se lee como una frase y se anuncia al cambiar */}
        <ThemedView
          type="backgroundElement"
          style={styles.tarjeta}
          accessible
          accessibilityLabel={simulado ? `${frase}. Modo simulado` : frase}
          accessibilityLiveRegion="polite">
          <ThemedText type="subtitle" themeColor={colorTitulo}>
            {titulo}
          </ThemedText>
          <ThemedText style={styles.frase}>{frase}</ThemedText>
          {simulado && <ThemedText themeColor="textSecondary">Modo simulado</ThemedText>}
        </ThemedView>

        <View style={styles.acciones}>
          <Boton texto="Repetir estado" pista="Dice en voz alta el estado actual" onPress={repetir} />
        </View>
      </SafeAreaView>
    </ThemedView>
  );
}

const styles = StyleSheet.create({
  pantalla: {
    flex: 1,
  },
  contenido: {
    flex: 1,
    padding: Spacing.four,
    paddingBottom: BottomTabInset + Spacing.three,
    gap: Spacing.four,
    justifyContent: 'center',
  },
  tarjeta: {
    borderRadius: 24,
    padding: Spacing.four,
    gap: Spacing.three,
  },
  frase: {
    fontSize: 24,
    lineHeight: 32,
  },
  acciones: {
    gap: Spacing.three,
  },
});
