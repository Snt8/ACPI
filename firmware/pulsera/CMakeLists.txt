cmake_minimum_required(VERSION 3.20)

project(pulsera
	VERSION 0.1.0
	LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compile commands for IDEs" FORCE)

option(PULSERA_USE_STUB_ARDUINO
	"Use local Arduino/ESP32 stub headers so the project can be configured without the real toolchain."
	ON
)

set(PULSERA_ARDUINO_INCLUDE_DIR "" CACHE PATH "Path to the real Arduino include directory (optional).")
set(PULSERA_ESP32_INCLUDE_DIR "" CACHE PATH "Path to the real ESP32 include directory (optional).")

file(GLOB_RECURSE PULSERA_SOURCES CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/lib/*.cpp"
)

file(GLOB_RECURSE PULSERA_HEADERS CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/lib/*.h"
)

add_library(pulsera_firmware STATIC)

target_sources(pulsera_firmware
	PRIVATE
		${PULSERA_SOURCES}
	PUBLIC
		${PULSERA_HEADERS}
)

target_compile_definitions(pulsera_firmware
	PUBLIC
		PROJECT_PULSERA=1
)

target_compile_features(pulsera_firmware PUBLIC cxx_std_17)

target_include_directories(pulsera_firmware
	PUBLIC
		"${CMAKE_CURRENT_SOURCE_DIR}/src"
		"${CMAKE_CURRENT_SOURCE_DIR}/lib"
		"${CMAKE_CURRENT_SOURCE_DIR}/lib/brujula"
		"${CMAKE_CURRENT_SOURCE_DIR}/lib/comunicacion"
		"${CMAKE_CURRENT_SOURCE_DIR}/lib/comunicacion/movil"
		"${CMAKE_CURRENT_SOURCE_DIR}/lib/comunicacion/semaforo"
		"${CMAKE_CURRENT_SOURCE_DIR}/lib/motores"
		"${CMAKE_CURRENT_SOURCE_DIR}/lib/utils"
)

if(PULSERA_ARDUINO_INCLUDE_DIR)
	target_include_directories(pulsera_firmware SYSTEM PUBLIC "${PULSERA_ARDUINO_INCLUDE_DIR}")
endif()

if(PULSERA_ESP32_INCLUDE_DIR)
	target_include_directories(pulsera_firmware SYSTEM PUBLIC "${PULSERA_ESP32_INCLUDE_DIR}")
endif()

if(PULSERA_USE_STUB_ARDUINO)
	target_include_directories(pulsera_firmware SYSTEM PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/cmake/stubs")
endif()

if(MSVC)
	target_compile_options(pulsera_firmware PRIVATE /W4)
else()
	target_compile_options(pulsera_firmware PRIVATE -Wall -Wextra -Wpedantic)
endif()

