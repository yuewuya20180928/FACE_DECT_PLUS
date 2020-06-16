#/bin/bash
cd sony_imx327
make clean; make
cp lib/lib* ../../Lib/hisi3516dv300/hi_mpp/ -f
cp lib/lib*.so ../../Demo/Lib/sensor/ -f
cp lib/lib*.so ../../output/ -f

cd ../sony_imx327_2L
make clean; make
cp lib/lib* ../../Lib/hisi3516dv300/hi_mpp/ -f
cp lib/lib*.so ../../Demo/Lib/sensor/ -f
cp lib/lib*.so ../../output/ -f
