CC = gcc
OBJS = parser.tab.c lex.yy.c ast.c interp.c codegen.c

calc: $(OBJS)
	$(CC) $(OBJS) -o calc

parser.tab.c parser.tab.h: parser.y
	bison -d -v parser.y

lex.yy.c: lexer.l parser.tab.h
	flex lexer.l

clean:
	rm -f parser.tab.c parser.tab.h parser.output lex.yy.c calc out*.py ast*.json
