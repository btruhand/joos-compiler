# Makefile for joos compiler
CC=g++
INCLUDE=-I./include -I./include/dfas -I./include/weeds -I./include/AST -I./include/sym-table -I./include/type-link -I./include/hierarchy-checking -I./include/disambiguate -I./include/type-checking -I./include/reachability -I./include/code-gen -I./include/code-gen/startup -I./include/code-gen/vtable -I./include/code-gen/object-layout -I./include/code-gen/label -I./include/code-gen/inheritance -I./include/code-gen/interface-methods -I./include/code-gen/static
ifeq ($(CODE_OUT),)
CFLAGS=-std=c++0x -c -Wall $(INCLUDE)
else
CFLAGS=-std=c++0x -c -Wall -DCODE_OUT='"$(CODE_OUT)"' $(INCLUDE)
endif
BUILD_DIR=build/src
OUT_FILE=joosc

# Test Folders
TEST_CASES = a1 a2 a3 a4 a5

# AST Code
AST_C = $(wildcard src/AST/*.cpp)
AST_O = $(addprefix build/src/AST/, $(notdir $(AST_C:.cpp=.o)))

# DFA Code
DFA_C = $(wildcard src/dfas/*.cpp)
DFA_O = $(addprefix build/src/dfas/, $(notdir $(DFA_C:.cpp=.o)))

# Symbol table code
SYMTABLE_C = $(wildcard src/sym-table/*.cpp)
SYMTABLE_O = $(addprefix build/src/sym-table/, $(notdir $(SYMTABLE_C:.cpp=.o)))

# Type linking code
TYPELINK_C = $(wildcard src/type-link/*.cpp)
TYPELINK_O = $(addprefix build/src/type-link/, $(notdir $(TYPELINK_C:.cpp=.o)))

# HIERARCHY CHECKING CODE
HC_C = $(wildcard src/hierarchy-checking/*.cpp)
HC_O = $(addprefix build/src/hierarchy-checking/, $(notdir $(HC_C:.cpp=.o)))

# DISAMBIGUATION CODE
DIS_C = $(wildcard src/disambiguate/*.cpp)
DIS_O = $(addprefix build/src/disambiguate/, $(notdir $(DIS_C:.cpp=.o)))

# TYPE CHECKING CODE
TYPECHECK_C = $(wildcard src/type-checking/*.cpp)
TYPECHECK_O = $(addprefix build/src/type-checking/, $(notdir $(TYPECHECK_C:.cpp=.o)))

# REACHABILITY CODE
REACHABILITY_C = $(wildcard src/reachability/*.cpp)
REACHABILITY_O = $(addprefix build/src/reachability/, $(notdir $(REACHABILITY_C:.cpp=.o)))

# CODE GENERATION CODE

STARTUP_C = $(wildcard src/code-gen/startup/*.cpp)
STARTUP_O = $(addprefix build/src/code-gen/startup/, $(notdir $(STARTUP_C:.cpp=.o)))

VTABLE_C = $(wildcard src/code-gen/vtable/*.cpp)
VTABLE_O = $(addprefix build/src/code-gen/vtable/, $(notdir $(VTABLE_C:.cpp=.o)))

OLAYOUT_C = $(wildcard src/code-gen/object-layout/*.cpp)
OLAYOUT_O = $(addprefix build/src/code-gen/object-layout/, $(notdir $(OLAYOUT_C:.cpp=.o)))

LABEL_C = $(wildcard src/code-gen/label/*.cpp)
LABEL_O = $(addprefix build/src/code-gen/label/, $(notdir $(LABEL_C:.cpp=.o)))

INHERITANCE_C = $(wildcard src/code-gen/inheritance/*.cpp)
INHERITANCE_O = $(addprefix build/src/code-gen/inheritance/, $(notdir $(INHERITANCE_C:.cpp=.o)))

INTERFACE_C = $(wildcard src/code-gen/interface-methods/*.cpp)
INTERFACE_O = $(addprefix build/src/code-gen/interface-methods/, $(notdir $(INTERFACE_C:.cpp=.o)))

STATIC_C = $(wildcard src/code-gen/static/*.cpp)
STATIC_O = $(addprefix build/src/code-gen/static/, $(notdir $(STATIC_C:.cpp=.o)))

CODEGEN_C = $(wildcard src/code-gen/*.cpp)
CODEGEN_O = $(addprefix build/src/code-gen/, $(notdir $(CODEGEN_C:.cpp=.o)))

# Main Code
SRC_C = $(wildcard src/*.cpp)
SRC_O = $(addprefix build/src/, $(notdir $(SRC_C:.cpp=.o))) $(DFA_O) $(AST_O) $(SYMTABLE_O) $(TYPELINK_O) $(HC_O) $(DIS_O) $(TYPECHECK_O)\
       	$(REACHABILITY_O) $(CODEGEN_O) $(STARTUP_O) $(VTABLE_O) $(OLAYOUT_O) $(LABEL_O) $(INHERITANCE_O) $(INTERFACE_O) $(STATIC_O)

# Test Code
TEST_C = $(wildcard tests/src/*.cpp)
TEST_O = $(addprefix build/tests/, $(notdir $(TEST_C:.cpp=.o)))
TEST_CFLAGS=-std=c++0x -c -Wall -I./include -I./tests/include -I./include/dfas -I./include/weeds -I./include/AST -I./include/sym-table -I./include/type-link -I./include/hierarchy-checking -I./include/disambiguate -I./include/type-checking -I./include/reachability -I./include/code-gen -I./include/code-gen/startup -I./include/code-gen/vtable -I./include/code-gen/object-layout -I./include/code-gen/label -I./include/code-gen/inheritance -I./include/code-gen/interface-methods -I./include/code-gen/static
TEST_LIB_PATH=-L build/lib -l joos
TEST_OUT_FILE=test_joosc

# Include
SRC_INC = $(wildcard include/*.h) $(wildcard include/dfas/*.h) $(wildcard include/weeds/*.h) $(wildcard include/AST/*.h) $(wildcard include/sym-table/*.h) $(wildcard include/type-link/*.h) $(wildcard include/hierarchy-checking/*.h) $(wildcard include/disambiguate/*.h) $(wildcard include/type-checking/*.h) $(wildcard include/reachability/*.h) $(wildcard include/code-gen/*.h) $(wildcard include/code-gen/startup/*.h) $(wildcard include/code-gen/vtable/*.h) $(wildcard include/code-gen/object-layout/*.h) $(wildcard include/code-gen/label/*.h) $(wildcard include/code-gen/inheritance/*.h) $(wildcard include/code-gen/interface-methods/*.h) $(wildcard include/code-gen/static/*.h)
TEST_INC = $(wildcard tests/include/*.h)

# Static Lib for Tests
LIB_O = $(filter-out build/src/main.o, $(SRC_O))
JOOSLIB = build/lib/libjoos.a

.PHONY: all
all: init $(OUT_FILE) $(TEST_OUT_FILE)

.PHONY: compiler
compiler: init $(OUT_FILE)

.PHONY: tests
tests: init $(TEST_OUT_FILE)

.PHONE: init
init:
	@mkdir -p $(BUILD_DIR) $(BUILD_DIR)/dfas $(BUILD_DIR)/AST $(BUILD_DIR)/sym-table $(BUILD_DIR)/type-link build/tests build/lib $(BUILD_DIR)/hierarchy-checking $(BUILD_DIR)/disambiguate $(BUILD_DIR)/type-checking $(BUILD_DIR)/reachability $(BUILD_DIR)/code-gen $(BUILD_DIR)/code-gen/startup $(BUILD_DIR)/code-gen/vtable $(BUILD_DIR)/code-gen/object-layout $(BUILD_DIR)/code-gen/label $(BUILD_DIR)/code-gen/inheritance $(BUILD_DIR)/code-gen/interface-methods $(BUILD_DIR)/code-gen/static
	@$(foreach TEST_CASE, $(TEST_CASES), python Extras/Scripts/listTestFiles.py tests/$(TEST_CASE)/ tests/include/$(TEST_CASE)TestFiles.h  tests/src/$(TEST_CASE)TestFiles.cpp $(TEST_CASE);)
	# @python Extras/Scripts/readParserRulesAndTable.py Extras/Grammar/grammarAndParseTable.txt include/parserRules.h src/parserRules.cpp include/parserActions.h src/parserActions.cpp
ifneq ($(CODE_OUT),)
	@mkdir -p $(CODE_OUT)
endif

# Main Compiler
build/src/AST/%.o: src/AST/%.cpp $(SRC_INC)
	$(CC) $(CFLAGS) $< -o $(BUILD_DIR)/AST/$(notdir $(@:.cpp=.o))

build/src/dfas/%.o: src/dfas/%.cpp $(SRC_INC)
	$(CC) $(CFLAGS) $< -o $(BUILD_DIR)/dfas/$(notdir $(@:.cpp=.o))

build/src/sym-table/%.o: src/sym-table/%.cpp $(SRC_INC)
	$(CC) $(CFLAGS) $< -o $(BUILD_DIR)/sym-table/$(notdir $(@:.cpp=.o))

build/src/type-link/%.o: src/type-link/%.cpp $(SRC_INC)
	$(CC) $(CFLAGS) $< -o $(BUILD_DIR)/type-link/$(notdir $(@:.cpp=.o))

build/src/hierarchy-checking/%.o: src/hierarchy-checking/%.cpp $(SRC_INC)
	$(CC) $(CFLAGS) $< -o $(BUILD_DIR)/hierarchy-checking/$(notdir $(@:.cpp=.o))

build/src/disambiguate/%.o: src/disambiguate/%.cpp $(SRC_INC)
	$(CC) $(CFLAGS) $< -o $(BUILD_DIR)/disambiguate/$(notdir $(@:.cpp=.o))

build/src/type-checking/%.o: src/type-checking/%.cpp $(SRC_INC)
	$(CC) $(CFLAGS) $< -o $(BUILD_DIR)/type-checking/$(notdir $(@:.cpp=.o))

build/src/reachability/%.o: src/reachability/%.cpp $(SRC_INC)
	$(CC) $(CFLAGS) $< -o $(BUILD_DIR)/reachability/$(notdir $(@:.cpp=.o))

build/src/code-gen/startup/%.o: src/code-gen/startup/%.cpp $(SRC_INC)
	$(CC) $(CFLAGS) $< -o $(BUILD_DIR)/code-gen/startup/$(notdir $(@:.cpp=.o))

build/src/code-gen/vtable/%.o: src/code-gen/vtable/%.cpp $(SRC_INC)
	$(CC) $(CFLAGS) $< -o $(BUILD_DIR)/code-gen/vtable/$(notdir $(@:.cpp=.o))

build/src/code-gen/object-layout/%.o: src/code-gen/object-layout/%.cpp $(SRC_INC)
	$(CC) $(CFLAGS) $< -o $(BUILD_DIR)/code-gen/object-layout/$(notdir $(@:.cpp=.o))

build/src/code-gen/label/%.o: src/code-gen/label/%.cpp $(SRC_INC)
	$(CC) $(CFLAGS) $< -o $(BUILD_DIR)/code-gen/label/$(notdir $(@:.cpp=.o))

build/src/code-gen/inheritance/%.o: src/code-gen/inheritance/%.cpp $(SRC_INC)
	$(CC) $(CFLAGS) $< -o $(BUILD_DIR)/code-gen/inheritance/$(notdir $(@:.cpp=.o))

build/src/code-gen/interface-methods/%.o: src/code-gen/interface-methods/%.cpp $(SRC_INC)
	$(CC) $(CFLAGS) $< -o $(BUILD_DIR)/code-gen/interface-methods/$(notdir $(@:.cpp=.o))

build/src/code-gen/static/%.o: src/code-gen/static/%.cpp $(SRC_INC)
	$(CC) $(CFLAGS) $< -o $(BUILD_DIR)/code-gen/static/$(notdir $(@:.cpp=.o))

build/src/code-gen/%.o: src/code-gen/%.cpp $(SRC_INC)
	$(CC) $(CFLAGS) $< -o $(BUILD_DIR)/code-gen/$(notdir $(@:.cpp=.o))

build/src/%.o: src/%.cpp $(SRC_INC)
	$(CC) $(CFLAGS) $< -o $(BUILD_DIR)/$(notdir $(@:.cpp=.o))

$(OUT_FILE): $(SRC_O)
	$(CC) $^ -o $(OUT_FILE)

# Test Code
$(JOOSLIB): $(LIB_O)
	ar rvs $(JOOSLIB) $(LIB_O)

build/tests/%.o: tests/src/%.cpp $(JOOSLIB) $(TEST_INC)
	$(CC) $(TEST_CFLAGS) $< -o build/tests/$(notdir $(@:.cpp=.o))

$(TEST_OUT_FILE): $(TEST_O)
	$(CC) $(TEST_O) $(TEST_LIB_PATH) -o $(TEST_OUT_FILE)

# Clean
.PHONY: clean
clean:
	@rm -f $(SRC_O) $(JOOSLIB) $(OUT_FILE) $(TEST_O) $(TEST_OUT_FILE) *.s

#debug print
print-%  : ; @echo $* = $($*)
