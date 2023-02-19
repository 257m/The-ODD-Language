#include "../include/common.h"
#include "../include/codegen.h"
#include "../include/handlers.h"

FILE* input_file;

int main(int argc, char** argv)
{
	char* output_filename = NULL;
	if (argc < 2) {
		fprintf(stderr, "ERROR: No arguments given to compiler\n");
		exit(-1);
	}
	if (argc == 2) {
		output_filename = "a.ll";
	}
	else {
		output_filename = alloca(strlen(argv[2]) + 1);
		strcpy(output_filename, argv[2]);
		/* Implementation of compile flags will be done one day
		if (argc > 3) {
			for (int i = 0;i < argc-3;i++) {

			}
		}*/
	}
	input_file = fopen(argv[1], "r");

	LLVMModuleRef module =
		LLVMModuleCreateWithName(argv[1]); // Creates LLVM module
	/*
	Note on definiton: Modules represent the top-level structure in an LLVM
	program. An LLVM module is effectively a translation unit or a collection of
	translation units merged together.
	*/
	LLVMBuilderRef builder = LLVMCreateBuilder();
	LLVMExecutionEngineRef engine;

	LLVMInitializeNativeTarget();
	LLVMLinkInMCJIT();
	LLVMInitializeNativeAsmPrinter();
	LLVMInitializeNativeAsmParser();

	// Create execution engine.
	char* msg;
	if (LLVMCreateExecutionEngineForModule(&engine, module, &msg) == 1) {
		fprintf(stderr, "%s\n", msg);
		LLVMDisposeMessage(msg);
		return 1;
	}

	// Setup optimizations.
	LLVMPassManagerRef pass_manager =
		LLVMCreateFunctionPassManagerForModule(module);
	LLVMAddInstructionCombiningPass(pass_manager);
	LLVMAddReassociatePass(pass_manager);
	LLVMAddGVNPass(pass_manager);
	LLVMAddCFGSimplificationPass(pass_manager);
	LLVMInitializeFunctionPassManager(pass_manager);

	getNextToken(); // Get first token from file
	MainLoop(
		module, builder, pass_manager, engine); // run the parser and codegen

	if (LLVMPrintModuleToFile(module, output_filename, &msg) == 1) {
		fprintf(stderr, "%s\n", msg);
		LLVMDisposeMessage(msg);
		return 1;
	}

	LLVMDisposePassManager(pass_manager);
	LLVMDisposeBuilder(builder);
	LLVMDisposeModule(module);
	return 0;
}