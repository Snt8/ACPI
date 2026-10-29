import { configServidor, cuerpoEventos } from '@/telemetria/servidor';

describe('cuerpoEventos', () => {
  it('arma el lote sin volver a serializar cada evento', () => {
    const cuerpo = cuerpoEventos(['{"id":"a","tipo":"cruce"}', '{"id":"b","tipo":"semaforo"}']);
    expect(JSON.parse(cuerpo)).toEqual({
      eventos: [
        { id: 'a', tipo: 'cruce' },
        { id: 'b', tipo: 'semaforo' },
      ],
    });
  });
});

describe('configServidor', () => {
  // Se cambian las claves una por una: babel-preset-expo lee process.env por referencia
  afterEach(() => {
    delete process.env.EXPO_PUBLIC_API_URL;
    delete process.env.EXPO_PUBLIC_API_TOKEN;
  });

  it('sin URL no hay configuración', () => {
    process.env.EXPO_PUBLIC_API_TOKEN = 'x';
    expect(configServidor()).toBeNull();
  });

  it('quita la barra final y el token es opcional', () => {
    process.env.EXPO_PUBLIC_API_URL = 'https://acpi.onrender.com/';
    expect(configServidor()).toEqual({ url: 'https://acpi.onrender.com', token: '' });
  });
});
