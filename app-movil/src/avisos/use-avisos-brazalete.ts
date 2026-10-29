import { useEffect } from 'react';

import { anunciar } from '@/avisos/anunciar';
import { detectarAviso } from '@/avisos/frases';
import { useBrazalete } from '@/brazalete/estado';

/** Anuncia por voz los cambios importantes del brazalete mientras la app está montada */
export function useAvisosBrazalete() {
  useEffect(
    () =>
      useBrazalete.subscribe((actual, anterior) => {
        const aviso = detectarAviso(anterior, actual);
        if (aviso) {
          anunciar(aviso);
        }
      }),
    [],
  );
}
