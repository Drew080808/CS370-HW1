CC        :=   gcc
CFLAGS    :=   -std=c23 -Wall -Wextra -Werror -g -O1 -Iinclude
SRC       :=   src/rbtree.c
TSRC      :=   tests/test_rbtree.c
WBSRC     :=   tests/test_rbtree_whitebox.c
BIN       :=   build/test_rbtree
FUZZBIN   :=   build/fuzz
WBBIN     :=   build/test_rbtree_whitebox

all: $(BIN) $(FUZZBIN) $(WBBIN)

$(BIN): $(SRC) $(TSRC) include/rbtree.h
	@mkdir -p build
	$(CC) $(CFLAGS) $(SRC) $(TSRC) -o $@

$(FUZZBIN): $(SRC) tests/fuzz.c include/rbtree.h
	@mkdir -p build
	$(CC) $(CFLAGS) $(SRC) tests/fuzz.c -o $@

# WBSRC #includes $(SRC) directly (white-box access to internals), so it
# must not be linked against $(SRC) again here.
$(WBBIN): $(WBSRC) $(SRC) include/rbtree.h
	@mkdir -p build
	$(CC) $(CFLAGS) $(WBSRC) -o $@

test: $(BIN) $(FUZZBIN) $(WBBIN)
	./$(BIN) && ./$(FUZZBIN) 100000 && ./$(WBBIN)

asan: CFLAGS += -fsanitize=address,undefined -fno-omit-frame-pointer
asan: clean test

memcheck: all
	valgrind --leak-check=full --show-leak-kinds=all \
		--error-exitcode=1 ./$(BIN)
	valgrind --leak-check=full --show-leak-kinds=all \
		--error-exitcode=1 ./$(FUZZBIN) 20000
	valgrind --leak-check=full --show-leak-kinds=all \
		--error-exitcode=1 ./$(WBBIN)

clean:
	rm -rf build

.PHONY: all test asan memcheck clean
