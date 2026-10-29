import { Stack } from 'expo-router';

export default function SemaforosLayout() {
  return (
    <Stack>
      <Stack.Screen name="index" options={{ title: 'Semáforos' }} />
      <Stack.Screen name="[id]" options={{ title: 'Semáforo' }} />
    </Stack>
  );
}
