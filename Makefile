all:
	gcc -lm -pthread -ggdb semantics.c general.c parse.c lex.c main.c -o pc
