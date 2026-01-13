CXX ?= g++
CXXFLAGS := -std=c++20 -O3 -march=native -mtune=native -flto -finline-functions -fstrict-aliasing -fno-exceptions -fno-rtti -pthread -Iinclude
OUT_DIR := out
SRC := $(wildcard src/*.cpp)
TESTS := $(wildcard tests/test_*.cpp)
BENCH_SRC := $(wildcard tests/bench_*.cpp)

.PHONY: all test clean

all: test

$(OUT_DIR):
	mkdir -p $(OUT_DIR)

$(OUT_DIR)/test_runner: $(OUT_DIR) $(SRC) $(TESTS)
	$(CXX) $(CXXFLAGS) $(SRC) $(TESTS) -o $@

$(OUT_DIR)/bench_%: $(OUT_DIR) $(SRC) tests/bench_%.cpp
	$(CXX) $(CXXFLAGS) $(SRC) tests/bench_$*.cpp -o $@

test: $(OUT_DIR)/test_runner

benches: $(OUT_DIR)/bench_basic $(OUT_DIR)/bench_alignment $(OUT_DIR)/bench_remote $(OUT_DIR)/bench_four_thread $(OUT_DIR)/bench_multialign $(OUT_DIR)/bench_remote_six $(OUT_DIR)/bench_remote_many_to_one

clean:
	rm -f $(OUT_DIR)/test_runner $(OUT_DIR)/bench_basic $(OUT_DIR)/bench_alignment $(OUT_DIR)/bench_remote $(OUT_DIR)/bench_four_thread $(OUT_DIR)/bench_multialign $(OUT_DIR)/bench_remote_six $(OUT_DIR)/bench_remote_many_to_one
