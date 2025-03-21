CC = gcc
CFLAGS = -I. -Wall -Wextra -g
DEPS = dberror.h buffer_mgr.h buffer_mgr_stat.h storage_mgr.h test_helper.h
OBJ = dberror.o buffer_mgr.o buffer_mgr_stat.o storage_mgr.o

default: test_assign2_1 test_assign2_2

test_assign2_1: $(OBJ) test_assign2_1.o
	$(CC) $(CFLAGS) -o test_assign2_1 test_assign2_1.o $(OBJ)

test_assign2_2: $(OBJ) test_assign2_2.o
	$(CC) $(CFLAGS) -o test_assign2_2 test_assign2_2.o $(OBJ)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $<

.PHONY: clean run_test1 run_test2

clean:
	rm -f *.o test_assign2_1 test_assign2_2

run_test1:
	./test_assign2_1

run_test2:
	./test_assign2_2