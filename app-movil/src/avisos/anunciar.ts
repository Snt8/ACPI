import * as Speech from 'expo-speech';
import { AccessibilityInfo, AppState } from 'react-native';

import type { Aviso } from '@/avisos/frases';

export const IDIOMA_VOZ = 'es-MX';

let lectorActivo = false;
void AccessibilityInfo.isScreenReaderEnabled().then((activo) => {
  lectorActivo = activo;
});
AccessibilityInfo.addEventListener('screenReaderChanged', (activo) => {
  lectorActivo = activo;
});

/**
 * Dice un aviso por un solo canal: con VoiceOver/TalkBack activo y la app al frente,
 * a través del lector (para no hablar encima de él); si no, con síntesis de voz.
 */
export function anunciar({ texto, prioridad }: Aviso) {
  if (lectorActivo && AppState.currentState === 'active') {
    AccessibilityInfo.announceForAccessibility(texto);
    return;
  }
  if (prioridad === 'alta') {
    void Speech.stop();
  }
  Speech.speak(texto, { language: IDIOMA_VOZ });
}
