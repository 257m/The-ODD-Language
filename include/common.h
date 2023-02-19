#ifndef COMMON_H
#define COMMON_H

#include <ctype.h>
#include <inttypes.h>
#include <malloc.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <llvm-c/Analysis.h>
#include <llvm-c/BitReader.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Transforms/Scalar.h>

#include "../include/uthash.h"
#include "../include/wrapper.h"

#define glass_house_mode 1

extern FILE* input_file;

#endif /* COMMON_H */