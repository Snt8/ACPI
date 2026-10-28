"""Figura 1 del informe de Infomatrix: arquitectura del sistema ACPI.

Uso (desde la raíz del repo):  python docs/figuras/generar_figura_arquitectura.py docs/figuras
Genera figura-1-arquitectura.png (300 dpi, para Word) y .svg. Requiere matplotlib.
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
GRIS = "#6b7280"
COLORES = {  # relleno, borde
    "semaforo": ("#fde8e8", "#c0392b"),
    "brazalete": ("#e6f0fb", "#1f5fa8"),
    "app": ("#e8f6ee", "#1e8449"),
    "web": ("#f3ecfa", "#6c3483"),
    "persona": ("#fff6e0", "#b9770e"),
}

plt.rcParams["font.family"] = "DejaVu Sans"
fig, ax = plt.subplots(figsize=(13.33, 6.0))
ax.set_xlim(0, 100)
ax.set_ylim(0, 45)
ax.axis("off")


def nodo(x, w, y, h, clave, titulo, lineas, tam=10):
    relleno, borde = COLORES[clave]
    ax.add_patch(FancyBboxPatch((x, y), w, h, boxstyle="round,pad=0.3,rounding_size=1.4",
                                fc=relleno, ec=borde, lw=2.2))
    ax.text(x + w / 2, y + h - 1.6, titulo, ha="center", va="top", fontsize=13.5,
            fontweight="bold", color=borde)
    ax.text(x + w / 2, y + h - 5.6, "\n".join(lineas), ha="center", va="top",
            fontsize=tam, color=TINTA, linespacing=1.5)


def enlace(x0, x1, y, color, texto):
    ax.add_patch(FancyArrowPatch((x0 + 0.6, y), (x1 - 0.6, y), arrowstyle="-|>",
                                 mutation_scale=20, lw=2.4, color=color))
    ax.text((x0 + x1) / 2, y + 1.4, texto, ha="center", va="bottom", fontsize=9.2,
            color=TINTA, linespacing=1.35)


# Fila principal: cajas con espacio entre ellas para las etiquetas de los enlaces
Y, H = 27, 15
cajas = {
    "semaforo": (1.0, 15.5),
    "brazalete": (28.8, 17.5),
    "app": (58.6, 15.0),
    "web": (85.9, 13.1),
}
nodo(*cajas["semaforo"], Y, H, "semaforo", "Nodo semáforo",
     ["ESP32 DevKit V1", "Magnetómetro", "LED rojo y verde", "(cambian cada 25 s)"])
nodo(*cajas["brazalete"], Y, H, "brazalete", "Brazalete ACPI",
     ["ESP32-C3 Super Mini", "Brújula: MPU6500", "+ QMC5883L", "2 motores vibradores"])
nodo(*cajas["app"], Y, H, "app", "App móvil",
     ["Celular del usuario", "iOS y Android", "Estado del cruce", "en tiempo real"])
nodo(*cajas["web"], Y, H, "web", "ACPI Web",
     ["Panel de", "telemetría:", "historial", "de cruces"])

def borde_der(c): return cajas[c][0] + cajas[c][1] + 0.3
def borde_izq(c): return cajas[c][0] - 0.3

YE = Y + 6.5
enlace(borde_der("semaforo"), borde_izq("brazalete"), YE, COLORES["semaforo"][1],
       "ESP-NOW\n2 veces/s:\norientación,\nluz y tiempo")
enlace(borde_der("brazalete"), borde_izq("app"), YE, COLORES["brazalete"][1],
       "Bluetooth\n(BLE):\nrumbo y estado\ndel cruce")
enlace(borde_der("app"), borde_izq("web"), YE, COLORES["app"][1],
       "Internet:\nmétricas de\ncada cruce")

# Persona, debajo del brazalete
xb, wb = cajas["brazalete"]
nodo(xb, wb, 2.0, 11.5, "persona", "Peatón",
     ["con discapacidad visual,", "lleva el brazalete", "en la muñeca"])
xc = xb + wb / 2
ax.add_patch(FancyArrowPatch((xc, Y - 0.5), (xc, 14.0), arrowstyle="-|>",
                             mutation_scale=20, lw=2.4, color=COLORES["brazalete"][1]))
ax.text(xc + 1.6, 20.2, "Vibraciones", ha="left", va="center", fontsize=10.5,
        fontweight="bold", color=COLORES["brazalete"][1])

# Nota a la izquierda, debajo del semáforo
ax.text(1.0, 14.5,
        "El semáforo, al otro lado de la calle,\n"
        "mira hacia el peatón. El brazalete\n"
        "compara su orientación con la del\n"
        "semáforo para saber si la persona\n"
        "está de frente al cruce.",
        ha="left", va="top", fontsize=9.6, color=GRIS, linespacing=1.5, style="italic")

# Patrones a la derecha
ax.text(58.6, 14.5, "Patrones de vibración", ha="left", va="top", fontsize=11.5,
        fontweight="bold", color=TINTA)
ax.text(58.6, 11.4,
        "1  Semáforo detectado: 3 pulsos cortos\n"
        "2  Girar: 1 pulso del lado hacia el que debe girar\n"
        "3  Cruce permitido: 2 pulsos de 2 s\n"
        "4  Cruce prohibido: 1 pulso de 2 s",
        ha="left", va="top", fontsize=10, color=TINTA, linespacing=1.6)

fig.tight_layout(pad=0.3)
for ext in ("png", "svg"):
    fig.savefig(salida / f"figura-1-arquitectura.{ext}", dpi=300, facecolor="white")
print("listo")
