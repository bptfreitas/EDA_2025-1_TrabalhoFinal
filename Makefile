SRC = main.c

all:
	gcc -O0 main.c -o schedulers -fopenmp

run: 
	sudo ./schedulers

clean:
	rm -f *.out schedulers