#!/bin/bash

rm -rf build
rm -rf release
mkdir release
mkdir build
cd build
cmake ..
make extract
cd ..

cd release
wget https://github.com/nxengine/translations/releases/download/v1.14/all.zip
unzip all.zip
rm all.zip
wget https://sarcasticat.com/cavestoryen.zip
unzip cavestoryen.zip
rm cavestoryen.zip
cd CaveStory
../../build/nxextract
cp -r data/* ../data/
cd ..
rm -rf CaveStory
cp -r ../data/* data/
cd ..
rm -rf build

mkdir build
cd build
cmake -DPLATFORM=switch -GNinja -DCMAKE_BUILD_TYPE=Release ..
ninja
cd ..
rm -rf release/data