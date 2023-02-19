#ifndef AST_H
#define AST_H

#include "common.h"
#include "tokenizer.h"
#include "vec.h"

typedef struct {
	double Value;
} NumberExprAST;

typedef struct {
	char Name[IDENTIFIER_MAX];
} VariableExprAST;

typedef struct {
	char Callee[IDENTIFIER_MAX];
	vector Args;
} CallExprAST;

typedef struct {
	char Name[IDENTIFIER_MAX];
	vector Args;
} PrototypeAST;

typedef struct {
	void* pointer;
	int type;
} ExprAST;

enum AST_Types {
	NoType,
	Number,
	Variable,
	Call,
	Prototype,
	Function,
	Binary,
	If_Statement,
	Block,
	Return,
	Goto,
	Label,
	Declaration,
	Void,
	Unary,
	Cast,
};

typedef struct {
	PrototypeAST Proto;
	ExprAST Body;
} FunctionAST;

typedef struct {
	ExprAST LHS;
	ExprAST RHS;
	char Op;
} BinaryExprAST;

typedef struct {
	ExprAST Value;
	char Op;
} UnaryExprAST;

typedef struct {
	ExprAST condition;
	ExprAST true_expr;
	ExprAST false_expr;
} IfExprAST;

typedef struct {
	ExprAST* Expressions;
	size_t size;
} BlockExprAST;

typedef struct {
	ExprAST Expression;
} ReturnExprAST;

typedef struct {
	char Name[IDENTIFIER_MAX];
} GotoExprAST;

typedef struct {
	ExprAST Identity;
	int8_t Type;
	uint8_t Indirection;
} DeclareExprAST;

typedef struct {
	ExprAST Value;
	int8_t Type;
	uint8_t Indirection;
} CastExprAST;

typedef struct {
	char* name;
	LLVMValueRef value;
	UT_hash_handle hh;
	bool isparam;
} NamedValue;

void ExprAST_Free(ExprAST* Expr);
void ExprAST_Free_Value(ExprAST* Expr);
void ProtoPrint(PrototypeAST* Proto);
void CallPrint(CallExprAST* FuncCall);
void ExpressionPrint(ExprAST* Expr);
int GetTokPrecedence();

#endif /* AST_H */