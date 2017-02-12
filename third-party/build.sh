#!/bin/bash

set -ex

if [ ! -e /usr/local/lib/libwtf.so ]; then
    pushd .
    cd ./tracing-framework/bindings/cpp/
    make clean
    make
    echo "Going to install Google Tracing Framework"
    sudo make install
    popd
fi

if [ ! -e third-party/tracing-framework/build-out/wtf_node_js_compiled.js ]; then
    # build js stuff
    pushd .
    cd tracing-framework
    npm install
    third_party/anvil-build/setup-local.sh
    third_party/anvil-build/anvil-local.sh deploy -o build-bin/ :wtf_node_js_compiled
    popd
fi
