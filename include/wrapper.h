#ifndef WRAPPER_H
#define WRAPPER_H

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

EXTERNC bool BlockhasNPredecessors(LLVMBasicBlockRef block,unsigned int N);
EXTERNC bool BlockhasNPredecessorsOrMore(LLVMBasicBlockRef block,unsigned int N);
EXTERNC LLVMTypeRef GetReturnTypeOfBlockParent(LLVMBasicBlockRef block);
EXTERNC unsigned int GetIntegerBitWidth(LLVMTypeRef integer);

#undef EXTERNC
#endif /* WRAPPER_H */