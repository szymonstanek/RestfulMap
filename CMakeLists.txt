cmake_minimum_required(VERSION 3.6)
project(RestfulMap)

find_package(PkgConfig REQUIRED)
pkg_check_modules(Pistache REQUIRED IMPORTED_TARGET libpistache)

add_executable(${PROJECT_NAME}
        main.cpp
        server.h
        globals.h
)

target_link_libraries(${PROJECT_NAME} PkgConfig::Pistache)

message("Using Pistache version: ${Pistache_VERSION}")
