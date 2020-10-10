#!/bin/sh

#编译sensor
cd Sensor
./build.sh

#编译媒体库
cd ../
make clean; make

cp output/*.so /nfs/ -f
cp Demo/test /nfs/ -f

rm -rf ./Bin

