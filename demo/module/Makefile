CC := $(CROSS_COMPILE)gcc
TARGET := module_demo

all:
	$(CC) -o $(TARGET) -g module.c -lratjs -licuuc -lgmp -lm

clean:
	rm -f $(TARGET)