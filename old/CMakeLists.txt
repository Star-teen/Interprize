# CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(ModelLanguageInterpreter)

#==============================================================================
# Настройки компилятора
#==============================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Опции сборки
option(BUILD_TESTS "Build internal tests" ON)
option(ENABLE_DEBUG "Enable debug symbols" OFF)

if(ENABLE_DEBUG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -Wall -Wextra")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wall")
endif()

#==============================================================================
# Директории
#==============================================================================

set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)

include_directories(${INCLUDE_DIR})

#==============================================================================
# Исходные файлы
#==============================================================================

set(SOURCES
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/token.cpp
    ${SRC_DIR}/lexer.cpp
    ${SRC_DIR}/symbol_table.cpp
    ${SRC_DIR}/poliz.cpp
    ${SRC_DIR}/parser.cpp
    ${SRC_DIR}/interpreter.cpp
)

#==============================================================================
# Исполняемый файл
#==============================================================================

add_executable(interpreter ${SOURCES})

#==============================================================================
# Установка
#==============================================================================

install(TARGETS interpreter DESTINATION bin)
install(FILES ${INCLUDE_DIR}/*.h DESTINATION include)

#==============================================================================
# Цель для запуска тестов
#==============================================================================

if(BUILD_TESTS)
    add_custom_target(test_program
        COMMAND ${CMAKE_BINARY_DIR}/interpreter --test
        DEPENDS interpreter
        COMMENT "Запуск встроенных тестов"
    )
endif()

#==============================================================================
# Вывод информации о сборке
#==============================================================================

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Source files: ${SOURCES}")