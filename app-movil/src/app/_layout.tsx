import { SQLiteProvider } from 'expo-sqlite';
import { DarkTheme, DefaultTheme, ThemeProvider } from 'expo-router';
import { useEffect } from 'react';
import { useColorScheme } from 'react-native';

import { useAvisosBrazalete } from '@/avisos/use-avisos-brazalete';
import { iniciarBrazalete } from '@/brazalete/estado';
import AppTabs from '@/components/app-tabs';
import { migrarBase } from '@/semaforos/repositorio';

export default function RootLayout() {
  const colorScheme = useColorScheme();

  // La conexión con el brazalete vive mientras la app está abierta, en cualquier pestaña
  useEffect(() => {
    void iniciarBrazalete();
  }, []);
  useAvisosBrazalete();

  return (
    <SQLiteProvider databaseName="acpi.db" onInit={migrarBase}>
      <ThemeProvider value={colorScheme === 'dark' ? DarkTheme : DefaultTheme}>
        <AppTabs />
      </ThemeProvider>
    </SQLiteProvider>
  );
}
