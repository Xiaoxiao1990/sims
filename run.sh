#!/bin/bash
make clean
make
#rm *.d
scp sim root@172.16.0.84:/root/
