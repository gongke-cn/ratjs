CC := $(CROSS_COMPILE)gcc
TARGET := native_module.njs
OBJ    := native_module.o

all:
	$(CC) -fPIC -c -o $(OBJ) native_module.c
	$(CC) -o $(TARGET) $(OBJ) -shared -lratjs -licuuc -lm

clean:
	rm -f $(TARGET) $(OBJ)
