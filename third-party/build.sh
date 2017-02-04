#!/bin/bash

set -ex

cd ./tracing-framework/bindings/cpp/
# as of 2017/02 default threading won't work; using STD threading model instead
make THREADING=std
echo "Going to install Google Tracing Framework"
sudo make install
