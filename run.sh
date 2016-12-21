#!/bin/bash
make clean
make
#rm *.d
scp sim root@172.16.0.78:/root/
