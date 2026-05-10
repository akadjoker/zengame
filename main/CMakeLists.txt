project(main LANGUAGES CXX)

find_package(raylib QUIET)

if(NOT raylib_FOUND)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(RAYLIB QUIET raylib)
    endif()

    find_path(RAYLIB_INCLUDE_DIR NAMES raylib.h HINTS ${RAYLIB_INCLUDE_DIRS})
    find_library(RAYLIB_LIBRARY NAMES raylib HINTS ${RAYLIB_LIBRARY_DIRS})

    if(NOT RAYLIB_INCLUDE_DIR OR NOT RAYLIB_LIBRARY)
        message(FATAL_ERROR
            "raylib nao encontrado. Instala o pacote de desenvolvimento "
            "ou define CMAKE_PREFIX_PATH/raylib_DIR.")
    endif()

    add_library(raylib INTERFACE)
    target_include_directories(raylib INTERFACE ${RAYLIB_INCLUDE_DIR})
    target_link_libraries(raylib INTERFACE ${RAYLIB_LIBRARY})
endif()

function(configure_test_target target_name source_file)
    add_executable(${target_name} ${source_file})
    target_include_directories(${target_name} PRIVATE src)
    target_link_libraries(${target_name} PRIVATE zenengine raylib)

    if(CMAKE_BUILD_TYPE MATCHES Debug)
        if(UNIX)
            target_compile_options(${target_name} PRIVATE -fsanitize=address -fsanitize=undefined -fsanitize=leak -g -D_DEBUG -DVERBOSE)
            target_link_options(${target_name} PRIVATE -fsanitize=address -fsanitize=undefined -fsanitize=leak -g -D_DEBUG)
        endif()
    endif()

    if(WIN32)
        target_link_libraries(${target_name} PRIVATE Winmm.lib)
    endif()

    if(UNIX)
        target_link_libraries(${target_name} PRIVATE m)
    endif()
endfunction()

configure_test_target(test_node src/test_node.cpp)
configure_test_target(test_node2d src/test_node2d.cpp)
configure_test_target(test_sprite src/test_sprite.cpp)
configure_test_target(test_spatialhash src/test_spatialhash.cpp)
configure_test_target(test_characterbody2d src/test_characterbody2d.cpp)
configure_test_target(test_tilemap_character src/test_tilemap_character.cpp)
configure_test_target(test_polymesh src/test_polymesh.cpp)
configure_test_target(test_parallax2d src/test_parallax2d.cpp)
configure_test_target(test_particles2d src/test_particles2d.cpp)
configure_test_target(test_light2d src/test_light2d.cpp)
