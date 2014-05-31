matcher: diagnostics.o general.o main.o optimize.o parser.o tokenizer.o tokens.o tree.o match.o subst.o infix-printer.o rulefiles.o dict.o apply-rules.o gc.o floateval.o hashtable.o
	cc $(CFLAGS) *.o -o matcher -lm

floateval.o: floateval.c
	cc $(CFLAGS) -c floateval.c -lm

clean:
	rm -f *.o matcher
