import { router } from 'expo-router';
import { Pressable, ScrollView, StyleSheet, Switch, View } from 'react-native';

import { useBrazalete, usarSimulado } from '@/brazalete/estado';
import { Boton } from '@/components/boton';
import { ThemedText } from '@/components/themed-text';
import { ThemedView } from '@/components/themed-view';
import { BottomTabInset, Spacing } from '@/constants/theme';

export default function PantallaAjustes() {
  const simulado = useBrazalete((s) => s.simulado);

  return (
    <ThemedView style={styles.pantalla}>
      <ScrollView contentInsetAdjustmentBehavior="automatic" contentContainerStyle={styles.contenido}>
        {/* Fila completa como un solo interruptor, para que el lector lea texto y estado juntos */}
        <Pressable
          style={styles.fila}
          accessibilityRole="switch"
          accessibilityLabel="Brazalete simulado"
          accessibilityHint="Reproduce un cruce de prueba sin usar el brazalete real"
          accessibilityState={{ checked: simulado }}
          onPress={() => void usarSimulado(!simulado)}>
          <View style={styles.textoFila}>
            <ThemedText style={styles.etiqueta}>Brazalete simulado</ThemedText>
            <ThemedText themeColor="textSecondary">
              Reproduce un cruce de prueba sin el brazalete real.
            </ThemedText>
          </View>
          <Switch
            value={simulado}
            onValueChange={(valor) => void usarSimulado(valor)}
            accessibilityElementsHidden
            importantForAccessibility="no"
          />
        </Pressable>

        <Boton
          texto="Diagnóstico (para técnicos)"
          variante="secundario"
          pista="Datos crudos del brazalete y pruebas de vibración"
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
  fila: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: Spacing.three,
    minHeight: 64,
  },
  textoFila: {
    flex: 1,
    gap: Spacing.one,
  },
  etiqueta: {
    fontSize: 18,
    fontWeight: 700,
  },
});
