import { Stack } from 'expo-router';

export default function AjustesLayout() {
  return (
    <Stack>
      <Stack.Screen name="index" options={{ title: 'Ajustes' }} />
      <Stack.Screen name="diagnostico" options={{ title: 'Diagnóstico' }} />
    </Stack>
  );
}
