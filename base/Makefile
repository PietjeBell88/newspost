all: test encode.o nntp.o newspost.o socket.o utils.o

test:
	$(CC) $(CFLAGS) -o test test.c
	./test
	-rm -f test test.o

clean:
	-rm -f test *.o *~

