lab11avkN32471.elf: lab11avkN32471.c
	gcc -o lab11avkN32471.elf -Wall -Wextra -Werror -O3 lab11avkN32471.c

clean:
	rm -f *.o *.elf

all: lab11avkN32471.elf
