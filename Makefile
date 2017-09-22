CC=gcc
CFLAGS=-Wall -O3
LDFLAGS=
LIBS=-lm
LISP_FLAGS=

TEST_FILES = tests/printf.bin \
             tests/arithmetic.bin \
             tests/variable.bin \
             tests/compare.bin \
             tests/conditional.bin \
             tests/while.bin
.PHONY: tests

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

all: main.o ast.o utils.o parser.o codegen.o
	gcc $(CFLAGS) $(LDFLAGS) -o lisp $^ $(LIBS)

%.bin: %.lisp
	-@./lisp $(LISP_FLAGS) -o $@ $<
	@./$@ > ./$<.result; \
	if [ "x$$?" = "x0" ] && [ -e "$<.expects" ] && [ "x$$(diff $<.result $<.expects | wc -l)" = "x0" ]; then \
		printf "\033[1m\033[32m[PASS]\033[0m $<\n"; \
	else \
		printf "\033[1m\033[31m[FAIL]\033[0m $<\n"; \
		echo "Results:"; cat $<.result; echo "Expects:"; cat $<.expects; \
	fi

tests: clean_tests do_tests

do_tests: $(TEST_FILES)

clean_tests:
	@rm -f tests/*.bin
	@rm -f tests/*.result
	@rm -f tests/*.o
	@rm -f tests/*.s

clean:
	rm -rf *.o
	rm -f lisp

