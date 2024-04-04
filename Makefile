CC = gcc
CFLAGS = -w
LIBS = -lm
MAIN = part_1a part_1b part_2

.PHONY: clean pack all run run_part_1a run_part_1b run_part_2

all: clean $(MAIN)

part_1a: part_1a.c
	$(CC) $(CFLAGS) -o part_1a part_1a.c $(LFLAGS) $(LIBS)

part_1b: part_1b.c
	$(CC) $(CFLAGS) -o part_1b part_1b.c $(LFLAGS) $(LIBS)

part_2: part_2.c
	$(CC) $(CFLAGS) -O2 -o part_2 part_2.c $(LFLAGS) $(LIBS)

run_part_1a: part_1a
	./part_1a 10000 34 6

run_part_1b: part_1b
	./part_1b 1000000 45 6

run_part_2: part_2
	./part_2 100000 45 2

run: run_part_1a run_part_1b run_part_2

clean:
	@echo "make clean: Cleaning up..."
	rm -f $(MAIN) *.txt

pack:
	@echo "Packing up..."
	rm -f proj1.zip
	zip -r proj1.zip *.c *.h README.md Makefile