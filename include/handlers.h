#ifndef HANDLERS_H
#define HANDLERS_H

#include "common.h"

void HandleDefinition(LLVMModuleRef module, LLVMBuilderRef builder,
	LLVMPassManagerRef pass_manager);
void HandleExtern(LLVMModuleRef module, LLVMBuilderRef builder);
void HandleTopLevelExpression(LLVMModuleRef module,
	LLVMBuilderRef builder, LLVMPassManagerRef pass_manager,
	LLVMExecutionEngineRef engine);
int MainLoop(LLVMModuleRef module, LLVMBuilderRef builder,
	LLVMPassManagerRef pass_manager, LLVMExecutionEngineRef engine);

#endif /* HANDLERS_H */