
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

hash_persistente.o: hash_persistente.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(BINS):	%:	%.o 
	$(LINKER) -o $@ $< $(LDFLAGS)

clean:
	rm -f *.o

veryclean:
	rm -f $(BINS) *.o
