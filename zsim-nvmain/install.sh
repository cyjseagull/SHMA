#!/bin/bash -e

# $lic$
#
# Copyright (C) 2014 by Adria Armejach <adria.armejach@bsc.es>
#
# This file is part of zsim.
#
# zsim is free software; you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation, version 2.
#
# If you use this software in your research, we request that you reference
# the zsim paper ("ZSim: Fast and Accurate Microarchitectural Simulation of
# Thousand-Core Systems", Sanchez and Kozyrakis, ISCA-40, June 2013) as the
# source of the simulator in any publications that use this software, and that
# you send us a citation of your work.
#
# zsim is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program. If not, see <http://www.gnu.org/licenses/>.

BASEDIR=$(dirname "$0")
BASEDIR=$(cd "$BASEDIR"; pwd)
PIN_ROOT=pin_kit
NVMAIN_ROOT=nvmain
PINVER=pin-2.13-61206-gcc.4.4.7-linux

if [ ! -d "$PIN_ROOT" ]; then
    echo "-----------------------"
    echo "To use this program, you must agree to the license of the Pin Tool, that you can read at:"
    echo "http://software.intel.com/sites/default/files/m/a/d/2/extlicense.txt"
    echo "If you do not agree, press Ctrl+C in the next 10 seconds, to quit"
    echo "-----------------------"
    sleep 10
    echo "Downloading and unpacking PIN tool"
    wget -nv -c "http://software.intel.com/sites/landingpage/pintool/downloads/$PINVER.tar.gz" -O- | tar xz
    echo "Done. PIN tool is downloaded and ready"
    mv "$PINVER" "$PIN_ROOT"
else
    echo "Pin seems to be already installed at $BASEDIR/$PIN_ROOT, skiping"
fi

if [ ! -d "$NVMAIN_ROOT" ]; then
    echo "Cloning NVMain repository..."
    echo "The code is available, but you may need to ask the authors for access to the repo"
    echo "If that doesn't work please contact this script's author to get a copy of the sources."
    hg clone https://bitbucket.org/mrp5060/nvmain $NVMAIN_ROOT
fi

echo "Installing necessary libraries..."
sudo yum install libelfg0-dev libhdf5-serial-dev scons libconfig++-dev

echo "Set up the following environment varaiables:"
echo "Add 'PINPATH=$BASEDIR/$PIN_ROOT' to your environment."
echo "Add 'NVMAINPATH=$BASEDIR/$NVMAIN_ROOT' to your environment."
echo "Add 'ZSIMPATH=$BASEDIR' to your environment."
echo "Install the Boost libraries and include 'LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/path/to/boost/stage/lib' to your environment."

exit 0
