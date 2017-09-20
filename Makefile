CC=gcc
CFLAGS=-Wall -O3
LDFLAGS=
LIBS=-lm

TEST_FILES = tests/printf.bin tests/arithmetic.bin
.PHONY: tests

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

all: main.o ast.o utils.o parser.o codegen.o
	gcc $(CFLAGS) $(LDFLAGS) -o lisp $^ $(LIBS)

%.bin: %.lisp
	@./lisp -o $@ $<
	@./$@ > ./$<.result
	-@if [ "x$$(diff $<.result $<.expects | wc -l)" = "x0" ]; then \
		echo "[PASS] $<"; \
	else \
		echo "[FAIL] $<"; \
		diff $<.result $<.expects; \
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

