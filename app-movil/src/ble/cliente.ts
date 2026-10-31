import { parsearTelemetria, type Comando, type Telemetria } from '@/protocolo/brazalete';

export type EstadoConexion =
  | 'apagado' // el cliente no se inició
  | 'sin-permisos'
  | 'bluetooth-apagado'
  | 'no-compatible' // el teléfono no tiene Bluetooth de bajo consumo
  | 'buscando'
  | 'conectando'
  | 'conectado';

export interface EventosCliente {
  alCambiarConexion(estado: EstadoConexion): void;
  alRecibirTelemetria(telemetria: Telemetria, textoCrudo: string): void;
  alRegistrar(mensaje: string): void;
}

/**
 * Conexión con el brazalete. Hay dos implementaciones: la real por BLE y una
 * simulada que reproduce un cruce completo, para probar la app sin el brazalete.
 * El resto de la app sólo conoce esta interfaz.
 */
export interface ClienteBrazalete {
  iniciar(): Promise<void>;
  detener(): Promise<void>;
  enviarComando(comando: Comando): Promise<void>;
}

export type CrearCliente = (eventos: EventosCliente) => ClienteBrazalete;

/** Entrega un paquete de telemetría; si no es válido lo deja en el registro y sigue */
export function entregarPaquete(eventos: EventosCliente, texto: string) {
  const telemetria = parsearTelemetria(texto);
  if (telemetria) {
    eventos.alRecibirTelemetria(telemetria, texto);
  } else {
    eventos.alRegistrar(`Paquete descartado: ${texto}`);
  }
}
