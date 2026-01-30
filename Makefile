SRC_DIR			:= src/
BIN_DIR			:= bin/
OBJ_DIR			:= bin/obj/

CC				:= g++
CC_FLAGS		:= -std=c++20 -g -Wall -O3 -D NDEBUG
CC_INCLUDE		:= -I inc -I lib/inc

LD				:= g++
LD_FLAGS		:= -g
LD_INCLUDE		:= -lglfw

DEP_FLAGS		:= -MMD -MP

CC_FILES_IN		:= $(SRC_DIR)core.cpp $(SRC_DIR)native.cpp $(SRC_DIR)nodes.cpp $(SRC_DIR)terminal.cpp $(SRC_DIR)util.cpp $(SRC_DIR)widgets.cpp $(SRC_DIR)window.cpp
CC_FILES_OUT	:= $(patsubst $(SRC_DIR)%.cpp, $(OBJ_DIR)%.o, $(CC_FILES_IN)) $(OBJ_DIR)glad.o
CC_FILES_DEP	:= $(patsubst $(SRC_DIR)%.cpp, $(OBJ_DIR)%.d, $(CC_FILES_IN))

EXE_OUT			:= $(BIN_DIR)strn-test
LIB_OUT			:= $(BIN_DIR)libstrn.a

.PHONY: clean $(BIN_DIR) $(OBJ_DIR)

all: library execute

$(OBJ_DIR)%.o: $(SRC_DIR)%.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling" $< to $@
	@$(CC) $(CC_FLAGS) $(CC_INCLUDE) $(DEP_FLAGS) -c $< -o $@

$(OBJ_DIR)glad.o: lib/src/glad.c
	@mkdir -p $(dir $@)
	@echo "Compiling" $< to $@
	@$(CC) $(CC_FLAGS) $(CC_INCLUDE) $(DEP_FLAGS) -c $< -o $@

-include $(CC_FILES_DEP) $(OBJ_DIR)glad.d $(OBJ_DIR)main.d

$(EXE_OUT): main.cpp $(LIB_OUT)
	@echo "Compiling" $@
	@$(CC) -o $(EXE_OUT) main.cpp -std=c++20 -g -Wall -O3 -D NDEBUG -I inc -L$(BIN_DIR) -lstrn -lglfw

$(LIB_OUT): $(CC_FILES_OUT)
	@echo "Linking" $@
	@#@ld -r -o $(BIN_DIR)libstrn.o $^ -static $(LD_INCLUDE)
	@#@$(LD) -shared -nostartfiles $(LD_FLAGS) -o $(BIN_DIR)libstrn.o $^ $(LD_INCLUDE)
	@ar rcs $@ $^

test: $(EXE_OUT)

library: $(LIB_OUT)

build: library test

execute: test
	@$(EXE_OUT)

clean:
	@rm -r $(BIN_DIR)
	