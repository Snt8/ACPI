import { useSQLiteContext } from 'expo-sqlite';
import { useState } from 'react';
import { Alert, FlatList, StyleSheet, View } from 'react-native';

import { enviarComando, reiniciarConexion, useBrazalete } from '@/brazalete/estado';
import { Boton } from '@/components/boton';
import { ThemedText } from '@/components/themed-text';
import { ThemedView } from '@/components/themed-view';
import { BottomTabInset, Spacing } from '@/constants/theme';
import type { Comando } from '@/protocolo/brazalete';
import { encolar, useEstadoTelemetria, vaciarCola } from '@/telemetria/cola';

/** Conexión, telemetría cruda, comandos de prueba y registro: para el equipo, no para el uso diario */
export default function PantallaDiagnostico() {
  const conexion = useBrazalete((s) => s.conexion);
  const telemetria = useBrazalete((s) => s.telemetria);
  const textoCrudo = useBrazalete((s) => s.textoCrudo);
  const registro = useBrazalete((s) => s.registro);
  const db = useSQLiteContext();
  const envio = useEstadoTelemetria();
  const [error, setError] = useState<string | null>(null);

  async function enviar(comando: Comando) {
    setError(null);
    try {
      await enviarComando(comando);
    } catch (e) {
      setError(e instanceof Error ? e.message : String(e));
    }
  }

  function confirmarReinicio() {
    Alert.alert(
      'Reiniciar brazalete',
      'El brazalete se reinicia y se desconecta unos segundos. La calibración de la brújula se conserva.',
      [
        { text: 'Cancelar', style: 'cancel' },
        { text: 'Reiniciar', style: 'destructive', onPress: () => void enviar('RESTART') },
      ],
    );
  }

  function confirmarRecalibracion() {
    Alert.alert(
      'Recalibrar brújula',
      'El brazalete se reinicia para calibrar (si no se completa, conserva la calibración actual). Gíralo en todas direcciones (de lado, muñeca ' +
        'arriba y abajo, boca abajo) lejos de metales hasta que vibre una vez largo. Tres vibraciones ' +
        'cortas: no se completó, inténtalo de nuevo.',
      [
        { text: 'Cancelar', style: 'cancel' },
        { text: 'Recalibrar', style: 'destructive', onPress: () => void enviar('RECALIBRATE') },
      ],
    );
  }

  function confirmarAlineacion() {
    Alert.alert(
      'Alinear con el semáforo',
      'Ponte con el brazalete exactamente de frente al semáforo y quédate quieto. Al tocar Alinear, ' +
        'esa postura pasa a contar como "de frente" y el brazalete lo guarda. Vibra 3 veces cortas si ' +
        'quedó alineado; el giro de esta pantalla debe quedar cerca de 0°.',
      [
        { text: 'Cancelar', style: 'cancel' },
        { text: 'Alinear', onPress: () => void enviar('ALIGN') },
      ],
    );
  }

  const conectado = conexion === 'conectado';
  const ext = telemetria?.extendida;
  const datos = telemetria
    ? `Rumbo ${telemetria.rumbo}°, estado ${telemetria.estado}, ` +
      `${telemetria.orientado ? 'orientado' : 'no orientado'}, ${telemetria.segundosRestantes} s, ` +
      `batería ${telemetria.bateria === null ? 'no disponible' : `${telemetria.bateria} %`}` +
      (ext
        ? `. Semáforo ${ext.semaforoId ?? '?'} a ${ext.rumboSemaforo ?? '?'}°, ` +
          `luz ${ext.verde === null ? '?' : ext.verde ? 'verde' : 'roja'}, giro ${ext.giro ?? '?'}°, ` +
          `brújula ${ext.brujula ?? '?'}, vibrando ${ext.patron ?? '?'}, ` +
          `${ext.paquetesSemaforo ?? '?'} de 20 paquetes del semáforo, ` +
          `señal ${ext.rssi === null ? '?' : `${ext.rssi} dBm`}, paquete ${ext.secuencia ?? '?'}`
        : '. Firmware sin telemetría extendida')
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
            <Boton
              texto="Reiniciar brazalete"
              pista="Envía RESTART: el firmware del brazalete arranca de nuevo"
              variante="secundario"
              disabled={!conectado}
              onPress={confirmarReinicio}
            />
            <Boton
              texto="Alinear con el semáforo"
              pista="Envía ALIGN: la postura actual pasa a contar como de frente al semáforo"
              variante="secundario"
              disabled={!conectado}
              onPress={confirmarAlineacion}
            />
            <Boton
              texto="Recalibrar brújula"
              pista="Envía RECALIBRATE: el brazalete calibra al reiniciar y, si falla, conserva la calibración actual"
              variante="secundario"
              disabled={!conectado}
              onPress={confirmarRecalibracion}
            />

            <ThemedText type="smallBold" accessibilityRole="header">
              Envío al servidor
            </ThemedText>
            <ThemedText accessibilityLiveRegion="polite">
              {!envio.configurada
                ? 'Sin configurar: falta EXPO_PUBLIC_API_URL en .env'
                : !envio.activa
                  ? 'Desactivada en Ajustes'
                  : `${envio.pendientes} eventos pendientes`}
            </ThemedText>
            {envio.ultimoEnvio && <ThemedText themeColor="textSecondary">{envio.ultimoEnvio}</ThemedText>}
            <Boton
              texto="Enviar dato de prueba"
              pista="Manda un evento de tipo prueba al servidor para comprobar la conexión"
              variante="secundario"
              disabled={!envio.activa}
              onPress={() => void encolar(db, 'prueba', { mensaje: 'Prueba desde Diagnóstico' })}
            />
            <Boton
              texto="Enviar pendientes ahora"
              variante="secundario"
              disabled={envio.pendientes === 0}
              onPress={() => void vaciarCola(db)}
            />

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
