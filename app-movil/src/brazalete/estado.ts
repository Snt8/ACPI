import Storage from 'expo-sqlite/kv-store';
import { create } from 'zustand';

import type { ClienteBrazalete, EstadoConexion } from '@/ble/cliente';
import { crearClienteBle } from '@/ble/cliente-ble';
import { crearClienteSimulado } from '@/ble/cliente-simulado';
import type { Comando, Telemetria } from '@/protocolo/brazalete';

const CLAVE_SIMULADO = 'brazalete.simulado';
const MAXIMO_REGISTRO = 100;

interface EstadoBrazalete {
  conexion: EstadoConexion;
  telemetria: Telemetria | null;
  textoCrudo: string | null;
  registro: string[]; // más reciente primero, para Diagnóstico
  simulado: boolean;
}

export const useBrazalete = create<EstadoBrazalete>(() => ({
  conexion: 'apagado',
  telemetria: null,
  textoCrudo: null,
  registro: [],
  simulado: Storage.getItemSync(CLAVE_SIMULADO) === 'true',
}));

/** Agrega una línea al registro que muestra Diagnóstico */
export function registrar(mensaje: string) {
  const hora = new Date().toLocaleTimeString();
  useBrazalete.setState((s) => ({
    registro: [`${hora}  ${mensaje}`, ...s.registro].slice(0, MAXIMO_REGISTRO),
  }));
}

const eventos = {
  alCambiarConexion(conexion: EstadoConexion) {
    useBrazalete.setState(conexion === 'conectado' ? { conexion } : { conexion, telemetria: null });
  },
  alRecibirTelemetria(telemetria: Telemetria, textoCrudo: string) {
    useBrazalete.setState({ telemetria, textoCrudo });
  },
  alRegistrar: registrar,
};

let cliente: ClienteBrazalete | null = null;

/** Arranca la conexión con el brazalete (real o simulado, según el ajuste guardado) */
export async function iniciarBrazalete() {
  if (cliente) {
    return;
  }
  const crear = useBrazalete.getState().simulado ? crearClienteSimulado : crearClienteBle;
  cliente = crear(eventos);
  try {
    await cliente.iniciar();
  } catch (error) {
    registrar(`No se pudo iniciar: ${String(error)}`);
  }
}

export async function detenerBrazalete() {
  const actual = cliente;
  cliente = null;
  await actual?.detener();
}

export async function usarSimulado(simulado: boolean) {
  Storage.setItemSync(CLAVE_SIMULADO, String(simulado));
  await detenerBrazalete();
  useBrazalete.setState({ simulado, textoCrudo: null });
  await iniciarBrazalete();
}

export async function reiniciarConexion() {
  await detenerBrazalete();
  await iniciarBrazalete();
}

export async function enviarComando(comando: Comando) {
  if (!cliente) {
    throw new Error('La conexión con el brazalete no está iniciada');
  }
  await cliente.enviarComando(comando);
}
