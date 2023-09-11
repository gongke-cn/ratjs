CC := $(CROSS_COMPILE)gcc
TARGET := script_demo

all:
	$(CC) -o $(TARGET) script.c -lratjs -licuuc -lgmp -lm

clean:
	rm -f $(TARGET)