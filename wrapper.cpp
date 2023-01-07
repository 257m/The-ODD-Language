#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Type.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Support/ErrorHandling.h"
using namespace llvm;
#include "wrapper.h"

bool BlockhasNPredecessors(LLVMBasicBlockRef block,unsigned int N)
{
	return unwrap(block)->hasNPredecessors(N);
}

bool BlockhasNPredecessorsOrMore(LLVMBasicBlockRef block,unsigned int N)
{
	return unwrap(block)->hasNPredecessorsOrMore(N);
}

LLVMTypeRef GetReturnTypeOfBlockParent(LLVMBasicBlockRef block)
{
	return wrap(unwrap(block)->getParent()->getReturnType());
}

unsigned int GetIntegerBitWidth(LLVMTypeRef integer)
{
	return unwrap(integer)->getIntegerBitWidth();
}
