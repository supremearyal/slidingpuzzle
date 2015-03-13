CC = gcc
INFILE = puzzle.c
OUTFILE = puzzle
CFLAGS = -Wall -ansi -pedantic `sdl-config --cflags`
LIBS = `sdl-config --libs`

all: $(OUTFILE)

$(OUTFILE): $(INFILE)
	$(CC) -g -o $(OUTFILE) $(INFILE) $(CFLAGS) $(LIBS)
