CC = arm-none-linux-gnueabi-gcc
CCFLAGS = -Wall -static -lpthread
TARGET = sim
all:$(TARGET)

$(TARGET):sims.o functions.o arbitrator.o sim.o spi.o log.o
	$(CC) $^  -o $@ $(CCFLAGS)

clean:
	-rm $(TARGET) *.o *.d

.PHONY:clean

sources = functions.c arbitrator.c sim.c spi.c sims.c log.c

include $(sources:.c=.d)

%.d:%.c
	set -e;rm -f $@;\
	$(CC) -MM $< > $@.$$$$;\
	sed 's,\($*\)\.o[ :]*,\1.o $@ :,g' < $@.$$$$ > $@;\
	rm -f $@.$$$$
