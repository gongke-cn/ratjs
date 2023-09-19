CC := $(CROSS_COMPILE)gcc
TARGET := module_demo

all:
	$(CC) -o $(TARGET) -g module.c -lratjs -licuuc -lm

clean:
	rm -f $(TARGET)