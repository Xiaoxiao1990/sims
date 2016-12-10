
CC = arm-none-linux-gnueabi-gcc
CCFLAGS = -static -Wall
TARGET = sim
all:$(TARGET)

$(TARGET):sims.o functions.o arbitrator.o sim.o spi.o
	$(CC) $^ $(CCFLAGS) -o $@

clean:
	-rm $(TARGET) *.o *.d

.PHONY:clean

sources = functions.c arbitrator.c sim.c spi.c sims.c

include $(sources:.c=.d)

%.d:%.c
	set -e;rm -f $@;\
	$(CC) -MM $< > $@.$$$$;\
	sed 's,\($*\)\.o[ :]*,\1.o $@ :,g' < $@.$$$$ > $@;\
	rm -f $@.$$$$
