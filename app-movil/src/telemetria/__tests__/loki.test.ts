import { configLoki, cuerpoLoki, momentoNs } from '@/telemetria/loki';

describe('cuerpoLoki', () => {
  it('agrupa por tipo con las etiquetas fijas y ordena por tiempo', () => {
    const cuerpo = JSON.parse(
      cuerpoLoki([
        { tipo: 'cruce', momentoNs: momentoNs(2000), linea: '{"b":1}' },
        { tipo: 'semaforo', momentoNs: momentoNs(1500), linea: '{"s":1}' },
        { tipo: 'cruce', momentoNs: momentoNs(1000), linea: '{"a":1}' },
      ]),
    );
    expect(cuerpo).toEqual({
      streams: [
        {
          stream: { app: 'acpi', tipo: 'cruce' },
          values: [
            ['1000000000', '{"a":1}'],
            ['2000000000', '{"b":1}'],
          ],
        },
        { stream: { app: 'acpi', tipo: 'semaforo' }, values: [['1500000000', '{"s":1}']] },
      ],
    });
  });

  it('escribe el tiempo en nanosegundos sin perder precisión', () => {
    expect(momentoNs(1_793_000_000_123)).toBe('1793000000123000000');
  });
});

describe('configLoki', () => {
  // Se cambian las claves una por una: babel-preset-expo lee process.env por referencia
  afterEach(() => {
    delete process.env.EXPO_PUBLIC_LOKI_URL;
    delete process.env.EXPO_PUBLIC_LOKI_USUARIO;
    delete process.env.EXPO_PUBLIC_LOKI_TOKEN;
  });

  it('sin credenciales no hay configuración', () => {
    delete process.env.EXPO_PUBLIC_LOKI_TOKEN;
    expect(configLoki()).toBeNull();
  });

  it('completa la ruta de push si se pega la URL base', () => {
    process.env.EXPO_PUBLIC_LOKI_URL = 'https://logs-prod-024.grafana.net/';
    process.env.EXPO_PUBLIC_LOKI_USUARIO = '123456';
    process.env.EXPO_PUBLIC_LOKI_TOKEN = 'glc_x';
    expect(configLoki()?.url).toBe('https://logs-prod-024.grafana.net/loki/api/v1/push');
    process.env.EXPO_PUBLIC_LOKI_URL = 'https://logs-prod-024.grafana.net/loki/api/v1/push';
    expect(configLoki()?.url).toBe('https://logs-prod-024.grafana.net/loki/api/v1/push');
  });
});
