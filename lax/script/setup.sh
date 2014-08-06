#!/bin/sh

echo "Installing required Linux system packets"
echo "-----------------------------------------------"
sudo apt-get install build-essential python3 python3-tk cifs-utils ghex
echo "-----------------------------------------------\n"

echo "Making LAX Kernel Modules"
echo "-----------------------------------------------"
cd ../kernel
make clean
make
cd ..
echo "-----------------------------------------------\n"

echo "Making LAX Application Modules"
echo "-----------------------------------------------"
cd ./app
make clean
make
cd ..
echo "-----------------------------------------------\n"
echo "Setup Finished!"

