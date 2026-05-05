
NAME := 'luna'

CXX := env_var_or_default('CXX', 'g++')
CXXFLAGS := env_var_or_default('CXXFLAGS', '-fPIC -std=c++23 -Wall -Wextra')
PFLAGS := '--cflags --libs'
PLIBS := 'Qt6Widgets Qt6WebEngineWidgets libcurl'
BUILD_DIR := 'build'

build EXTRA_CXXFLAGS='-ggdb -O0':
    [[ -d {{BUILD_DIR}} ]] || mkdir -p {{BUILD_DIR}}
    {{CXX}} {{CXXFLAGS}} -DLUNA_DEBUG_BUILD {{EXTRA_CXXFLAGS}} src/main.cpp -o {{BUILD_DIR}}/luna-browser $(pkg-config {{PFLAGS}} {{PLIBS}})

test EXTRA_CXXFLAGS='-ggdb -O0':
    [[ -d {{BUILD_DIR}} ]] || mkdir -p {{BUILD_DIR}}
    {{CXX}} {{CXXFLAGS}} {{EXTRA_CXXFLAGS}} tests/main.cpp -o {{BUILD_DIR}}/luna-test $(pkg-config {{PFLAGS}} {{PLIBS}})
    {{BUILD_DIR}}/luna-test

build-realease EXTRA_CXXFLAGS='-O3': 
    just build {{EXTRA_CXXFLAGS}}

#  build-static EXTRA_CXXFLAGS='-O3 -static -static-libstdc++ -static-libgcc':
#      [[ -d {{BUILD_DIR}} ]] || mkdir -p {{BUILD_DIR}}
#      {{CXX}} {{CXXFLAGS}} {{EXTRA_CXXFLAGS}} src/main.cpp -o {{BUILD_DIR}}/luna-browser-static $(pkg-config --static {{PFLAGS}} {{PLIBS}})

build-static EXTRA_CXXFLAGS='-O3 -static-libstdc++ -static-libgcc':
    [[ -d {{BUILD_DIR}} ]] || mkdir -p {{BUILD_DIR}}
    {{CXX}} {{CXXFLAGS}} {{EXTRA_CXXFLAGS}} src/main.cpp -o {{BUILD_DIR}}/luna-browser $(pkg-config {{PFLAGS}} {{PLIBS}})

download-easylist:
    curl -O https://easylist.to/easylist/easylist.txt

clean:
    git clean -ffdx
