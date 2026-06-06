# Skill: ESP32 Sensor Fusion & Pedestrian Orientation (MPU6050 + QMC5883L)

## Metadata
- **Name:** esp32-sensor-driver
- **Description:** Encapsulates professional drivers, noise filtering, and mathematical fusion algorithms for the MPU6050 accelerometer/gyroscope and QMC5883L digital compass over I2C. Use this skill when asked to initialize sensors, track pedestrian orientation, detect walking steps, or compute heading vectors locally on the ESP32-C3 wristband.

## 1. Context & Hardware Mapping
The wristband (ESP32-C3 Super Mini) uses a shared I2C bus to poll two specific sensors:
- **MPU6050:** 6-axis IMU (Accelerometer + Gyroscope). Default I2C Address: `0x68`.
- **QMC5883L:** 3-axis Digital Compass/Magnetometer. Default I2C Address: `0x0D`.

## 2. Mandatory Low-Pass & Fusion Implementation
When generating code for orientation tracking, the agent must avoid raw value assignment due to environmental noise. You must implement the following math strategies:
- **Complementary Filter:** Combine the high-frequency response of the gyroscope with the long-term stability of the accelerometer to calculate pitch and roll without drift.
  $$\text{Angle} = \alpha \times (\text{Angle} + \text{GyroData} \times dt) + (1 - \alpha) \times \text{AccelData}$$
  *(Where $\alpha$ is typically set between 0.95 and 0.98)*.
- **Tilt Compensation for Heading:** The QMC5883L heading calculation depends on the board being perfectly flat. The agent must use the Roll ($R$) and Pitch ($P$) derived from the MPU6050 to compensate the Magnetometer vectors ($X_m, Y_m, Z_m$) before calculating the final azimuth (`yaw` / `heading`).
  $$X_h = X_m \cos(P) + Z_m \sin(P)$$
  $$Y_h = X_m \sin(R) \sin(P) + Y_m \cos(R) - Z_m \sin(R) \cos(P)$$
  $$\text{Heading} = \dots_2(Y_h, X_h)$$

## 3. Code Generation Template Structure
Every time the user asks to implement or update the sensor pipeline, structure the output using clean C++ abstractions under the `src/sensors/` directory.

### Header Abstract (`SensorManager.h`)
```cpp
#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Wire.h>

enum WristbandOrientation {
    CROSSING_FORWARD,
    DEVIATING_LEFT,
    DEVIATING_RIGHT,
    UNKNOWN
};

class SensorManager {
private:
    float roll, pitch, heading;
    const float alpha = 0.96; // Filter coefficient
    unsigned long lastUpdate;

public:
    bool begin();
    void update();
    float getHeading() const { return heading; }
    WristbandOrientation getPedestrianDirection() const;
};

#endif

```

## 4. Operational Guardrails

* **Calibration Subroutine:** Always include a non-blocking calibration routine for the QMC5883L (hard-iron offsets subtraction) that can be triggered at boot or via a BLE command.
* **I2C Bus Recovery:** If a sensor fails to respond (`EndTransmission != 0`), do not freeze the firmware. Implement an automatic I2C bus soft reset using `Wire.end()` and `Wire.begin()` before retrying.
* **Interrupts for Step Detection:** If requested to implement pedometer features, utilize the MPU6050's hardware interrupt pin rather than continuous polling loops