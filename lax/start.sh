#!/bin/sh

./script/umount_win.sh
./script/mount_win.sh

cd kernel
sudo ./unload_lax
sudo ./load_lax
cd ..

cd ./app
python3 lax.py
cd ..
