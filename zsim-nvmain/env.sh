#!/bin/sh
PINPATH=/home/chenyujie/zsim-nvmain/pin_kit
NVMAINPATH=/home/chenyujie/zsim-nvmain/nvmain
PBBPATH=/home/chenyuejie/benchs/pbb
ZSIMPATH=/home/chenyujie/zsim-nvmain
BOOST=/usr/local/boost_1.59
HDF5=/usr/local/hdf5-1.8.3
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PINPATH/intel64/lib:$PINPATH/intel64/runtime:$PINPATH/intel64/lib:$PINPATH/intel64/lib-ext:$BOOST/lib:$HDF5/lib:/usr/local/gmp-4.3.2/lib:/usr/local/mpfr-2.4.2/lib:/usr/local/mpc-0.8.1/lib
INCLUDE=$INCLUDE:$HDF5/include
LIBRARY_PATH=$LIBRARY_PATH:$HDF5/lib
CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH:$HDF5/include
CPUSPECPATH=/home/chenyujie/benchs/spec_cpu2006/benchspec/CPU2006
CONFIG=/media/liao/chenyujie/HSCH/config
TESTCONFIG=/media/liao/chenyujie/863_test/config
export ZSIMPATH PINPATH NVMAINPATH LD_LIBRARY_PATH BOOST CPLUS_INCLUDE_PATH LIBRARY_PATH CPUSPECPATH CONFIG PBBPATH TESTCONFIG
