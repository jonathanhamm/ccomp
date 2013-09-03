all:
	gcc -O3 -pthread -ggdb general.c parse.c lex.c main.c -o pc
