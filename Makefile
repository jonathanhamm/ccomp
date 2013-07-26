all:
	gcc -pthread -ggdb general.c parse.c lex.c main.c -o pc
