#!/bin/sh
# Compila y corre las pruebas del brazalete en la PC con g++ o clang++.
# Uso: sh test/ejecutar_pruebas.sh   (desde cualquier carpeta)
set -e
AQUI=$(cd "$(dirname "$0")" && pwd)
SALIDA="$AQUI/build"
CXX=${CXX:-$(command -v g++ || command -v clang++ || true)}
if [ -z "$CXX" ]; then
    echo "No se encontro g++ ni clang++ (en Windows usar ejecutar_pruebas.bat)." >&2
    exit 1
fi

mkdir -p "$SALIDA"
"$CXX" -std=c++17 -Wall -Wextra -Werror -o "$SALIDA/pruebas" \
    "$AQUI/main.cpp" \
    "$AQUI/test_controlador_patrones.cpp" \
    "$AQUI/test_evaluador_semaforo.cpp" \
    "$AQUI/test_escenarios.cpp" \
    "$AQUI/test_utilidades.cpp" \
    "$AQUI/test_ciclo_escucha.cpp" \
    "$AQUI/../src/vibracion/controlador_patrones.cpp" \
    "$AQUI/../src/control/evaluador_semaforo.cpp" \
    "$AQUI/../src/control/ciclo_escucha.cpp" \
    "$AQUI/../src/control/coordinador_cruce.cpp" \
    "$AQUI/../src/utils/calcular_direccion.cpp"

"$SALIDA/pruebas"
