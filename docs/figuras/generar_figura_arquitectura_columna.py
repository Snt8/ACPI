"""Figura 1 del informe de Infomatrix, versión vertical para una columna (7.2 cm).

El cuerpo del informe va en dos columnas: la versión horizontal
(generar_figura_arquitectura.py) queda para diapositivas y pósteres.

Uso (desde la raíz del repo):  python docs/figuras/generar_figura_arquitectura_columna.py docs/figuras
Genera figura-1-arquitectura-columna.png (600 dpi). Requiere matplotlib.
"""
import sys
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import FancyBboxPatch, FancyArrowPatch

salida = Path(sys.argv[1])
salida.mkdir(parents=True, exist_ok=True)

TINTA = "#1f2937"
COLORES = {
    "semaforo": ("#fde8e8", "#c0392b"),
    "brazalete": ("#e6f0fb", "#1f5fa8"),
    "app": ("#e8f6ee", "#1e8449"),
    "web": ("#f3ecfa", "#6c3483"),
    "persona": ("#fff6e0", "#b9770e"),
}

plt.rcParams["font.family"] = "DejaVu Sans"
ANCHO_CM, ALTO_CM = 7.1, 8.6
fig, ax = plt.subplots(figsize=(ANCHO_CM / 2.54, ALTO_CM / 2.54))
ax.set_xlim(0, 71)
ax.set_ylim(0, 86)
ax.axis("off")


def caja(x, y, w, h, clave, titulo, detalle):
    relleno, borde = COLORES[clave]
    ax.add_patch(FancyBboxPatch((x, y), w, h, boxstyle="round,pad=0.4,rounding_size=2",
                                fc=relleno, ec=borde, lw=1.1))
    ax.text(x + w / 2, y + h - 2.2, titulo, ha="center", va="top", fontsize=7.2,
            fontweight="bold", color=borde)
    ax.text(x + w / 2, y + h - 6.8, detalle, ha="center", va="top", fontsize=5.6,
            color=TINTA, linespacing=1.3)


def flecha_v(x, y0, y1, color, texto):
    ax.add_patch(FancyArrowPatch((x, y0), (x, y1), arrowstyle="-|>", mutation_scale=8,
                                 lw=1.2, color=color))
    ax.text(x + 2.0, (y0 + y1) / 2, texto, ha="left", va="center", fontsize=5.4,
            color=TINTA, linespacing=1.25)


X, W, H = 1.5, 35, 13
XC = X + W / 2
caja(X, 70.5, W, H, "semaforo", "Nodo semáforo", "ESP32 + magnetómetro\nLED rojo y verde (25 s)")
caja(X, 47.5, W, H, "brazalete", "Brazalete ACPI",
     "ESP32-C3, brújula\ny 2 motores vibradores")
caja(X, 24.5, W, H, "app", "App móvil", "iOS y Android\nestado en tiempo real")
caja(X, 1.5, W, H, "web", "ACPI Web", "panel de telemetría\nhistorial de cruces")

flecha_v(XC, 70.1, 60.9, COLORES["semaforo"][1], "ESP-NOW, 2 veces/s:\norientación, luz, tiempo")
flecha_v(XC, 47.1, 37.9, COLORES["brazalete"][1], "Bluetooth (BLE):\nrumbo y estado")
flecha_v(XC, 24.1, 14.9, COLORES["app"][1], "Internet: métricas\nde cada cruce")

# Peatón a la derecha del brazalete
caja(47.5, 47.5, 22, H, "persona", "Peatón", "con discapacidad\nvisual")
ax.add_patch(FancyArrowPatch((37.4, 54.0), (47.1, 54.0), arrowstyle="-|>", mutation_scale=8,
                             lw=1.2, color=COLORES["brazalete"][1]))
ax.text(42.2, 56.6, "vibra", ha="center", va="center", fontsize=5.4,
        color=COLORES["brazalete"][1], fontweight="bold")

fig.subplots_adjust(0, 0, 1, 1)
fig.savefig(salida / "figura-1-arquitectura-columna.png", dpi=600, facecolor="white")
print("listo")
