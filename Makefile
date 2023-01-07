all: compile_c

CC = clang
OC = codd
COM_FILE = test
C_FILE = libodd/object_files/oddio
OUTPUT_FILE = single

compile_c:
	clang -I/nix/store/ny8vpd071f90p066bbd68skbnjwd297h-llvm-13.0.0-rc1-dev/include -std=c99 -fno-exceptions -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -g -o $(OC).o -c $(OC).c
	clang -L/nix/store/lai4qa79hcqb7ag18aiw4raybqp0hj6v-llvm-13.0.0-rc1-lib/lib wrapper.o $(OC).o -o $(OC) -lLLVM-13git -lm
	mv ./codd ./bin

calr:
	$(OC) $(COM_FILE).odd $(COM_FILE).ll
	opt -S -O3 $(COM_FILE).ll -o opt.ll
	llc -filetype=obj opt.ll -o $(COM_FILE).o
	clang -o $(COM_FILE) $(COM_FILE).o $(C_FILE).o -lm
	rm rf -f $(COM_FILE).o
	./$(COM_FILE)

temp:
	llc -filetype=obj temp.ll -o temp.o
	clang -o temp temp.o $(C_FILE).o
	rm rf -f temp.o
	./temp

comp_wrap:
	clang++ -c wrapper.cpp `llvm-config --cppflags` -o wrapper.o

clean:
	rm -f main

vim:
	export VIMINIT='source .vimrc'

VGCORES = $(shell find -type f -name '*vgcore*')

clean_vg:
	rm -rf $(VGCORES)

format:
	clang-format -i -style=file $(OC).c