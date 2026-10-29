import { Pressable, StyleSheet, Text, type PressableProps } from 'react-native';

import { AlturaBoton, Spacing } from '@/constants/theme';
import { useTheme } from '@/hooks/use-theme';

type Props = Omit<PressableProps, 'children'> & {
  texto: string;
  // Qué pasa al tocarlo, si no es obvio por el texto (lo lee el lector de pantalla)
  pista?: string;
  variante?: 'principal' | 'secundario' | 'peligro';
};

/** Botón grande con texto: nunca sólo un icono, siempre con rol y etiqueta */
export function Boton({ texto, pista, variante = 'principal', disabled, style, ...resto }: Props) {
  const tema = useTheme();
  const fondo = variante === 'principal' ? tema.acento : variante === 'peligro' ? tema.peligro : tema.backgroundElement;
  const color = variante === 'secundario' ? tema.text : tema.textoAcento;

  return (
    <Pressable
      accessibilityRole="button"
      accessibilityLabel={texto}
      accessibilityHint={pista}
      accessibilityState={{ disabled: !!disabled }}
      disabled={disabled}
      style={(estado) => [
        styles.boton,
        { backgroundColor: fondo, opacity: disabled ? 0.5 : estado.pressed ? 0.8 : 1 },
        typeof style === 'function' ? style(estado) : style,
      ]}
      {...resto}>
      <Text style={[styles.texto, { color }]}>{texto}</Text>
    </Pressable>
  );
}

const styles = StyleSheet.create({
  boton: {
    minHeight: AlturaBoton,
    borderRadius: 16,
    paddingHorizontal: Spacing.four,
    paddingVertical: Spacing.three,
    alignItems: 'center',
    justifyContent: 'center',
  },
  texto: {
    fontSize: 20,
    fontWeight: 700,
    textAlign: 'center',
  },
});
