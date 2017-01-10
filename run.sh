#!/bin/bash
make clean
make;
rm *.d *.o
scp sim root@192.168.1.55:/root
