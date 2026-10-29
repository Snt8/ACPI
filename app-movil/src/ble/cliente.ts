import type { Comando, Telemetria } from '@/protocolo/brazalete';

export type EstadoConexion =
  | 'apagado' // el cliente no se inició
  | 'sin-permisos'
  | 'bluetooth-apagado'
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
