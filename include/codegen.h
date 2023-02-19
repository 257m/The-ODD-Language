#ifndef CODEGEN_H
#define CODEGEN_H

#include "common.h"
#include "ast.h"

LLVMTypeRef ReturnPointerizedTypeIfIndirect(
	LLVMTypeRef type, uint8_t Indirection);
LLVMTypeRef get_data_type(DeclareExprAST* Expr);
char* get_expr_name(ExprAST* Expr);
LLVMValueRef codegen_number(ExprAST* Expr, ExprAST* Parent);
LLVMValueRef codegen_variable(ExprAST* Expr);
bool IsVariableParam(ExprAST* Expr);
LLVMValueRef GetZeroConstOfType(LLVMTypeRef Type);
LLVMOpcode get_cast_opcode(LLVMTypeRef source, LLVMTypeRef dest);
inline bool IsFloatVariant(LLVMTypeKind typekind);
LLVMValueRef codegen_binary_expr(
	ExprAST* Expr, LLVMModuleRef module, LLVMBuilderRef builder);
LLVMValueRef codegen_call(
	ExprAST* Expr, LLVMModuleRef module, LLVMBuilderRef builder);
LLVMValueRef codegen_prototype(
	ExprAST* Expr, LLVMModuleRef module, LLVMTypeRef Return_Type);
LLVMValueRef codegen_function(ExprAST* Expr, LLVMModuleRef module,
	LLVMBuilderRef builder, LLVMTypeRef Return_Type);
LLVMValueRef codegen_if_expr(ExprAST* Expr, LLVMModuleRef module,
	LLVMBuilderRef builder, LLVMBasicBlockRef block);
LLVMValueRef codegen_block(ExprAST* Expr, LLVMModuleRef module,
	LLVMBuilderRef builder, LLVMBasicBlockRef block);
LLVMValueRef codegen_return(ExprAST* Expr, LLVMModuleRef module,
	LLVMBuilderRef builder, LLVMBasicBlockRef block);
LLVMValueRef codegen_goto(
	ExprAST* Expr, LLVMBuilderRef builder, LLVMBasicBlockRef block);
LLVMValueRef codegen_label(ExprAST* Expr, LLVMBuilderRef builder);
LLVMValueRef codegen_declaration(
	ExprAST* Expr, LLVMModuleRef module, LLVMBuilderRef builder);
LLVMValueRef codegen_unary(
	ExprAST* Expr, LLVMModuleRef module, LLVMBuilderRef builder);
LLVMValueRef codegen_cast(ExprAST* Expr, LLVMModuleRef module, LLVMBuilderRef builder);
LLVMValueRef codegen(ExprAST* Expr, LLVMModuleRef module,
	LLVMBuilderRef builder, LLVMBasicBlockRef block, ExprAST* Parent);

#define depointerize(pointer)                                                  \
	{                                                                          \
		if (LLVMGetTypeKind(LLVMTypeOf(pointer)) == LLVMPointerTypeKind)       \
			pointer = LLVMBuildLoad(builder, pointer, "depoint");              \
	}

#define getvalue(Expr, value)                                                  \
	{                                                                          \
		if (Expr.type == Variable)                                             \
			if (!IsVariableParam(&Expr))                                       \
				value = LLVMBuildLoad(builder, value, "depoint");              \
	}

#define getvalue2(Expr, value)                                                 \
	{                                                                          \
		if (Expr->type == Variable)                                            \
			if (!IsVariableParam(Expr))                                        \
				value = LLVMBuildLoad(builder, value, "depoint");              \
	}

#endif /* CODEGEN_H */