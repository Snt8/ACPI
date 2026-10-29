import { router } from 'expo-router';
import { useSQLiteContext } from 'expo-sqlite';
import { ScrollView, StyleSheet } from 'react-native';

import { useBrazalete, usarSimulado } from '@/brazalete/estado';
import { Boton } from '@/components/boton';
import { FilaInterruptor } from '@/components/fila-interruptor';
import { ThemedView } from '@/components/themed-view';
import { BottomTabInset, Spacing } from '@/constants/theme';
import { activarTelemetria, useEstadoTelemetria } from '@/telemetria/cola';

export default function PantallaAjustes() {
  const db = useSQLiteContext();
  const simulado = useBrazalete((s) => s.simulado);
  const telemetriaActiva = useEstadoTelemetria((s) => s.activa);

  return (
    <ThemedView style={styles.pantalla}>
      <ScrollView contentInsetAdjustmentBehavior="automatic" contentContainerStyle={styles.contenido}>
        <FilaInterruptor
          titulo="Brazalete simulado"
          descripcion="Reproduce un cruce de prueba sin el brazalete real."
          pista="Reproduce un cruce de prueba sin usar el brazalete real"
          activo={simulado}
          alCambiar={(valor) => void usarSimulado(valor)}
        />

        <FilaInterruptor
          titulo="Compartir cruces con ACPI"
          descripcion="Envía al equipo de ACPI la ubicación de cada semáforo y cuánto duró el cruce. No incluye tu nombre."
          pista="Al desactivarlo también se borra lo que faltaba por enviar"
          activo={telemetriaActiva}
          alCambiar={(valor) => void activarTelemetria(db, valor)}
        />

        <Boton
          texto="Diagnóstico (para técnicos)"
          variante="secundario"
          pista="Datos crudos del brazalete, pruebas de vibración y envío de telemetría"
          onPress={() => router.push('/ajustes/diagnostico')}
        />
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
    paddingBottom: BottomTabInset + Spacing.three,
    gap: Spacing.four,
  },
});
