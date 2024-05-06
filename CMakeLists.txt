cmake_minimum_required(VERSION 3.6)
project(MyPistacheProject)

# Używamy find_package do znalezienia pakietu Pistache
find_package(PkgConfig REQUIRED)
pkg_check_modules(Pistache REQUIRED IMPORTED_TARGET libpistache)

# Dodajemy plik źródłowy główny projektu
add_executable(${PROJECT_NAME} main.cpp
        server.h
        globals.h
)

# Linkujemy bibliotekę Pistache do naszego projektu
target_link_libraries(${PROJECT_NAME} PkgConfig::Pistache)

# Wypisujemy informacje o konfiguracji
message("Using Pistache version: ${Pistache_VERSION}")
