#include "../include/parser.h"
#include "../include/codegen.h"
#include "../include/handlers.h"

// Handlers //
void HandleDefinition(LLVMModuleRef module, LLVMBuilderRef builder,
	LLVMPassManagerRef pass_manager)
{
	ExprAST Definition = ParseExpression();
	if (!Definition.pointer) {
		fprintf(stderr, "ERROR with Definition\n");
		// Skip token for error recovery.
		getNextToken();
	}
	else {
		if (glass_house_mode) {
			printf("Parsed a definition\n");
			ExpressionPrint(&Definition);
			printf("\n");
		}
		LLVMValueRef value = codegen(&Definition, module, builder, NULL, NULL);
		if (value == NULL) {
			fprintf(
				stderr, "Failed to generate code for function\nExiting...\n");
			exit(-1);
		}
		if (glass_house_mode)
			LLVMDumpValue(value);
		LLVMRunFunctionPassManager(pass_manager, value);
		ExprAST_Free_Value(&Definition);
	}
}

void HandleExtern(LLVMModuleRef module, LLVMBuilderRef builder)
{
	ExprAST Definition = ParseExtern();
	if (!Definition.pointer) {
		fprintf(stderr, "ERROR with Extern\n");
		// Skip token for error recovery.
		getNextToken();
	}
	else {
		if (glass_house_mode) {
			printf("Parsed a extern\n");
			printf("Extern is...\n");
			ExpressionPrint(&Definition);
			printf("\n");
		}
		LLVMValueRef value = codegen(&Definition, module, builder, NULL, NULL);
		if (value == NULL) {
			fprintf(stderr, "Failed to generate code for extern\nExiting...\n");
			exit(-1);
		}
		if (glass_house_mode)
			LLVMDumpValue(value);
		ExprAST_Free_Value(&Definition);
	}
}

void HandleTopLevelExpression(LLVMModuleRef module,
	LLVMBuilderRef builder, LLVMPassManagerRef pass_manager,
	LLVMExecutionEngineRef engine)
{
	FunctionAST* Func = ParseTopLevelExpr();
	if (!Func) {
		fprintf(stderr, "ERROR with Top Level Expression\n");
		// Skip token for error recovery.
		getNextToken();
	}
	else {
		if (glass_house_mode) {
			printf("Parsed a Top Level Expression\n");
			ProtoPrint(&Func->Proto);
			ExpressionPrint(&Func->Body);
			printf("\n");
		}
		LLVMValueRef value = codegen_function(
			&((ExprAST){Func, Function}), module, builder, LLVMDoubleType());
		if (value == NULL) {
			printf("Failed to generate code for top level "
				   "expression\nExiting...\n");
			exit(-1);
		}
		LLVMRunFunctionPassManager(pass_manager, value);
		if (glass_house_mode) {
			LLVMDumpValue(value);
			void* fp = LLVMGetPointerToGlobal(engine, value);
			double (*FP)() = (double (*)())(intptr_t)fp;
			fprintf(stderr, "Evaluted to %f\n", FP());
		}
		ExprAST_Free_Value(&((ExprAST){Func, Function}));
	}
}

int MainLoop(LLVMModuleRef module, LLVMBuilderRef builder,
	LLVMPassManagerRef pass_manager, LLVMExecutionEngineRef engine)
{
	while (true) {
		if (CurTok <= tok_int8 && CurTok >= tok_unsigned) {
			HandleDefinition(module, builder, pass_manager);
			continue;
		}
		switch (CurTok) {
			case tok_eof:
				if (glass_house_mode)
					printf("End of file\n");
				return 0;
			case ';': // ignore top-level semicolons.
				getNextToken();
				break;
			case tok_extern:
				HandleExtern(module, builder);
				break;
			default:
				HandleTopLevelExpression(module, builder, pass_manager, engine);
				break;
		}
	}
	return 1;
}