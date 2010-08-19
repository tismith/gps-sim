all: main.c
	gcc -o gpssim main.c

.PHONY: clean
clean:
	-rm *.o
	-rm gpssim
