#!/bin/sh

#先编译sensor

cd Sensor

./build.sh
cp libs/* /nfs -f

cd ../

#编译媒体库
make clean; make
cp Lib/hisi3516dv300/libmedia.so ./output/ -f
cp Lib/hisi3516dv300/libmedia.so /nfs/ -f
cp Demo/test /nfs/ -f

rm -rf ./Bin

