#!/bin/bash

set -e

rm -rf libs

mkdir -p libs
cd libs/

git clone https://github.com/sudomesh/LoRaLayer2

git clone https://github.com/sudomesh/disaster-radio

git clone https://github.com/bombela/backward-cpp
cd backward-cpp
git checkout v1.6
cd ../..
