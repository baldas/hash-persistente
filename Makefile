
#
#

LDFLAGS = -lpmemobj

BINS = hash_persistente

LINKER=$(CC)

CFLAGS=-std=gnu99

.PHONY:	all clean


all:	$(BINS)

execute: veryclean $(BINS)
	./$(BINS)

test: veryclean $(BINS)
	./$(BINS) 10

massive_test: veryclean hash_persistente.c
	$(CC) hash_persistente.c -o $(BINS)  $(LDFLAGS) -DMASSIVE_TEST
	$(CC) big_test.c -o big_test
	./big_test 20

hash_persistente.o: hash_persistente.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(BINS):	%:	%.o 
	$(LINKER) -o $@ $< $(LDFLAGS)

clean:
	rm -f *.o

veryclean:
	rm -f $(BINS) big_test *.o hash_pool*.obj
