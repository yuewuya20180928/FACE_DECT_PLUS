#!/bin/sh

#编译sensor
cd Sensor
./build.sh

#编译DSP(独立进程)
cd ../
make clean; make

#编译DSP API(应用调用接口)
cd ./Api
make clean; make

cd ../
cp output/*.so /nfs/ -f
cp output/dspzx /nfs/ -f
cp Demo/test /nfs/ -f

rm -rf ./Bin

