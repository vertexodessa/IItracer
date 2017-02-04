#!/bin/bash

set -ex

pushd .
cd ./tracing-framework/bindings/cpp/
# as of 2017/02 default threading won't work; using STD threading model instead
make THREADING=std
echo "Going to install Google Tracing Framework"
sudo make install
popd

# build js stuff
pushd .
cd tracing-framework
npm install
third_party/anvil-build/setup-local.sh
third_party/anvil-build/anvil-local.sh deploy -o build-bin/ :wtf_node_js_compiled
popd