.PHONY: test libkvdb

test: libkvdb-test-64 libkvdb-test-32
	@echo "==== TEST 64 bit ===="
	@LD_LIBRARY_PATH=.. ./libkvdb-test-64
	@echo "==== TEST 32 bit ===="
	@LD_LIBRARY_PATH=.. ./libkvdb-test-32

libkvdb:
	@cd .. && make -s

libkvdb-test-64: main.c libkvdb
	gcc -I.. -L.. -m64 main.c -o libkvdb-test-64 -lkvdb-64 -lpthread

libkvdb-test-32: main.c libkvdb
	gcc -I.. -L.. -m32 main.c -o libkvdb-test-32 -lkvdb-32 -lpthread

test1: libkvdb-test-64
	@echo "==== TEST 64 bit ===="
	@LD_LIBRARY_PATH=.. ./libkvdb-test-64
test2: libkvdb-test-32
	@echo "==== TEST 32 bit ===="
	@LD_LIBRARY_PATH=.. ./libkvdb-test-32

clean:
	rm -f libkvdb-test-32 libkvdb-test-64
