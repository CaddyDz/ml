# Headless regression tests for the non-Qt core. Needs GNU make (gmake on BSD).
# Usage: make check    (sanitizers on; build without them with: make check SANITIZE=)
#        make leaks    (macOS: runs the tests, then the system leaks tool)

# GNU make predefines CXX (as g++): only replace that default, not a user's choice
ifneq ($(filter default undefined,$(origin CXX)),)
CXX := c++
endif
SANITIZE ?= -fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer
CXXFLAGS ?= -std=c++14 -g -O1 -Wall -Wextra

SRC := ../src
CORE := $(SRC)/defs.cpp $(SRC)/node.cpp $(SRC)/tree.cpp $(SRC)/parser.cpp \
        $(SRC)/treegenerator.cpp $(SRC)/classifier.cpp $(SRC)/data.cpp

# rebuild when the compiler or the flags change (e.g. make check SANITIZE=): decided
# when the Makefile is read, so it does not depend on file timestamps
FLAGS := $(CXX) $(CXXFLAGS) $(SANITIZE)
ifneq ($(strip $(FLAGS)),$(strip $(shell cat .flags 2>/dev/null)))
$(shell rm -f test_core; echo '$(FLAGS)' > .flags)
endif

test_core: test_core.cpp $(CORE) $(wildcard $(SRC)/*.hpp)
	$(CXX) $(CXXFLAGS) $(SANITIZE) -I$(SRC) test_core.cpp $(CORE) -o $@

# stack-use-after-return detection is off by default in AddressSanitizer
check: test_core
	ASAN_OPTIONS="detect_stack_use_after_return=1:$$ASAN_OPTIONS" ./test_core

# macOS has no LeakSanitizer: check for leaks with the system tool on a plain build,
# once the tests pass (leaks itself only reports leaks)
leaks: test_core.cpp $(CORE) $(wildcard $(SRC)/*.hpp)
	$(CXX) $(CXXFLAGS) -I$(SRC) test_core.cpp $(CORE) -o test_core_plain
	./test_core_plain > /dev/null
	leaks --atExit -- ./test_core_plain

clean:
	rm -rf test_core test_core_plain .flags *.dSYM

.PHONY: check leaks clean
