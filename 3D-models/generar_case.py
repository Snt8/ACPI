"""Genera los STL del case de la pulsera ACPI (caja + tapa) de forma paramétrica.

Cierre: la tapa tiene una pestaña perimetral que entra en la caja con holgura
(HOLGURA por lado) para que entre aunque la impresora deje material de más, y
unos nervios verticales de fricción que sí invaden la pared (INTERFERENCIA).
Los nervios se aplastan un poco al cerrar y son los que sujetan la tapa.

Uso:  python generar_case.py        (requiere trimesh y manifold3d)
"""
from pathlib import Path

import numpy as np
import trimesh
from manifold3d import CrossSection, JoinType, Manifold, OpType

# ---------------------------------------------------------------- parámetros
PCB_X, PCB_Y = 48.24, 65.0    # contorno de la PCB v3.5 (Gerber_BoardOutlineLayer)
MARGEN = 2.0                  # holgura por lado entre la PCB y el muro
MURO_X, MURO_Y = 3.0, 4.0     # grosor de los muros (el de +y lleva el USB)
R_ESQ = 1.0                   # radio de las esquinas verticales exteriores
ALTO_CAJA = 15.0              # caja
ALTO_TAPA = 2.5               # tapa: total 17.5 mm, 15 mm libres por dentro
PISO = 1.0
TECHO = 1.5

CHAFLAN_BASE = 0.4            # esconde la "pata de elefante" de la 1.a capa
CHAFLAN_UNION = 0.5           # ranura en V en la unión, para abrir con la uña
CHAFLAN_ENTRADA = 0.4         # boca interior de la caja, guía la pestaña

PESTANA_GROSOR = 1.2
PESTANA_ALTO = 3.0
HOLGURA = 0.25                # por lado, entre la pestaña y la pared interior
INTERFERENCIA = 0.15          # cuánto invaden los nervios la pared interior
R_NERVIO = 0.5
NERVIOS_X = (-12.0, 12.0)     # posiciones en los muros de ±y
NERVIOS_Y = (-22.0, 0.0, 22.0)  # posiciones en los muros de ±x

# Abertura del USB-C en el muro de +y. Del tamaño de la funda del cable, porque el
# conector del ESP32 queda a MARGEN + MURO_Y del exterior y la funda tiene que entrar.
# USB_X: el USB-C del Super Mini está 0.92 mm a la izquierda del centro de la PCB
# (vista de arriba, con el borde del USB hacia +y).
USB_X, USB_ANCHO = -0.92, 12.5
USB_Z0, USB_Z1 = 4.0, 11.0

ALTO_PRUEBA = 4.5             # marco de prueba de ajuste (parte alta de la caja)
SEG = 64                      # segmentos por círculo completo
# ---------------------------------------------------------------------------

CAV_X, CAV_Y = PCB_X + 2 * MARGEN, PCB_Y + 2 * MARGEN
ANCHO, LARGO = CAV_X + 2 * MURO_X, CAV_Y + 2 * MURO_Y
R_CAV = 0.5
EPS = 0.01


def rect_redondeado(w, l, r):
    r = max(r, 0.05)
    return CrossSection.square((w - 2 * r, l - 2 * r), center=True).offset(
        r, JoinType.Round, circular_segments=SEG)


def losa(w, l, r, z0, alto):
    return rect_redondeado(w, l, r).extrude(alto).translate((0, 0, z0))


def bloque_achaflanado(w, l, r, z0, alto, ch_abajo, ch_arriba):
    """Prisma de esquinas redondeadas con chaflán de 45° abajo y arriba."""
    partes = [losa(w, l, r, z0 + ch_abajo, alto - ch_abajo - ch_arriba)]
    if ch_abajo:
        partes.append(losa(w - 2 * ch_abajo, l - 2 * ch_abajo, r - ch_abajo, z0, EPS))
    if ch_arriba:
        partes.append(losa(w - 2 * ch_arriba, l - 2 * ch_arriba, r - ch_arriba,
                           z0 + alto - EPS, EPS))
    return Manifold.batch_hull(partes)


def nervio(cx, cy, z0, z1):
    """Nervio vertical de fricción, con la punta inferior en cono para que entre."""
    cono = 0.8
    cuerpo = Manifold.cylinder(z1 - z0 - cono, R_NERVIO, circular_segments=SEG)
    punta = Manifold.cylinder(cono, 0.15, R_NERVIO, circular_segments=SEG)
    return (punta + cuerpo.translate((0, 0, cono))).translate((cx, cy, z0))


def caja():
    cuerpo = bloque_achaflanado(ANCHO, LARGO, R_ESQ, 0, ALTO_CAJA,
                                CHAFLAN_BASE, CHAFLAN_UNION)
    hueco = losa(CAV_X, CAV_Y, R_CAV, PISO, ALTO_CAJA)
    entrada = Manifold.batch_hull([
        losa(CAV_X, CAV_Y, R_CAV, ALTO_CAJA - CHAFLAN_ENTRADA, EPS),
        losa(CAV_X + 2 * CHAFLAN_ENTRADA, CAV_Y + 2 * CHAFLAN_ENTRADA,
             R_CAV + CHAFLAN_ENTRADA, ALTO_CAJA, EPS),
    ])
    usb = rect_redondeado(USB_ANCHO, USB_Z1 - USB_Z0, 1.5).extrude(MURO_Y + 2)
    usb = usb.rotate((90, 0, 0)).translate(
        (USB_X, LARGO / 2 + 1, (USB_Z0 + USB_Z1) / 2))
    return cuerpo - hueco - entrada - usb


def tapa():
    """Tapa en posición de montaje (z de ALTO_CAJA a ALTO_CAJA + ALTO_TAPA)."""
    z0, z1 = ALTO_CAJA, ALTO_CAJA + ALTO_TAPA
    px, py = CAV_X - 2 * HOLGURA, CAV_Y - 2 * HOLGURA            # cara exterior pestaña
    ix, iy = px - 2 * PESTANA_GROSOR, py - 2 * PESTANA_GROSOR      # cara interior

    cuerpo = bloque_achaflanado(ANCHO, LARGO, R_ESQ, z0, ALTO_TAPA,
                                CHAFLAN_UNION, CHAFLAN_BASE)
    zp = z0 - PESTANA_ALTO
    pestana = Manifold.batch_hull([
        losa(px - 1.0, py - 1.0, 0.5, zp, EPS),                    # chaflán de la punta
        losa(px, py, 1.0, zp + 0.5, z1 - TECHO - zp - 0.5),
    ])
    hueco = losa(ix, iy, 0.5, zp - 1, z1 - TECHO - zp + 1)

    nervios = []
    zn0 = zp + 0.6
    for y in NERVIOS_Y:
        for s in (-1, 1):
            nervios.append(nervio(s * (CAV_X / 2 + INTERFERENCIA - R_NERVIO), y, zn0, z0))
    for x in NERVIOS_X:
        for s in (-1, 1):
            nervios.append(nervio(x, s * (CAV_Y / 2 + INTERFERENCIA - R_NERVIO), zn0, z0))

    return Manifold.batch_boolean([cuerpo, pestana] + nervios,
                                  OpType.Add) - hueco


def a_trimesh(m):
    mesh = m.to_mesh()
    return trimesh.Trimesh(np.asarray(mesh.vert_properties)[:, :3],
                           np.asarray(mesh.tri_verts), process=True)


def main():
    dest = Path(__file__).resolve().parent
    c, t = caja(), tapa()

    # tapa para imprimir: boca abajo, el techo apoyado en la cama
    t_imp = t.rotate((180, 0, 0))
    zmin = t_imp.bounding_box()[2]
    t_imp = t_imp.translate((0, 0, -zmin))

    marco = c.trim_by_plane((0, 0, 1), ALTO_CAJA - ALTO_PRUEBA)
    marco = marco.translate((0, 0, -(ALTO_CAJA - ALTO_PRUEBA)))

    salidas = {
        "ACPI-Case.stl": c,
        "ACPI-Cover.stl": t_imp,
        "ACPI-Bracelet.stl": c + t,
        "ACPI-Prueba-Ajuste.stl": marco,
    }
    for nombre, m in salidas.items():
        a_trimesh(m).export(dest / nombre)
        print(f"{nombre}: {np.round(m.bounding_box(), 2)}  vol={m.volume():.0f} mm3")


if __name__ == "__main__":
    main()
