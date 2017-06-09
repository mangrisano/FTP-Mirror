CC = gcc
CFLAGS = -Wall -ansi -pedantic -Wstrict-prototypes -D_DEFAULT_SOURCE
PROG = server
SOURCES = $(PROG).c


$(PROG): $(SOURCES)
	$(CC) $(CFLAGS) -o $(PROG) $(SOURCES) $(LDFLAGS)


.PHONY: clean all

# Removing the binary
clean:
	$(RM) $(PROG) *~ ~.o
