# Plain-make alternative to CMake (Linux / macOS / MSYS2). Usage:  make  |  make test  |  make run
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
ROOT     := $(CURDIR)
INC      := -Ibackend/include
BUILD    := build-make

CORE_SRC := $(wildcard backend/src/models/*.cpp backend/src/services/*.cpp backend/src/repositories/*.cpp) \
            backend/src/utils/Json.cpp backend/src/utils/StringUtils.cpp backend/src/utils/DateUtils.cpp
API_SRC  := $(wildcard backend/src/controllers/*.cpp) backend/src/utils/HttpServer.cpp
TEST_SRC := $(wildcard tests/*.cpp)

obj = $(patsubst %.cpp,$(BUILD)/%.o,$(1))
CORE_OBJ := $(call obj,$(CORE_SRC))
API_OBJ  := $(call obj,$(API_SRC))
TEST_OBJ := $(call obj,$(TEST_SRC))

all: $(BUILD)/smartlibrary $(BUILD)/smartlibrary_tests

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@

$(BUILD)/main.o: backend/main.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INC) -DSMARTLIBRARY_ROOT='"$(ROOT)"' -c $< -o $@

$(BUILD)/smartlibrary: $(BUILD)/main.o $(API_OBJ) $(CORE_OBJ)
	$(CXX) $^ -o $@ -pthread $(LDFLAGS)

$(BUILD)/smartlibrary_tests: $(TEST_OBJ) $(API_OBJ) $(CORE_OBJ)
	$(CXX) $^ -o $@ -pthread $(LDFLAGS)

test: $(BUILD)/smartlibrary_tests
	./$(BUILD)/smartlibrary_tests

run: $(BUILD)/smartlibrary
	./$(BUILD)/smartlibrary

clean:
	rm -rf $(BUILD)

.PHONY: all test run clean
