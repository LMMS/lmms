TESTS=test_new test_cond_wait test_cond_wait_simple test_printf

jack_interposer.so: jack_interposer.c checkers.c manual.c
	gcc -Wall -fPIC -o jack_interposer.so -shared jack_interposer.c -pthread -ldl -ljack

checkers.c: functions checker_fragment.c
	./generate_checkers.pl < functions

.PHONY clean: 
	rm jack_interposer.so test_cond_wait test_cond_wait_simple || true

test: $(TESTS) jack_interposer.so
	LD_PRELOAD=./jack_interposer.so ./test_cond_wait_simple
	LD_PRELOAD=./jack_interposer.so ./test_cond_wait
	LD_PRELOAD=./jack_interposer.so ./test_new
	LD_PRELOAD=./jack_interposer.so ./test_printf

test_new: test_new.cpp
	g++ -o test_new test_new.cpp -ljack

test_cond_wait_simple: test_cond_wait_simple.c
	gcc -o test_cond_wait_simple test_cond_wait_simple.c -pthread -ldl

test_cond_wait: test_cond_wait.c
	gcc -o test_cond_wait test_cond_wait.c -ljack -lpthread

test_printf: test_printf.c
	gcc -o test_printf test_printf.c -ljack 
