cmake_minimum_required(VERSION 3.20)
project(slugkit-geo CXX)

include(FetchContent)
FetchContent_Declare(
    libmaxminddb
    GIT_REPOSITORY https://github.com/maxmind/libmaxminddb.git
    GIT_TAG main
)

FetchContent_MakeAvailable(libmaxminddb)

add_library(libmaxminddb ALIAS maxminddb)

set(${PROJECT_NAME}_SRC
    src/slugkit/geo/middleware.cpp
    src/slugkit/geo/context_config.cpp

    src/slugkit/geo/maxmind_db_lookup.cpp
)

set(${PROJECT_NAME}_HEADERS
    include/slugkit/geo/lookup_component_base.hpp
    include/slugkit/geo/context_config.hpp
    include/slugkit/geo/middleware.hpp

    include/slugkit/geo/maxmind_db_lookup.hpp
)

add_library(${PROJECT_NAME} STATIC ${${PROJECT_NAME}_SRC} ${${PROJECT_NAME}_HEADERS})

target_include_directories(
    ${PROJECT_NAME} 
    PUBLIC
        include
    PRIVATE
        ${libmaxminddb_SOURCE_DIR}/include
        ${libmaxminddb_BINARY_DIR}/generated
)
target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC
        userver::core
        maxminddb
)
