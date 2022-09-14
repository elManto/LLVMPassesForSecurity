CFLAGS=-g -O0 -fno-discard-value-names

main: main.c benign.so
	$(CC) $(CFLAGS) -I./ ./main.c ./benign.so -o main.bin

benign.so: add.c
	$(CC) $(CFLAGS) -fPIC -shared -o ./benign.so add.c

clean:
	rm ./benign.so main.bin
