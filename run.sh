#!/bin/bash
make clean
make;
rm *.d *.o
scp sim root@172.16.0.91:/root
