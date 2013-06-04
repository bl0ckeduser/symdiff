matcher: diagnostics.o general.o main.o optimize.o parser.o tokenizer.o tokens.o tree.o match.o subst.o infix-printer.o rulefiles.o dict.o apply-rules.o
	cc *.o -o matcher

clean:
	rm -f *.o matcher
