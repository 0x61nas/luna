
NAME := 'luna'

CXX := env_var_or_default('CXX', 'g++')
CXXFLAGS := env_var_or_default('CXXFLAGS', '-fPIC -std=c++23 -Wall -Wextra')
PFLAGS := '--cflags --libs'
PLIBS := 'Qt6Widgets Qt6WebEngineWidgets libcurl'
BUILD_DIR := 'build'
SU := env_var_or_default('SU', 'su -c')
DESTDIR := env_var_or_default('DESTDIR', '/usr/local/bin')

build EXTRA_CXXFLAGS='-ggdb -O0 -DLUNA_DEBUG_BUILD':
    [[ -d {{BUILD_DIR}} ]] || mkdir -p {{BUILD_DIR}}
    {{CXX}} {{CXXFLAGS}} {{EXTRA_CXXFLAGS}} src/main.cpp -o {{BUILD_DIR}}/luna-browser $(pkg-config {{PFLAGS}} {{PLIBS}})

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

sanitize SANITIZER='address,undefined':
    [[ -d {{BUILD_DIR}} ]] || mkdir -p {{BUILD_DIR}}
    clang++ {{CXXFLAGS}} -O0 -g -fsanitize={{SANITIZER}} src/main.cpp -o {{BUILD_DIR}}/luna-browser-santize $(pkg-config {{PFLAGS}} {{PLIBS}})
    LSAN_OPTIONS=suppressions=lsan_suppr.txt {{BUILD_DIR}}/luna-browser-santize

download-easylist:
    curl -O https://easylist.to/easylist/easylist.txt

install:
    just build-realease
    {{SU}} 'install -Dm755 {{BUILD_DIR}}/luna-browser {{DESTDIR}}/luna-browser'

clean:
    git clean -ffdx
