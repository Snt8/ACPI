# Compila main.cpp (que incluye ../brazalete.ino) y todos los módulos de ../src
file(GLOB_RECURSE SRCS_BRAZALETE "${CMAKE_CURRENT_LIST_DIR}/../src/*.cpp")

idf_component_register(SRCS "main.cpp" ${SRCS_BRAZALETE}
                       INCLUDE_DIRS ".")
