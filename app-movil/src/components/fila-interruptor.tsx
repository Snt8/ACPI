import { Pressable, StyleSheet, Switch, View } from 'react-native';

import { ThemedText } from '@/components/themed-text';
import { Spacing } from '@/constants/theme';

interface Props {
  titulo: string;
  descripcion: string;
  pista?: string;
  activo: boolean;
  alCambiar: (activo: boolean) => void;
}

/** Fila completa como un solo interruptor, para que el lector lea texto y estado juntos */
export function FilaInterruptor({ titulo, descripcion, pista, activo, alCambiar }: Props) {
  return (
    <Pressable
      style={styles.fila}
      accessibilityRole="switch"
      accessibilityLabel={titulo}
      accessibilityHint={pista}
      accessibilityState={{ checked: activo }}
      onPress={() => alCambiar(!activo)}>
      <View style={styles.texto}>
        <ThemedText style={styles.titulo}>{titulo}</ThemedText>
        <ThemedText themeColor="textSecondary">{descripcion}</ThemedText>
      </View>
      <Switch
        value={activo}
        onValueChange={alCambiar}
        accessibilityElementsHidden
        importantForAccessibility="no"
      />
    </Pressable>
  );
}

const styles = StyleSheet.create({
  fila: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: Spacing.three,
    minHeight: 64,
  },
  texto: {
    flex: 1,
    gap: Spacing.one,
  },
  titulo: {
    fontSize: 18,
    fontWeight: 700,
  },
});
