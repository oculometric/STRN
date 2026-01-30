SRC_DIR			:= src/
BIN_DIR			:= bin/
OBJ_DIR			:= bin/obj/

CC				:= g++
CC_FLAGS		:= -std=c++20 -g -Wall -O3 -D NDEBUG
CC_INCLUDE		:= 

LD				:= g++
LD_FLAGS		:= -g
LD_INCLUDE		:= -lglfw

DEP_FLAGS		:= -MMD -MP

CC_FILES_IN		:= $(SRC_DIR)core.cpp $(SRC_DIR)native.cpp $(SRC_DIR)node.cpp $(SRC_DIR)terminal.cpp $(SRC_DIR)util.cpp $(SRC_DIR)widgets.cpp $(SRC_DIR)window.cpp
CC_FILES_OUT	:= $(patsubst $(SRC_DIR)%.cpp, $(OBJ_DIR)%.o, $(CC_FILES_IN))
CC_FILES_DEP	:= $(patsubst $(SRC_DIR)%.cpp, $(OBJ_DIR)%.d, $(CC_FILES_IN))

EXE_OUT			:= $(BIN_DIR)strn-test
LIB_OUT			:= $(BIN_DIR)strn.a

.PHONY: clean $(BIN_DIR) $(OBJ_DIR)

all: library execute

$(OBJ_DIR)%.o: $(SRC_DIR)%.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling" $< to $@
	@$(CC) $(CC_FLAGS) $(CC_INCLUDE) $(DEP_FLAGS) -c $< -o $@

-include $(CC_FILES_DEP) $(CC_FILES_DEP_PB)

$(EXE_OUT): $(CC_FILES_OUT) $(OBJ_DIR)main.o
	@echo "Linking" $@
	@$(LD) $(LD_FLAGS) -o $@ $< $(LD_INCLUDE)

$(LIB_OUT): $(CC_FILES_OUT)
	@echo "Linking" $@
	@ar rcs $@ $<

test: $(EXE_OUT)

library: $(LIB_OUT)

build: library test

execute: test
	@$(EXE_OUT)

clean:
	@rm -r $(BIN_DIR)
	