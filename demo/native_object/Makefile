CC := $(CROSS_COMPILE)gcc
TARGET := native_object_demo

all:
	$(CC) -o $(TARGET) native_object.c -lratjs -licuuc -lm

clean:
	rm -f $(TARGET)
