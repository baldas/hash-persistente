
#
#

LDFLAGS = -lpmemobj

BINS = HashVers24fev2026

LINKER=$(CC)

CFLAGS=-std=gnu99

.PHONY:	all clean


all:	$(BINS)

HashVers24fev2026.o: HashVers24fev2026.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(BINS):	%:	%.o 
	$(LINKER) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(BINS) *.o


