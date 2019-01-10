#!/bin/bash
set -e

if [ -d clang-arm-fropi -a -d gcc-arm-none-eabi-5_3-2016q1 -a -d nanos-secure-sdk-f9e1c7b8904df2eee0ae7e603f552b876c169334 ]; then
    echo "Devend is already prepared. Exiting."
    exit
fi

wget https://launchpad.net/gcc-arm-embedded/5.0/5-2016-q1-update/+download/gcc-arm-none-eabi-5_3-2016q1-20160330-linux.tar.bz2
tar xf gcc-arm-none-eabi-5_3-2016q1-20160330-linux.tar.bz2

wget http://releases.llvm.org/4.0.0/clang+llvm-4.0.0-x86_64-linux-gnu-ubuntu-16.10.tar.xz
tar xf clang+llvm-4.0.0-x86_64-linux-gnu-ubuntu-16.10.tar.xz
mv clang+llvm-4.0.0-x86_64-linux-gnu-ubuntu-16.10 clang-arm-fropi

wget https://github.com/LedgerHQ/nanos-secure-sdk/archive/f9e1c7b8904df2eee0ae7e603f552b876c169334.tar.gz -O nanos-1421.tar.gz
tar xf nanos-1421.tar.gz
