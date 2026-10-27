# Compila main.cpp (que incluye ../semaforo.ino) y todos los módulos de ../src
file(GLOB_RECURSE SRCS_SEMAFORO "${CMAKE_CURRENT_LIST_DIR}/../src/*.cpp")

idf_component_register(SRCS "main.cpp" ${SRCS_SEMAFORO}
                       INCLUDE_DIRS ".")
