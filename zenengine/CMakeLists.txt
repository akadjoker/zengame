project(zenengine LANGUAGES CXX)

file(GLOB SOURCES CONFIGURE_DEPENDS "src/*.cpp")
add_library(zenengine STATIC ${SOURCES})
target_link_libraries(zenengine PUBLIC poly2tri)

target_include_directories(zenengine PUBLIC include src)
target_compile_definitions(zenengine PUBLIC USE_GRAPHICS)
target_compile_options(zenengine PRIVATE
    -fexceptions
    -frtti
    -fno-strict-aliasing
    -Wextra
    -Wall
    -Wno-missing-field-initializers
    -Wundef
    -Wuninitialized
    -Wunused
    -Wno-unused-parameter
    -Wwrite-strings
    -Wchar-subscripts
    -funsigned-char
)
target_precompile_headers(zenengine PRIVATE src/pch.hpp)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_options(zenengine PRIVATE -fsanitize=address -fsanitize=undefined -fsanitize=leak -g -Winvalid-pch -D_DEBUG)
    target_link_options(zenengine PRIVATE -fsanitize=address -fsanitize=undefined -fsanitize=leak -g -Winvalid-pch -D_DEBUG)
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_options(zenengine PRIVATE -O3   -DNDEBUG)
    target_link_options(zenengine PRIVATE -O3  -DNDEBUG)
endif()
