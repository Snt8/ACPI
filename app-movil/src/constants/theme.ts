/**
 * Colores de la app en modo claro y oscuro. Todos los pares texto/fondo superan 7:1
 * de contraste, porque parte de los usuarios tiene baja visión (guía, sección 4.3).
 * El estado nunca se comunica sólo con color: siempre va acompañado de texto.
 */

import { Platform } from 'react-native';

export const Colors = {
  light: {
    text: '#000000',
    background: '#ffffff',
    backgroundElement: '#F0F0F3',
    backgroundSelected: '#E0E1E6',
    textSecondary: '#3A3D42',
    acento: '#0B4FA8',
    textoAcento: '#ffffff',
    permitido: '#0A5C2B',
    prohibido: '#9B1111',
    peligro: '#9B1111',
  },
  dark: {
    text: '#ffffff',
    background: '#000000',
    backgroundElement: '#212225',
    backgroundSelected: '#2E3135',
    textSecondary: '#D0D3D8',
    acento: '#8EC5FF',
    textoAcento: '#000000',
    permitido: '#7EE2A0',
    prohibido: '#FF9C9C',
    peligro: '#FF9C9C',
  },
} as const;

export type ThemeColor = keyof typeof Colors.light & keyof typeof Colors.dark;

export const Fonts = Platform.select({
  ios: {
    /** iOS `UIFontDescriptorSystemDesignDefault` */
    sans: 'system-ui',
    /** iOS `UIFontDescriptorSystemDesignSerif` */
    serif: 'ui-serif',
    /** iOS `UIFontDescriptorSystemDesignRounded` */
    rounded: 'ui-rounded',
    /** iOS `UIFontDescriptorSystemDesignMonospaced` */
    mono: 'ui-monospace',
  },
  default: {
    sans: 'normal',
    serif: 'serif',
    rounded: 'normal',
    mono: 'monospace',
  },
});

export const Spacing = {
  half: 2,
  one: 4,
  two: 8,
  three: 16,
  four: 24,
  five: 32,
  six: 64,
} as const;

export const BottomTabInset = Platform.select({ ios: 50, android: 80 }) ?? 0;

// Mínimo táctil: 44 pt en iOS, 48 dp en Android; los botones principales usan más
export const AlturaBoton = 64;
