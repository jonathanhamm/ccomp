all:
	gcc -g3 -lm -pthread -ggdb semantics.c general.c parse.c lex.c main.c -o pc -lm
