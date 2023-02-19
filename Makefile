# This is the makefile for our project

# This defines the compiler we are using
CC = clang
OC = codd
COM_FILE = test
C_FILE = libodd/object_files/oddio

# These are the flags we are going to pass to the compiler
CFLAGS = -I/nix/store/ny8vpd071f90p066bbd68skbnjwd297h-llvm-13.0.0-rc1-dev/include -g
LFLAGS = -L/nix/store/lai4qa79hcqb7ag18aiw4raybqp0hj6v-llvm-13.0.0-rc1-lib/lib obj/wrapper.o -lLLVM-13git -lm

# This is the name of the executable we are going to create
EXE = codd

# This is the directory where our object files will be stored
OBJ_DIR = obj

# This is the directory where our assembly files will be stored
ASM_DIR = asm

# This is the list of all source files
SRC = $(wildcard src/*.c)

# This is the list of object files that we need to create
OBJ = $(patsubst src/%.c, $(OBJ_DIR)/%.o, $(SRC))

# This is the list of assembly files that we need to create
ASM = $(patsubst src/%.c, $(ASM_DIR)/%.s, $(SRC))

# This is the default target that is going to be built when we run 'make'
all: $(EXE)

# This is the rule to build the executable
$(EXE): $(OBJ)
		$(CC) $(LFLAGS) $^ -o bin/$@

# This is the rule to build the object files
$(OBJ_DIR)/%.o: src/%.c
		$(CC) -c $(CFLAGS) $< -o $@

# This is the rule to build assembly files
assembly: $(ASM)
		$($(ASM_DIR)/$<)

$(ASM_DIR)/%.s: src/%.c
		$(CC) $(CFLAGS) -S -fverbose-asm $< -o $@

comp_wrap:
	clang++ -c src/wrapper.cpp `llvm-config --cppflags` -o obj/wrapper.o

# This target is used to clean up the project directory
clean:
		rm -f $(OBJ_DIR)/*.o $(EXE)

format:
		clang-format -i -style=file $(SRC)

add_obj:
	mkdir -p obj

calr:
	$(OC) $(COM_FILE).odd $(COM_FILE).ll
	opt -S -O3 $(COM_FILE).ll -o opt.ll
	llc -filetype=obj opt.ll -o $(COM_FILE).o
	clang -o $(COM_FILE) $(COM_FILE).o $(C_FILE).o -lm
	rm rf -f $(COM_FILE).o
	./$(COM_FILE)