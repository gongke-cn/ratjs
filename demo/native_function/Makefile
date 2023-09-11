CC := $(CROSS_COMPILE)gcc
TARGET := native_function_demo

all:
	$(CC) -o $(TARGET) native_function.c -lratjs -licuuc -lgmp -lm

clean:
	rm -f $(TARGET)
