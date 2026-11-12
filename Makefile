# Relief Load-and-Route Planner - CSEG3053 Phase II
#
#   mingw32-make          build build/relief.exe
#   mingw32-make run      build and start the application
#   mingw32-make check    build and run all automated tests
#   mingw32-make clean    remove build outputs

CC      = gcc
CFLAGS  = -std=c17 -Wall -Wextra -Wpedantic -O2 -Isrc
LDLIBS  = -lm

BUILD   = build
APP     = $(BUILD)/relief.exe

# Every module except main.c; tests link against these.
MODULES = \
	src/metrics.c \
	src/ui_io.c \
	src/csv_util.c \
	src/sort_search.c \
	src/graph.c \
	src/knapsack_dp.c \
	src/knapsack_greedy.c \
	src/knapsack_bb.c \
	src/relief_model.c \
	src/validation.c \
	src/relief_planner.c \
	src/dataset_io.c \
	src/benchmark.c \
	src/ui_menu.c

HEADERS = $(wildcard src/*.h)

TESTS = \
	$(BUILD)/test_algorithms.exe \
	$(BUILD)/test_planner.exe \
	$(BUILD)/test_edge_cases.exe \
	$(BUILD)/test_stress.exe

.PHONY: all run tests check clean

all: $(APP)

$(BUILD):
	mkdir -p $(BUILD)

$(APP): src/main.c $(MODULES) $(HEADERS) | $(BUILD)
	$(CC) $(CFLAGS) src/main.c $(MODULES) -o $@ $(LDLIBS)

$(BUILD)/%.exe: tests/%.c tests/test_util.h $(MODULES) $(HEADERS) | $(BUILD)
	$(CC) $(CFLAGS) -Itests $< $(MODULES) -o $@ $(LDLIBS)

tests: $(TESTS)

check: tests
	./$(BUILD)/test_algorithms.exe
	./$(BUILD)/test_planner.exe
	./$(BUILD)/test_edge_cases.exe
	./$(BUILD)/test_stress.exe

run: $(APP)
	./$(APP)

clean:
	$(RM) $(APP) $(TESTS)
