import type { ExpoConfig } from 'expo/config';

const config: ExpoConfig = {
  name: 'ACPI',
  slug: 'acpi',
  version: '1.0.0',
  orientation: 'portrait',
  icon: './assets/images/icon.png',
  scheme: 'acpi',
  userInterfaceStyle: 'automatic',
  ios: {
    icon: './assets/expo.icon',
    bundleIdentifier: 'com.snt8.acpi',
    supportsTablet: false,
  },
  android: {
    package: 'com.snt8.acpi',
    adaptiveIcon: {
      backgroundColor: '#E6F4FE',
      foregroundImage: './assets/images/android-icon-foreground.png',
      backgroundImage: './assets/images/android-icon-background.png',
      monochromeImage: './assets/images/android-icon-monochrome.png',
    },
    predictiveBackGestureEnabled: false,
  },
  plugins: [
    'expo-router',
    [
      'expo-splash-screen',
      {
        backgroundColor: '#208AEF',
        image: './assets/images/splash-icon.png',
        imageWidth: 76,
      },
    ],
    'expo-sqlite',
    [
      'expo-location',
      {
        locationWhenInUsePermission:
          'ACPI usa tu ubicación para guardar semáforos y decirte a qué distancia están.',
      },
    ],
    [
      'react-native-ble-manager',
      {
        bluetoothAlwaysPermission: 'ACPI usa Bluetooth para conectarse a tu brazalete.',
      },
    ],
    '@maplibre/maplibre-react-native',
  ],
  experiments: {
    typedRoutes: true,
    reactCompiler: true,
  },
};

export default config;
