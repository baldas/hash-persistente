
#
#

LDFLAGS = -lpmemobj

BINS = hash_persistente

LINKER=$(CC)

CFLAGS=-std=gnu99

.PHONY:	all clean


all:	$(BINS)

hash_persistente.o: hash_persistente.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(BINS):	%:	%.o 
	$(LINKER) -o $@ $< $(LDFLAGS)

debbug: hash_persistente.o
	$(LINKER) -o $@ $< $(LDFLAGS) -DSIMULATE_CRASH

clean:
	rm -f $(BINS) *.o


