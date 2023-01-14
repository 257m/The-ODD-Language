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

#include "uthash.h"
#include "wrapper.h"

#define glass_house_mode 1

FILE* input_file;

void writeData(char* dest, char* data, size_t data_size)
{
	for (int i = 0; i < data_size; i++) {
		dest[i] = data[i];
	}
}

#define cast(var, type) (type) var

typedef struct {
	char* data;
	size_t size;
} vector;

#define vector_new_alloc(vec, size)                                            \
	{                                                                          \
		vec.data = malloc(size);                                               \
		vec.size = size;                                                       \
	}

void vector_append(vector* vec, void* data, size_t append_size)
{
	if (vec->size)
		vec->data = realloc(vec->data, vec->size + append_size);
	else
		vec->data = malloc(append_size);

	writeData(vec->data + vec->size, data, append_size);
	vec->size += append_size;
}

// The lexer returns tokens [0-255] if it is an unknown character, otherwise one
// of these for known things.
enum Token {
	tok_eof = -1,

	// commands
	tok_def = -2,
	tok_extern = -3,
	tok_return = -4,
	tok_goto = -5,

	// primary
	tok_identifier = -6,
	tok_number = -7,

	// control
	tok_if = -8,
	tok_else = -9,

	// multi character comparison operators
	tok_dequ = -10,
	tok_nequ = -11,
	tok_lte = -12,
	tok_gte = -13,
	tok_dand = -14,
	tok_dor = -15,

	// data types keywords
	tok_int8 = -18,
	tok_int16 = -19,
	tok_int32 = -20,
	tok_int64 = -21,
	tok_int128 = -22,
	tok_float = -23,
	tok_double = -24,
	tok_quad = -25,
	tok_bool = -26,
	tok_void = -27,
	tok_unsigned = -28,
};

#define IDENTIFIER_MAX 32
#define NUMBER_MAX 384

char IdentifierStr[IDENTIFIER_MAX]; // Filled in if tok_identifier
double NumVal;						// Filled in if tok_number

/* Custom String Functions */
bool string_compare(const char* str1, const char* str2)
{
	int charnum = 0;

	while (str1[charnum] == str2[charnum]) {
		if (str1[charnum] == '\0' || str2[charnum] == '\0')
			break;
		charnum++;
	}

	if (str1[charnum] == '\0' && str2[charnum] == '\0')
		return 1;

	return 0;
}

char* CharToNull(char* string, char* output, char c)
{
	unsigned int charnum = 0;
	while (string[charnum] != '\0') {
		if (string[charnum] != c) {
			output[charnum] = string[charnum];
			charnum++;
		}
		else {
			output[charnum] = '\0';
			return output;
		}
	}
	return output;
}

void str_append(char* string, char c)
{
	unsigned int charnum = 0;

	while (string[charnum] != '\0')
		charnum++;

	string[charnum] = c;
}

void SetStringToChar(char* string, char c)
{
	unsigned int charnum = 0;
	string[charnum++] = c;
	while (string[charnum] != '\0')
		string[charnum++] = '\0';
}

int check_for_type(char* identifier)
{
	if (identifier[0] == 'i') {
		if (string_compare(identifier + 1, "nt8"))
			return tok_int8;
		if (string_compare(identifier + 1, "nt16"))
			return tok_int16;
		if (string_compare(identifier + 1, "nt32"))
			return tok_int32;
		if (string_compare(identifier + 1, "nt64"))
			return tok_int64;
		if (string_compare(identifier + 1, "nt128"))
			return tok_int128;
		return 0;
	}
	if (identifier[0] == 'f')
		if (string_compare(identifier + 1, "loat"))
			return tok_float;
	if (identifier[0] == 'd')
		if (string_compare(identifier + 1, "ouble"))
			return tok_double;
	if (identifier[0] == 'q')
		if (string_compare(identifier + 1, "uad"))
			return tok_quad;
	if (identifier[0] == 'b')
		if (string_compare(identifier + 1, "ool"))
			return tok_bool;
	if (identifier[0] == 'v')
		if (string_compare(identifier + 1, "oid"))
			return tok_void;
	if (identifier[0] == 'u')
		if (string_compare(identifier + 1, "nsigned"))
			return tok_unsigned;
	return 0;
}

bool isoperator(int op)
{
	return (op == '+') || (op == '-') || (op == '*') || (op == '/') ||
		(op == '=') || (op == tok_nequ) || (op == tok_lte) || (op == tok_gte) ||
		(op == tok_dand) || (op == tok_dor);
}

int CurTok = 0;
uint32_t linenumber = 1;
uint64_t linestart = 0;
// gettok - Return the next token from standard input.
int gettok()
{
	static int LastChar = ' ';

	// Skip any whitespace.
	while (isspace(LastChar)) {
		if (LastChar == '\n') {
			linestart = ftell(input_file);
			linenumber++;
		}
		LastChar = fgetc(input_file);
	}

	if (isalpha(LastChar)) { // identifier: [a-zA-Z][a-zA-Z0-9]*
		SetStringToChar(IdentifierStr, LastChar);
		while (isalnum((LastChar = fgetc(input_file))))
			str_append(IdentifierStr, LastChar);

		if (string_compare(IdentifierStr, "def"))
			return tok_def;
		if (string_compare(IdentifierStr, "extern"))
			return tok_extern;
		if (string_compare(IdentifierStr, "if"))
			return tok_if;
		if (string_compare(IdentifierStr, "else"))
			return tok_else;
		if (string_compare(IdentifierStr, "return"))
			return tok_return;
		if (string_compare(IdentifierStr, "goto"))
			return tok_goto;
		int data_type = check_for_type(IdentifierStr);
		if (data_type != 0)
			return data_type;
		return tok_identifier;
	}

	if (isdigit(LastChar) || LastChar == '.' ||
		(LastChar == '-' && CurTok != tok_number &&
			CurTok != tok_identifier)) { // Number: [0-9.]+
		char NumStr[NUMBER_MAX] = "";

		do {
			str_append(NumStr, LastChar);
			LastChar = fgetc(input_file);
		} while (isdigit(LastChar) || LastChar == '.');

		NumVal = strtod(NumStr, 0);
		return tok_number;
	}

	// Double Operators //
	if (LastChar == '=') {
		LastChar = fgetc(input_file);
		if (LastChar != '=') // checks if it a single operator
			return '=';		 // if so just return the character
		LastChar = fgetc(input_file);
		return tok_dequ; // elsewise we know this is a double operator
	}

	if (LastChar == '!') {
		LastChar = fgetc(input_file);
		if (LastChar != '=')
			return '!';
		LastChar = fgetc(input_file);
		return tok_nequ;
	}

	if (LastChar == '<') {
		LastChar = fgetc(input_file);
		if (LastChar != '=')
			return '<';
		LastChar = fgetc(input_file);
		return tok_lte;
	}

	if (LastChar == '>') {
		LastChar = fgetc(input_file);
		if (LastChar != '=')
			return '>';
		LastChar = fgetc(input_file);
		return tok_gte;
	}

	if (LastChar == '&') {
		LastChar = fgetc(input_file);
		if (LastChar != '&')
			return '&';
		LastChar = fgetc(input_file);
		return tok_dand;
	}

	if (LastChar == '|') {
		LastChar = fgetc(input_file);
		if (LastChar != '|')
			return '|';
		LastChar = fgetc(input_file);
		return tok_dor;
	}

	if (LastChar == '#') {
		// Comment until end of line.
		do
			LastChar = fgetc(input_file);
		while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

		if (LastChar != EOF)
			return gettok(input_file);
	}

	// Check for end of file.  Don't eat the EOF.
	if (LastChar == EOF)
		return tok_eof;

	// character_return:
	// Otherwise, just return the character as its ascii value.
	int ThisChar = LastChar;
	LastChar = fgetc(input_file);
	return ThisChar;
}

#define getNextToken() (CurTok = gettok())

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

void ExprAST_Free_Value(ExprAST* Expr);

void ExprAST_Free(ExprAST* Expr)
{
	switch (Expr->type) {
		case Call:
			free(((CallExprAST*)Expr->pointer)->Args.data);
			break;
		case Prototype:
			free(((PrototypeAST*)Expr->pointer)->Args.data);
			break;
		case Function:
			free(((FunctionAST*)Expr->pointer)->Proto.Args.data);
			ExprAST_Free_Value(&((FunctionAST*)Expr->pointer)->Body);
			break;
		case Binary:
			ExprAST_Free_Value(&((BinaryExprAST*)Expr->pointer)->LHS);
			ExprAST_Free_Value(&((BinaryExprAST*)Expr->pointer)->RHS);
			break;
		case If_Statement:
			ExprAST_Free_Value(&((IfExprAST*)Expr->pointer)->condition);
			ExprAST_Free_Value(&((IfExprAST*)Expr->pointer)->true_expr);
			ExprAST_Free_Value(&((IfExprAST*)Expr->pointer)->false_expr);
			break;
		case Return:
			ExprAST_Free(Expr->pointer);
			break;
		case NoType:
			fprintf(stderr,
				"The Expression your trying to free is of type "
				"NULL\nExiting...");
			exit(-1);
	}
	free(Expr->pointer);
}

void ExprAST_Free_Value(ExprAST* Expr)
{
	switch (Expr->type) {
		case Call:
			free(((CallExprAST*)Expr->pointer)->Args.data);
			return;
		case Prototype:
			free(((PrototypeAST*)Expr->pointer)->Args.data);
			return;
		case Function:
			free(((FunctionAST*)Expr->pointer)->Proto.Args.data);
			ExprAST_Free_Value(&((FunctionAST*)Expr->pointer)->Body);
			return;
		case Binary:
			ExprAST_Free_Value(&((BinaryExprAST*)Expr->pointer)->LHS);
			ExprAST_Free_Value(&((BinaryExprAST*)Expr->pointer)->RHS);
			return;
		case If_Statement:
			ExprAST_Free_Value(&((IfExprAST*)Expr->pointer)->condition);
			ExprAST_Free_Value(&((IfExprAST*)Expr->pointer)->true_expr);
			ExprAST_Free_Value(&((IfExprAST*)Expr->pointer)->false_expr);
			return;
		case Return:
			ExprAST_Free(Expr->pointer);
			return;
		case NoType:
			fprintf(stderr,
				"The Value your trying to free is of type NULL\nExiting...");
			exit(-1);
	}
}

void ExpressionPrint(ExprAST* Expr);

void ProtoPrint(PrototypeAST* Proto)
{
	printf("Name: %s\n", Proto->Name);
	printf("Args:\n");

	for (int i = 0; i < (Proto->Args.size / sizeof(ExprAST)); i++) {
		printf("Arg %d: ", i);
		ExpressionPrint((ExprAST*)Proto->Args.data + (i * sizeof(ExprAST)));
		printf("\n");
	}
}

void CallPrint(CallExprAST* FuncCall)
{
	printf("Name: %s\n", FuncCall->Callee);
	printf("Args:\n");

	for (int i = 0; i < (FuncCall->Args.size / sizeof(ExprAST)); i++) {
		printf("Arg %d: ", i);
		ExpressionPrint(&((ExprAST*)(FuncCall->Args.data))[i]);
		printf("\n");
	}
}

void ExpressionPrint(ExprAST* Expr)
{
	switch (Expr->type) {
		case Number:
			printf("%f", ((NumberExprAST*)Expr->pointer)->Value);
			return;
		case Variable:
			printf("%s", ((VariableExprAST*)Expr->pointer)->Name);
			return;
		case Call:
			CallPrint((CallExprAST*)Expr->pointer);
			return;
		case Prototype:
			ProtoPrint(Expr->pointer);
			return;
		case Function:
			ProtoPrint(&((FunctionAST*)Expr->pointer)->Proto);
			ExpressionPrint(&((FunctionAST*)Expr->pointer)->Body);
			return;
		case Binary:
			ExpressionPrint(&((BinaryExprAST*)Expr->pointer)->LHS);
			printf(" %c ", ((BinaryExprAST*)Expr->pointer)->Op);
			ExpressionPrint(&((BinaryExprAST*)Expr->pointer)->RHS);
			return;
		case If_Statement:
			printf("Cond: ");
			ExpressionPrint(&((IfExprAST*)Expr->pointer)->condition);
			printf("\nIf_true: ");
			ExpressionPrint(&((IfExprAST*)Expr->pointer)->true_expr);
			printf("\nelsewise: ");
			ExpressionPrint(&((IfExprAST*)Expr->pointer)->false_expr);
			return;
		case Block:
			printf("Block:\n");
			for (int i = 0;
				 i < ((BlockExprAST*)Expr->pointer)->size / sizeof(ExprAST);
				 i++) {
				ExpressionPrint(
					&((BlockExprAST*)Expr->pointer)->Expressions[i]);
				printf("\n");
			}
			return;
		case Return:
			printf("return ");
			ExpressionPrint(&((ReturnExprAST*)Expr->pointer)->Expression);
			printf("\n");
			return;
		case Goto:
			printf("goto %s\n", ((GotoExprAST*)Expr->pointer)->Name);
		case Label:
			printf("@%s:\n", ((GotoExprAST*)Expr->pointer)->Name);
		case NoType:
			printf("NULL");
			return;
	}
}

static int GetTokPrecedence()
{
	switch (CurTok) {
		case '+':
			return 20;
		case '-':
			return 20;
		case '*':
			return 40;
		case '/':
			return 40;
		case '%':
			return 15;
		case '<':
			return 10;
		case '>':
			return 10;
		case '&':
			return 20;
		case '|':
			return 20;
		case '^':
			return 20;
		case '=':
			return 1;
		case tok_dequ:
			return 10;
		case tok_nequ:
			return 10;
		case tok_lte:
			return 10;
		case tok_gte:
			return 10;
		case tok_dand:
			return 5;
		case tok_dor:
			return 5;
	}
	return -1;
}

ExprAST ParseExpression();

ExprAST ParseNumberExpr()
{
	ExprAST Result = {.pointer = malloc(sizeof(NumberExprAST)), .type = Number};
	((NumberExprAST*)Result.pointer)->Value = NumVal;
	getNextToken(); // consume the number
	return Result;
}

static ExprAST ParseDeclaration();

ExprAST ParseParenExpr()
{
	getNextToken(); // eat (.
	if (CurTok <= tok_int8 && CurTok >= tok_unsigned)
		return ParseDeclaration();
	ExprAST V = ParseExpression();
	if (!V.pointer)
		return (ExprAST){NULL, NoType};
	getNextToken(); // eat ).
	return V;
}

ExprAST ParseIdentifierExpr()
{
	ExprAST returnCall = {
		.pointer = malloc(sizeof(VariableExprAST)), .type = Variable};

	strcpy(((VariableExprAST*)returnCall.pointer)->Name, IdentifierStr);
	ExprAST Arg = {NULL, 0};

	getNextToken(); // eat identifier.

	if (CurTok != '(') { // Simple variable reference.
		return returnCall;
	}

	returnCall.pointer = realloc(returnCall.pointer, sizeof(CallExprAST));
	returnCall.type = Call;
	((CallExprAST*)(returnCall.pointer))->Args.data = NULL;
	((CallExprAST*)(returnCall.pointer))->Args.size = 0;

	getNextToken(); // eat '('
	if (CurTok != ')') {
		while (true) {
			Arg = ParseExpression();
			if (Arg.pointer)
				vector_append(&((CallExprAST*)(returnCall.pointer))->Args, &Arg,
					sizeof(ExprAST));
			else {
				// If parsing failed scrap the current return call and return
				// NULL for error
				free(((CallExprAST*)(returnCall.pointer))->Args.data);
				free(returnCall.pointer);
				return (ExprAST){NULL, NoType};
			}

			if (CurTok == ')') // If end of arguments found exit the loop
				break;

			getNextToken();
		}
	}

	// if everything was successful eat the ')'.
	getNextToken();
	return returnCall;
}

ExprAST ParseIfExpr()
{
	ExprAST IfExpr = {
		.pointer = malloc(sizeof(IfExprAST)), .type = If_Statement};
	getNextToken();	   // eat the if.
	if (CurTok == '(') // eat the brackets because they are not important
		getNextToken();

	// condition.
	((IfExprAST*)(IfExpr.pointer))->condition = ParseExpression();
	if (!((IfExprAST*)(IfExpr.pointer))->condition.pointer)
		return (ExprAST){NULL, NoType};

	if (CurTok == ')') // eat the closing brackets
		getNextToken();

	((IfExprAST*)(IfExpr.pointer))->true_expr = ParseExpression();
	if (!((IfExprAST*)(IfExpr.pointer))->true_expr.pointer)
		return (ExprAST){NULL, NoType};

	if (CurTok != tok_else) {
		((IfExprAST*)(IfExpr.pointer))->false_expr.pointer = NULL;
		((IfExprAST*)(IfExpr.pointer))->false_expr.type = NoType;
		return IfExpr;
	}

	getNextToken(); // eat the else
	((IfExprAST*)(IfExpr.pointer))->false_expr = ParseExpression();
	if (!((IfExprAST*)(IfExpr.pointer))->false_expr.pointer)
		return (ExprAST){NULL, NoType};

	return IfExpr;
}

ExprAST ParseBlockExpr()
{
	ExprAST returnBlock = {
		.pointer = malloc(sizeof(BlockExprAST)), .type = Block};
	((BlockExprAST*)returnBlock.pointer)->Expressions = NULL;
	((BlockExprAST*)returnBlock.pointer)->size = 0;
	getNextToken(); // eat the '{'
	// printf("Curtok: %d,%s\n",CurTok,IdentifierStr);

	while (1) {
		ExprAST TempExpr = ParseExpression();
		if (TempExpr.pointer == NULL) {
			if (CurTok == ';') {
				getNextToken();
				continue;
			}
			if (CurTok == '}') {
				getNextToken();
				break;
			}
			printf("Line %u:Char %lu: Parse Failed\n", linenumber, ftell(input_file)-linestart);
			printf("CurTok = %d\nIdentifierStr = %s\n",CurTok,IdentifierStr);
			return (ExprAST){NULL, NoType};
		}
		vector_append(returnBlock.pointer, &TempExpr, sizeof(ExprAST));
	}
	return returnBlock;
}

static ExprAST ParseReturnExpr()
{
	getNextToken(); // eat the return
	ExprAST returnReturn = {
		.pointer = malloc(sizeof(ReturnExprAST)), .type = Return};
	if (CurTok == ';') {
		((ReturnExprAST*)returnReturn.pointer)->Expression.pointer = NULL;
		((ReturnExprAST*)returnReturn.pointer)->Expression.type = Void;
		getNextToken(); // eat the ';'
		return returnReturn;
	}
	((ReturnExprAST*)returnReturn.pointer)->Expression = ParseExpression();
	if (!((ReturnExprAST*)returnReturn.pointer)->Expression.pointer) {
		printf("Expression to return is invalid expression!\n");
		return (ExprAST){NULL, NoType};
	}
	return returnReturn;
}

static ExprAST ParseGotoExpr()
{
	getNextToken(); // eat the goto
	if (CurTok != tok_identifier) {
		printf("Label in goto statement is not a identifier!\n");
		return (ExprAST){NULL, NoType};
	}
	ExprAST returnGoto = {malloc(sizeof(GotoExprAST)), Goto};
	strcpy(((GotoExprAST*)returnGoto.pointer)->Name, IdentifierStr);
	getNextToken(); // eat the label name
	return returnGoto;
}

static ExprAST ParseLabelExpr()
{
	getNextToken(); // eat the @
	if (CurTok != tok_identifier)
		return (ExprAST){NULL, NoType};
	ExprAST returnLabel = {malloc(sizeof(GotoExprAST)), Label};
	strcpy(((GotoExprAST*)returnLabel.pointer)->Name, IdentifierStr);
	getNextToken(); // eat the label name
	if (CurTok == ':')
		getNextToken(); // eat the :
	return returnLabel;
}

static PrototypeAST* ParsePrototype()
{
	PrototypeAST* returnProto = malloc(sizeof(PrototypeAST));
	strcpy(returnProto->Name, IdentifierStr);
	returnProto->Args.data = NULL;
	returnProto->Args.size = 0;

	getNextToken(); // eat prototype name.

	if (CurTok == '=' || CurTok == ';' || CurTok == ',' || CurTok == ')' ||
		CurTok == tok_identifier) { // Just a variable declaration.
		free(returnProto);
		return NULL;
	}

	// Check if the prototype args are encapsulated with a '(' at the start
	if (CurTok != '(')
		fprintf(stderr, "Expected \'(\' at the start of a prototype\n");
	else
		getNextToken(); // eat the '('

	ExprAST Arg = {NULL, 0};

	while (1) {
		Arg = ParseExpression();
		if (Arg.pointer == NULL)
			break;
		if (Arg.type == Declaration) { // if identifier
			vector_append(&returnProto->Args, &Arg, sizeof(ExprAST));
		}
		else if (CurTok == ',')
			getNextToken(); // eat the ','
		else
			break;
		// getNextToken(); // eat the identifier or ','
	}

	/*
	Check if the prototype args are encapsulated with a ')' at the end or the
	last arg is a non-identifier
	*/
	if (CurTok != ')')
		fprintf(stderr,
			"Expected \')\' at the end of a prototype\nPrototype Name:%s\n",
			returnProto->Name);
	else
		getNextToken(); // eat the ')'

	return returnProto;
}

static FunctionAST* ParseFunction(PrototypeAST* Proto)
{
	FunctionAST* returnFunc = malloc(sizeof(FunctionAST));
	returnFunc->Proto = *Proto;
	free(Proto);
	returnFunc->Body = ParseExpression();
	if (returnFunc->Body.pointer)
		return returnFunc;
	fprintf(stderr, "Body Parse Failed\n");
	return NULL;
}

static ExprAST ParseDeclaration()
{
	ExprAST returnDeclaration = {
		.pointer = malloc(sizeof(DeclareExprAST)), .type = Declaration};
	((DeclareExprAST*)returnDeclaration.pointer)->Type = 0;
	((DeclareExprAST*)returnDeclaration.pointer)->Indirection = 0;

	if (CurTok == tok_unsigned) {
		((DeclareExprAST*)returnDeclaration.pointer)->Type |= 0x80;
		getNextToken(); // eat the unsigned
	}

	if (CurTok > tok_int8 || CurTok < tok_void) {
		if (glass_house_mode)
			fprintf(stderr,
				"Parse Failed: Invalid type for declaration\n%d\n%s\n", CurTok,
				IdentifierStr);
		return (ExprAST){NULL, NoType};
	}

	((DeclareExprAST*)returnDeclaration.pointer)->Type |= CurTok;
	getNextToken(); // eat the type

	while (CurTok == '*') {
		((DeclareExprAST*)returnDeclaration.pointer)->Indirection++;
		getNextToken();
	}

	if (CurTok == ')') {
		returnDeclaration.type = Cast;
		getNextToken();
		((CastExprAST*)returnDeclaration.pointer)->Value = ParseExpression();
		return returnDeclaration;
	}

	if (CurTok != tok_identifier) {
		fprintf(stderr, "Parse Failed: token after type is not identifier!\n");
		return (ExprAST){NULL, NoType};
	}

	PrototypeAST* Proto;
	if ((Proto = ParsePrototype())) {
		if (CurTok == ';') {
			((DeclareExprAST*)returnDeclaration.pointer)->Identity =
				(ExprAST){Proto, Prototype};
			getNextToken(); // eat the ';'
		}
		else
			((DeclareExprAST*)returnDeclaration.pointer)->Identity =
				(ExprAST){ParseFunction(Proto), Function};
		return returnDeclaration;
	}

	((DeclareExprAST*)returnDeclaration.pointer)->Identity =
		(ExprAST){malloc(sizeof(VariableExprAST)), Variable};
	strcpy(((VariableExprAST*)((DeclareExprAST*)returnDeclaration.pointer)
				   ->Identity.pointer)
			   ->Name,
		IdentifierStr);
	return returnDeclaration;
}

ExprAST ParseUnaryExpr()
{
	ExprAST returnUnary = {
		.pointer = malloc(sizeof(ExprAST)),
		.type = Unary,
	};
	((UnaryExprAST*)returnUnary.pointer)->Op = CurTok;
	getNextToken(); // eat the unary operator
	((UnaryExprAST*)returnUnary.pointer)->Value = ParseExpression();
	return returnUnary;
}

static ExprAST ParsePrimary()
{
	if (CurTok <= tok_int8 && CurTok >= tok_unsigned)
		return ParseDeclaration();
	switch (CurTok) {
		case tok_identifier:
			return ParseIdentifierExpr();
		case tok_number:
			return ParseNumberExpr();
		case '(':
			return ParseParenExpr();
		case tok_if:
			return ParseIfExpr();
		case '{':
			return ParseBlockExpr();
		case tok_return:
			return ParseReturnExpr();
		case tok_goto:
			return ParseGotoExpr();
		case '@':
			return ParseLabelExpr();
		case '*':
		case '&':
			return ParseUnaryExpr();
	}
	// printf("Did not detect anything\nCurTok: %d\n%s\n",CurTok,IdentifierStr);
	return (ExprAST){NULL, NoType};
}

static ExprAST ParseBinOpRHS(int ExprPrec, ExprAST LHS)
{
	ExprAST save_expr;
	// If this is a binop, find its precedence.
	while (true) {
		int TokPrec = GetTokPrecedence();

		// If this is a binop that binds at least as tightly as the current
		// binop, consume it, otherwise we are done.
		if (TokPrec < ExprPrec)
			return LHS;

		// Okay, we know this is a binop.
		int BinOp = CurTok;
		getNextToken(); // eat binop

		// Parse the primary expression after the binary operator.
		ExprAST RHS = ParsePrimary();
		if (!RHS.pointer) {
			fprintf(stderr, "Line %u:Char %ld: BinOp Parse failed: RHS is null!\n",linenumber,ftell(input_file)-linestart);
			return (ExprAST){NULL, NoType};
		}

		// If BinOp binds less tightly with RHS than the operator after RHS, let
		// the pending operator take RHS as its LHS.
		int NextPrec = GetTokPrecedence();
		if (TokPrec < NextPrec) {
			save_expr = RHS;
			RHS = ParseBinOpRHS(TokPrec + 1, RHS);
			if (!RHS.pointer) {
				fprintf(stderr,
					"BinOp Parse failed: RHS is null from child BinOp!\n");
				ExprAST_Free(&save_expr);
				return (ExprAST){NULL, NoType};
			}
		}

		// Merge LHS/RHS into new binop and store the new binop's address in
		// LHS.
		save_expr = LHS;
		LHS.pointer = malloc(sizeof(BinaryExprAST));
		LHS.type = Binary;
		((BinaryExprAST*)LHS.pointer)->LHS = save_expr;
		((BinaryExprAST*)LHS.pointer)->RHS = RHS;
		((BinaryExprAST*)LHS.pointer)->Op = BinOp;
	}
}

ExprAST ParseExpression()
{
	ExprAST LHS = ParsePrimary();
	if (!LHS.pointer) {
		// printf("ParseExpression Error: LHS is NULL!\n");
		return (ExprAST){NULL, NoType};
	}

	return ParseBinOpRHS(0, LHS);
}

static FunctionAST* ParseTopLevelExpr()
{
	ExprAST E = ParseExpression();
	if (E.pointer) {
		// Make an anonymous proto.
		PrototypeAST Proto = {"", {NULL, 0}};
		FunctionAST* Func = malloc(sizeof(FunctionAST));
		Func->Proto = Proto;
		Func->Body = E;
		return Func;
	}
	return NULL;
}

static ExprAST ParseExtern()
{
	getNextToken(); // eat the extern word
	// printf("%s\n",IdentifierStr);
	return ParseDeclaration(); // return the function name and args to handler
}

/*LLVMTypeRef get_data_type(int8_t type)
{
	switch (type | 0x80) {
		case tok_int8:
			return LLVMInt8Type();
		case tok_int16:
			return LLVMInt16Type();
		case tok_int32:
			return LLVMInt32Type();
		case tok_int64:
			return LLVMInt64Type();
		case tok_float:
			return LLVMFloatType();
		case tok_double:
			return LLVMDoubleType();
		case tok_quad:
			return LLVMFP128Type();
		case tok_bool:
			return LLVMInt1Type();
		case tok_void:
			return LLVMVoidType();
	}
	return NULL;
}*/

LLVMTypeRef ReturnPointerizedTypeIfIndirect(
	LLVMTypeRef type, uint8_t Indirection)
{
	if (Indirection)
		return LLVMPointerType(type, Indirection - 1);
	return type;
}

LLVMTypeRef get_data_type(DeclareExprAST* Expr)
{
	switch (Expr->Type | 0x80) {
		case tok_int8:
			return ReturnPointerizedTypeIfIndirect(LLVMInt8Type(), Expr->Indirection);
		case tok_int16:
			return ReturnPointerizedTypeIfIndirect(LLVMInt16Type(), Expr->Indirection);
		case tok_int32:
			return ReturnPointerizedTypeIfIndirect(LLVMInt32Type(), Expr->Indirection);
		case tok_int64:
			return ReturnPointerizedTypeIfIndirect(LLVMInt64Type(), Expr->Indirection);
		case tok_float:
			return ReturnPointerizedTypeIfIndirect(LLVMFloatType(), Expr->Indirection);
		case tok_double:
			return ReturnPointerizedTypeIfIndirect(
				LLVMDoubleType(), Expr->Indirection);
		case tok_quad:
			return ReturnPointerizedTypeIfIndirect(LLVMFP128Type(), Expr->Indirection);
		case tok_bool:
			return ReturnPointerizedTypeIfIndirect(LLVMInt1Type(), Expr->Indirection);
		case tok_void:
			return ReturnPointerizedTypeIfIndirect(LLVMVoidType(), Expr->Indirection);
	}
	return NULL;
}

char* get_expr_name(ExprAST* Expr)
{
	switch (Expr->type) {
		case Variable:
			return ((VariableExprAST*)Expr->pointer)->Name;
		case Prototype:
			return ((PrototypeAST*)Expr->pointer)->Name;
		case Function:
			return ((FunctionAST*)Expr->pointer)->Proto.Name;
		case Declaration:
			return get_expr_name(&((DeclareExprAST*)Expr->pointer)->Identity);
		case Call:
			return ((CallExprAST*)Expr->pointer)->Callee;
		case Label:
			return ((GotoExprAST*)Expr->pointer)->Name;
		case Goto:
			return ((GotoExprAST*)Expr->pointer)->Name;
		default:
			return "null";
	}
}

NamedValue* named_values = NULL;

LLVMValueRef codegen(ExprAST* Expr, LLVMModuleRef module,
	LLVMBuilderRef builder, LLVMBasicBlockRef block, ExprAST* Parent);

LLVMValueRef codegen_number(ExprAST* Expr, ExprAST* Parent)
{
	if (Parent) {
		if (((BinaryExprAST*)Parent->pointer)->Op == '=') {
			if (((BinaryExprAST*)Parent->pointer)->LHS.type == Declaration) {
				LLVMTypeRef dest_type = get_data_type(
					((BinaryExprAST*)Parent->pointer)->LHS.pointer);
				switch (LLVMGetTypeKind(dest_type)) {
					case LLVMPointerTypeKind:
					case LLVMIntegerTypeKind:
						return LLVMConstInt(dest_type,(long long int)((NumberExprAST*)Expr->pointer)->Value,0);
					case LLVMHalfTypeKind:
					case LLVMBFloatTypeKind:
					case LLVMFloatTypeKind:
					case LLVMDoubleTypeKind:
					case LLVMX86_FP80TypeKind:
					case LLVMFP128TypeKind:
					case LLVMPPC_FP128TypeKind: 
						return LLVMConstReal(dest_type, ((NumberExprAST*)Expr->pointer)->Value);
					default:
							LLVMConstInt(dest_type,(long long int)((NumberExprAST*)Expr->pointer)->Value,0);
				}
			}
			else if (((BinaryExprAST*)Parent->pointer)->LHS.type == Variable) {
				NamedValue* value = NULL;
				HASH_FIND_STR(named_values,
					((VariableExprAST*)((BinaryExprAST*)Parent->pointer)
							->LHS.pointer)
						->Name,
					value);
				if (value) {
					LLVMTypeRef dest_type = LLVMTypeOf(value->value);
					switch (LLVMGetTypeKind(dest_type)) {
						case LLVMPointerTypeKind:
						case LLVMIntegerTypeKind:
							return LLVMConstInt(dest_type,(long long int)((NumberExprAST*)Expr->pointer)->Value,0);
						case LLVMHalfTypeKind:
						case LLVMBFloatTypeKind:
						case LLVMFloatTypeKind:
						case LLVMDoubleTypeKind:
						case LLVMX86_FP80TypeKind:
						case LLVMFP128TypeKind:
						case LLVMPPC_FP128TypeKind: 
							return LLVMConstReal(dest_type, ((NumberExprAST*)Expr->pointer)->Value);
						default:
							LLVMConstInt(dest_type,(long long int)((NumberExprAST*)Expr->pointer)->Value,0);
					}
				}
			}
		}
		
		/*ExprAST other;
		bool side = 0;
		if (Expr == ((BinaryExprAST*)Parent->pointer)->LHS)
			other = ((BinaryExprAST*)Parent->pointer)->LHS;
		else {
			other = ((BinaryExprAST*)Parent->pointer)->RHS;
			side = 1;
		}
		if (other.type == Number) {
			if (!side)
				goto default_type;
			
		}*/
	}
	default_type:
	if (floor(((NumberExprAST*)Expr->pointer)->Value) ==
		((NumberExprAST*)Expr->pointer)->Value)
		return LLVMConstInt(LLVMInt32Type(),
			(long long int)((NumberExprAST*)Expr->pointer)->Value, 0);
	return LLVMConstReal(
		LLVMDoubleType(), ((NumberExprAST*)Expr->pointer)->Value);
}

LLVMValueRef codegen_variable(ExprAST* Expr)
{
	NamedValue* value = NULL;
	HASH_FIND_STR(named_values, ((VariableExprAST*)Expr->pointer)->Name, value);
	if (value == NULL)
		return NULL;

	return value->value;
}

bool IsVariableParam(ExprAST* Expr)
{
	NamedValue* value = NULL;
	HASH_FIND_STR(named_values, ((VariableExprAST*)Expr->pointer)->Name, value);
	if (value == NULL)
		return 0;
	return value->isparam;
}

/*LLVMValueRef ArithmeticBinaryExprCast(LLVMBuilderRef builder, LLVMValueRef
lhs, LLVMValueRef rhs, LLVMValueRef (*IntBuild)(LLVMBuilderRef, LLVMValueRef,
LLVMValueRef, const char *), LLVMValueRef (*FloatBuild)(LLVMBuilderRef,
LLVMValueRef, LLVMValueRef, const char *),const char* tmpname)
{
	if (LLVMTypeKind(lhs) == LLVMTypeKind(rhs)) {
		if (LLVMTypeKind(lhs) == LLVMIntegerTypeKind)
			return IntBuild(builder, lhs, rhs, tmpname);
		else
			return FloatBuild(builder, lhs, rhs, tmpname);
	}
	else {
		if (LLVMTypeOf(lhs) != LLVMDoubleType())
			lhs = LLVMBuildUIToFP(builder,lhs,LLVMDoubleType(),"lhstmp");
		else if (LLVMTypeOf(rhs) != LLVMDoubleType())
			rhs = LLVMBuildUIToFP(builder,rhs,LLVMDoubleType(),"rhstmp");
		return FloatBuild(builder, lhs, rhs, tmpname);
	}
}*/

/*LLVMValueRef ComparisonBinaryExprCast(LLVMBuilderRef builder, LLVMIntPredicate
IntPredicate, LLVMRealPredicate FloatPredicate, LLVMValueRef lhs, LLVMValueRef
rhs, LLVMValueRef (*IntBuild)(LLVMBuilderRef, LLVMIntPredicate, LLVMValueRef,
LLVMValueRef, const char *), LLVMValueRef (*FloatBuild)(LLVMBuilderRef,
LLVMRealPredicate, LLVMValueRef, LLVMValueRef, const char *),const char*
tmpname)
{
	if (LLVMTypeOf(lhs) == LLVMTypeOf(rhs)) {
		if (LLVMTypeOf(lhs) == LLVMDoubleType()) {
			return FloatBuild(builder, FloatPredicate, lhs, rhs, tmpname);
		}
		else {
			return IntBuild(builder, IntPredicate, lhs, rhs, tmpname);
		}
	}
	else {
		if (LLVMTypeOf(lhs) != LLVMDoubleType())
			lhs = LLVMBuildUIToFP(builder,lhs,LLVMDoubleType(),"lhstmp");
		else if (LLVMTypeOf(rhs) != LLVMDoubleType())
			rhs = LLVMBuildUIToFP(builder,rhs,LLVMDoubleType(),"rhstmp");
		return FloatBuild(builder, FloatPredicate, lhs, rhs, tmpname);
	}
}*/

LLVMValueRef GetZeroConstOfType(LLVMTypeRef Type)
{
	switch (LLVMGetTypeKind(Type)) {
		case LLVMIntegerTypeKind:
			return LLVMConstInt(Type, 0, 0);
		case LLVMHalfTypeKind:
		case LLVMBFloatTypeKind:
		case LLVMFloatTypeKind:
		case LLVMDoubleTypeKind:
		case LLVMX86_FP80TypeKind:
		case LLVMFP128TypeKind:
		case LLVMPPC_FP128TypeKind:
			return LLVMConstReal(Type, 0);
		case LLVMPointerTypeKind:
			return LLVMConstPointerNull(Type);
		default:
			// Function, Label, or Opaque type?
			printf("Cannot create a null constant of that type!\nType = %d\n",
				LLVMGetTypeKind(Type));
			exit(-1);
	}
	return NULL;
}

LLVMOpcode get_cast_opcode(LLVMTypeRef source, LLVMTypeRef dest)
{
	switch (LLVMGetTypeKind(source)) {
		case LLVMIntegerTypeKind:
			switch (LLVMGetTypeKind(dest)) {
				case LLVMIntegerTypeKind:
					if (GetIntegerBitWidth(source) > GetIntegerBitWidth(dest))
						return LLVMTrunc;
					else if (GetIntegerBitWidth(source) <
						GetIntegerBitWidth(dest))
						return LLVMSExt;
					else
						return 0;
				case LLVMHalfTypeKind:
				case LLVMFloatTypeKind:
				case LLVMDoubleTypeKind:
				case LLVMX86_FP80TypeKind:
				case LLVMFP128TypeKind:
					return LLVMSIToFP;
				case LLVMPointerTypeKind:
					return LLVMIntToPtr;
				default:
					return 0;
			}
		case LLVMHalfTypeKind:
		case LLVMFloatTypeKind:
		case LLVMDoubleTypeKind:
		case LLVMX86_FP80TypeKind:
		case LLVMFP128TypeKind:
			switch (LLVMGetTypeKind(dest)) {
				case LLVMIntegerTypeKind:
					return LLVMFPToSI;
				case LLVMHalfTypeKind:
				case LLVMFloatTypeKind:
				case LLVMDoubleTypeKind:
				case LLVMX86_FP80TypeKind:
				case LLVMFP128TypeKind:
					if (LLVMGetTypeKind(source) < LLVMGetTypeKind(dest))
						return LLVMFPExt;
					else if (LLVMGetTypeKind(source) > LLVMGetTypeKind(dest))
						return LLVMFPTrunc;
					else
						return 0;
				default:
					return 0;
			}
		case LLVMPointerTypeKind:
			switch(LLVMGetTypeKind(dest)) {
				case LLVMIntegerTypeKind:
					return LLVMPtrToInt;
				case LLVMPointerTypeKind:
					if (LLVMGetPointerAddressSpace(dest) != LLVMGetPointerAddressSpace(source))
						return LLVMAddrSpaceCast;
					else
						return 0;
				default:
					return 0;
			}
		default:
			return 0;
	}
}

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

bool IsFloatVariant(LLVMTypeKind typekind)
{
	switch(typekind) {
		case LLVMHalfTypeKind:
		case LLVMFloatTypeKind:
		case LLVMDoubleTypeKind:
		case LLVMX86_FP80TypeKind:
		case LLVMFP128TypeKind:
			return true;
		default:
			return false;
	}
}

LLVMValueRef codegen_binary_expr(
	ExprAST* Expr, LLVMModuleRef module, LLVMBuilderRef builder)
{
	// Evaluate left and right hand values.
	LLVMValueRef lhs;
	LLVMValueRef rhs;
	// this is gonna be a temporary fix
	if (((BinaryExprAST*)Expr->pointer)->Op != '=') {
		lhs = codegen(
			&((BinaryExprAST*)Expr->pointer)->LHS, module, builder, NULL, Expr);
		rhs = codegen(
			&((BinaryExprAST*)Expr->pointer)->RHS, module, builder, NULL, Expr);
	}
	else {
		rhs = codegen(
			&((BinaryExprAST*)Expr->pointer)->RHS, module, builder, NULL, Expr);
		lhs = codegen(
			&((BinaryExprAST*)Expr->pointer)->LHS, module, builder, NULL, Expr);
	}

	// Return NULL if one of the sides is invalid.
	if ((!lhs &&
			(((BinaryExprAST*)Expr->pointer)->Op != '=' &&
				((BinaryExprAST*)Expr->pointer)->LHS.type != Variable)) ||
		!rhs)
		return NULL;

	// Create different IR code depending on the operator.
	switch (((BinaryExprAST*)Expr->pointer)->Op) {
		case '+':
			getvalue(((BinaryExprAST*)Expr->pointer)->LHS, lhs);
			getvalue(((BinaryExprAST*)Expr->pointer)->RHS, rhs);
			if (IsFloatVariant(LLVMGetTypeKind(LLVMTypeOf(lhs))) ||
				IsFloatVariant(LLVMGetTypeKind(LLVMTypeOf(rhs))))
				return LLVMBuildFAdd(builder, lhs, rhs, "addtmp");
			return LLVMBuildAdd(builder, lhs, rhs, "addtmp");
		case '-':
			getvalue(((BinaryExprAST*)Expr->pointer)->LHS, lhs);
			getvalue(((BinaryExprAST*)Expr->pointer)->RHS, rhs);
			if (IsFloatVariant(LLVMGetTypeKind(LLVMTypeOf(lhs))) ||
				IsFloatVariant(LLVMGetTypeKind(LLVMTypeOf(rhs))))
				return LLVMBuildFSub(builder, lhs, rhs, "subtmp");
			return LLVMBuildSub(builder, lhs, rhs, "subtmp");
		case '*':
			getvalue(((BinaryExprAST*)Expr->pointer)->LHS, lhs);
			getvalue(((BinaryExprAST*)Expr->pointer)->RHS, rhs);
			if (IsFloatVariant(LLVMGetTypeKind(LLVMTypeOf(lhs))) ||
				IsFloatVariant(LLVMGetTypeKind(LLVMTypeOf(rhs))))
				return LLVMBuildFMul(builder, lhs, rhs, "multmp");
			return LLVMBuildMul(builder, lhs, rhs, "multmp");
		case '/':
			if (IsFloatVariant(LLVMGetTypeKind(LLVMTypeOf(lhs))) ||
				IsFloatVariant(LLVMGetTypeKind(LLVMTypeOf(rhs))))
				return LLVMBuildFDiv(builder, lhs, rhs, "divtmp");
			return LLVMBuildSDiv(builder, lhs, rhs, "divtmp");
		case '%':
			getvalue(((BinaryExprAST*)Expr->pointer)->LHS, lhs);
			getvalue(((BinaryExprAST*)Expr->pointer)->RHS, rhs);
			if (IsFloatVariant(LLVMGetTypeKind(LLVMTypeOf(lhs))) ||
				IsFloatVariant(LLVMGetTypeKind(LLVMTypeOf(rhs))))
				return LLVMBuildFRem(builder, lhs, rhs, "remtmp");
			return LLVMBuildSRem(builder, lhs, rhs, "remtmp");
		case '<':
			getvalue(((BinaryExprAST*)Expr->pointer)->LHS, lhs);
			getvalue(((BinaryExprAST*)Expr->pointer)->RHS, rhs);
			if (IsFloatVariant(LLVMGetTypeKind(LLVMTypeOf(lhs))) ||
				IsFloatVariant(LLVMGetTypeKind(LLVMTypeOf(rhs))))
				return LLVMBuildFCmp(builder, LLVMRealULT, lhs, rhs, "lttmp");
			return LLVMBuildICmp(builder, LLVMIntULT, lhs, rhs, "lttmp");
		case '>':
			getvalue(((BinaryExprAST*)Expr->pointer)->LHS, lhs);
			getvalue(((BinaryExprAST*)Expr->pointer)->RHS, rhs);
			if (IsFloatVariant(LLVMGetTypeKind(LLVMTypeOf(lhs))) ||
				IsFloatVariant(LLVMGetTypeKind(LLVMTypeOf(rhs))))
				return LLVMBuildFCmp(builder, LLVMRealUGT, lhs, rhs, "gttmp");
			return LLVMBuildICmp(builder, LLVMIntUGT, lhs, rhs, "gttmp");
		case '&':
			getvalue(((BinaryExprAST*)Expr->pointer)->LHS, lhs);
			getvalue(((BinaryExprAST*)Expr->pointer)->RHS, rhs);
			return LLVMBuildAnd(builder, lhs, rhs, "cmptmp");
		case '|':
			getvalue(((BinaryExprAST*)Expr->pointer)->LHS, lhs);
			getvalue(((BinaryExprAST*)Expr->pointer)->RHS, rhs);
			return LLVMBuildOr(builder, lhs, rhs, "cmptmp");
		case '^':
			getvalue(((BinaryExprAST*)Expr->pointer)->LHS, lhs);
			getvalue(((BinaryExprAST*)Expr->pointer)->RHS, rhs);
			return LLVMBuildXor(builder, lhs, rhs, "cmptmp");
		case '=': {
			getvalue(((BinaryExprAST*)Expr->pointer)->RHS, rhs);
			// depointerize(lhs);
			if (lhs == NULL) {
			new_var:;
				NamedValue* value = malloc(sizeof(NamedValue));
				value->name =
					strdup(((VariableExprAST*)((BinaryExprAST*)Expr->pointer)
								->LHS.pointer)
							   ->Name);
				value->value =
					LLVMBuildAlloca(builder, LLVMTypeOf(rhs), "alloc");
				value->isparam = false;
				HASH_ADD_KEYPTR(
					hh, named_values, value->name, strlen(value->name), value);
				lhs = value->value;
			}
			else if (IsVariableParam(&(((BinaryExprAST*)Expr->pointer)->LHS)))
				goto new_var;
			return LLVMBuildStore(builder, rhs, lhs);
		}
		case tok_dequ:
			getvalue(((BinaryExprAST*)Expr->pointer)->LHS, lhs);
			getvalue(((BinaryExprAST*)Expr->pointer)->RHS, rhs);
			if (IsFloatVariant(LLVMGetTypeKind(LLVMTypeOf(lhs))) ||
				IsFloatVariant(LLVMGetTypeKind(LLVMTypeOf(rhs))))
				return LLVMBuildFCmp(builder, LLVMRealUEQ, lhs, rhs, "eqtmp");
			return LLVMBuildICmp(builder, LLVMIntEQ, lhs, rhs, "eqtmp");
		case tok_nequ:
			getvalue(((BinaryExprAST*)Expr->pointer)->LHS, lhs);
			getvalue(((BinaryExprAST*)Expr->pointer)->RHS, rhs);
			if (IsFloatVariant(LLVMGetTypeKind(LLVMTypeOf(lhs))) ||
				IsFloatVariant(LLVMGetTypeKind(LLVMTypeOf(rhs))))
				return LLVMBuildFCmp(builder, LLVMRealUNE, lhs, rhs, "netmp");
			return LLVMBuildICmp(builder, LLVMIntNE, lhs, rhs, "netmp");
		case tok_lte:
			getvalue(((BinaryExprAST*)Expr->pointer)->LHS, lhs);
			getvalue(((BinaryExprAST*)Expr->pointer)->RHS, rhs);
			if (IsFloatVariant(LLVMGetTypeKind(LLVMTypeOf(lhs))) ||
				IsFloatVariant(LLVMGetTypeKind(LLVMTypeOf(rhs))))
				return LLVMBuildFCmp(builder, LLVMRealULE, lhs, rhs, "letmp");
			return LLVMBuildICmp(builder, LLVMIntULE, lhs, rhs, "letmp");
		case tok_gte:
			getvalue(((BinaryExprAST*)Expr->pointer)->LHS, lhs);
			getvalue(((BinaryExprAST*)Expr->pointer)->RHS, rhs);
			if (IsFloatVariant(LLVMGetTypeKind(LLVMTypeOf(lhs))) ||
				IsFloatVariant(LLVMGetTypeKind(LLVMTypeOf(rhs))))
				return LLVMBuildFCmp(builder, LLVMRealUGE, lhs, rhs, "getmp");
			return LLVMBuildICmp(builder, LLVMIntUGE, lhs, rhs, "getmp");
		case tok_dand:
			getvalue(((BinaryExprAST*)Expr->pointer)->LHS, lhs);
			getvalue(((BinaryExprAST*)Expr->pointer)->RHS, rhs);
			return LLVMBuildAnd(builder, lhs, rhs, "cmptmp");
		case tok_dor:
			getvalue(((BinaryExprAST*)Expr->pointer)->LHS, lhs);
			getvalue(((BinaryExprAST*)Expr->pointer)->RHS, rhs);
			return LLVMBuildOr(builder, lhs, rhs, "cmptmp");
	}

	return NULL;
}

LLVMValueRef codegen_call(
	ExprAST* Expr, LLVMModuleRef module, LLVMBuilderRef builder)
{
	// Retrieve function.
	LLVMValueRef func =
		LLVMGetNamedFunction(module, ((CallExprAST*)Expr->pointer)->Callee);

	// Return error if function not found in module.
	if (func == NULL) {
		fprintf(stderr, "Function not found in module\n");
		return NULL;
	}

	// Return error if number of arguments doesn't match.
	if (LLVMCountParams(func) !=
		((CallExprAST*)Expr->pointer)->Args.size / sizeof(ExprAST)) {
		fprintf(stderr, "Number of Args dosen't match\n");
		fprintf(stderr, "Expected %d have %d\n", LLVMCountParams(func),
			((CallExprAST*)Expr->pointer)->Args.size / sizeof(ExprAST));
		return NULL;
	}

	// Evaluate arguments.
	LLVMValueRef* args = malloc(sizeof(LLVMValueRef) *
		(((CallExprAST*)Expr->pointer)->Args.size / sizeof(ExprAST)));
	unsigned int i;
	unsigned int arg_count =
		((CallExprAST*)Expr->pointer)->Args.size / sizeof(ExprAST);

	for (i = 0; i < arg_count; i++) {
		args[i] = codegen((ExprAST*)(((CallExprAST*)Expr->pointer)->Args.data +
							  (i * sizeof(ExprAST))),
			module, builder, NULL, NULL);
		getvalue2(((ExprAST*)(((CallExprAST*)Expr->pointer)->Args.data +
					  (i * sizeof(ExprAST)))),
			args[i]);
		if (args[i] == NULL) {
			fprintf(stderr, "Codegen failed for arguments in function call\n");
			free(args);
			return NULL;
		}
	}

	// Create call instruction.
	return LLVMBuildCall(builder, func, args, arg_count, "calltmp");
}

LLVMValueRef codegen_prototype(
	ExprAST* Expr, LLVMModuleRef module, LLVMTypeRef Return_Type)
{
	unsigned int i = 0;
	unsigned int arg_count =
		((PrototypeAST*)Expr->pointer)->Args.size / sizeof(ExprAST);

	// Use an existing definition if one exists.
	LLVMValueRef func =
		LLVMGetNamedFunction(module, ((PrototypeAST*)Expr->pointer)->Name);
	if (func != NULL) {
		// Verify parameter count matches.
		if (LLVMCountParams(func) != arg_count) {
			fprintf(stderr,
				"Existing function exists with different parameter count");
			return NULL;
		}

		// Verify that the function is empty.
		if (LLVMCountBasicBlocks(func) != 0) {
			fprintf(stderr, "Existing function exists with a body");
			return NULL;
		}
	}
	// Otherwise create a new function definition.
	else {
		// Create argument list.
		LLVMTypeRef* params = malloc(sizeof(LLVMTypeRef) * arg_count);
		for (i = 0; i < arg_count; i++) {
			params[i] = get_data_type(
				((ExprAST*)(((PrototypeAST*)Expr->pointer)->Args.data +
					 sizeof(ExprAST) * i))
					->pointer);
		}

		// Create function type.
		LLVMTypeRef funcType =
			LLVMFunctionType(Return_Type, params, arg_count, 0);

		// Create function.
		func = LLVMAddFunction(
			module, ((PrototypeAST*)Expr->pointer)->Name, funcType);
		if (func == NULL) {
			fprintf(stderr, "Failed to create function\n");
			return NULL;
		}
		LLVMSetLinkage(func, LLVMExternalLinkage);
	}

	// Assign arguments to named values lookup.
	for (i = 0; i < arg_count; i++) {
		LLVMValueRef param = LLVMGetParam(func, i);
		LLVMSetValueName(param,
			get_expr_name(
				&((DeclareExprAST*)((
					  (ExprAST*)(((PrototypeAST*)Expr->pointer)->Args.data +
						  sizeof(ExprAST) * i))
										->pointer))
					 ->Identity));

		NamedValue* val = malloc(sizeof(NamedValue));
		val->name = strdup(get_expr_name(
			&((DeclareExprAST*)((
				  (ExprAST*)(((PrototypeAST*)Expr->pointer)->Args.data +
					  sizeof(ExprAST) * i))
									->pointer))
				 ->Identity));
		val->value = param;
		val->isparam = true;
		HASH_ADD_KEYPTR(hh, named_values, val->name, strlen(val->name), val);
		if (glass_house_mode)
			printf("Added %s as parameter\n", val->name);
	}
	// printf("Prototype Address: %x\n",func);
	return func;
}

LLVMValueRef codegen_function(ExprAST* Expr, LLVMModuleRef module,
	LLVMBuilderRef builder, LLVMTypeRef Return_Type)
{
	HASH_CLEAR(hh, named_values);

	// Generate the prototype first.
	LLVMValueRef func = codegen_prototype(
		&((ExprAST){Expr->pointer, Prototype}), module,
		Return_Type);
	if (func == NULL)
		return NULL;

	// Create basic block.
	LLVMBasicBlockRef entry = LLVMAppendBasicBlock(func, "entry");
	LLVMPositionBuilderAtEnd(builder, entry);

	// Generate body.
	LLVMValueRef body = codegen(
		&((FunctionAST*)Expr->pointer)->Body, module, builder, entry, NULL);

	if (body == NULL) {
		printf("Failed to codegen function body\n");
		LLVMDeleteFunction(func);
		return NULL;
	}

	// Insert body as return value.
	if (LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(builder)) == NULL) {
		if (glass_house_mode)
			printf(
				"Function not terminated manually\nAuto terminator applied\n");
		if (LLVMGetTypeKind(GetReturnTypeOfBlockParent(entry)) ==
			LLVMVoidTypeKind)
			LLVMBuildRetVoid(builder);
		else if (LLVMGetTypeKind(LLVMTypeOf(body)) ==
			LLVMGetTypeKind(LLVMGetReturnType(LLVMTypeOf(func))))
			LLVMBuildRet(builder, body);
		else
			LLVMBuildRet(
				builder, GetZeroConstOfType(GetReturnTypeOfBlockParent(entry)));
	}

	// LLVMDumpValue(func);
	// Verify function.
	if (LLVMVerifyFunction(func, LLVMPrintMessageAction) == 1) {
		fprintf(stderr, "Invalid function\n");
		LLVMDumpValue(func);
		LLVMDeleteFunction(func);
		return NULL;
	}

	return func;
}

LLVMValueRef codegen_if_expr(ExprAST* Expr, LLVMModuleRef module,
	LLVMBuilderRef builder, LLVMBasicBlockRef block)
{
	// Generate the condition.
	LLVMValueRef condition = codegen(&((IfExprAST*)(Expr->pointer))->condition,
		module, builder, block, NULL);
	if (condition == NULL)
		return NULL;

	// Convert condition to bool.
	if (LLVMTypeOf(condition) == LLVMDoubleType()) {
		LLVMValueRef zero = LLVMConstReal(LLVMDoubleType(), 0);
		condition =
			LLVMBuildFCmp(builder, LLVMRealONE, condition, zero, "ifcond");
	}

	// Retrieve function.
	LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));

	// Generate true/false expr and merge.
	LLVMBasicBlockRef then_block = LLVMAppendBasicBlock(func, "then");
	LLVMBasicBlockRef else_block;
	if (((IfExprAST*)(Expr->pointer))->false_expr.pointer) {
		else_block = LLVMAppendBasicBlock(func, "else");
	}
	else {
		else_block = NULL;
	}
	LLVMBasicBlockRef merge_block = LLVMAppendBasicBlock(func, "ifcont");

	if (else_block)
		LLVMBuildCondBr(builder, condition, then_block, else_block);
	else
		LLVMBuildCondBr(builder, condition, then_block, merge_block);

	// Generate 'true' block.
	LLVMPositionBuilderAtEnd(builder, then_block);
	LLVMValueRef then_value = codegen(&((IfExprAST*)(Expr->pointer))->true_expr,
		module, builder, then_block, NULL);
	if (then_value == NULL) {
		return NULL;
	}
	if (LLVMGetBasicBlockTerminator(then_block) == NULL)
		LLVMBuildBr(builder, merge_block);
	then_block = LLVMGetInsertBlock(builder);

	LLVMValueRef else_value;
	if (else_block) {
		LLVMPositionBuilderAtEnd(builder, else_block);
		else_value = codegen(&((IfExprAST*)(Expr->pointer))->false_expr, module,
			builder, else_block, NULL);
		if (else_value == NULL) {
			return NULL;
		}
		if (LLVMGetBasicBlockTerminator(else_block) == NULL)
			LLVMBuildBr(builder, merge_block);

		else_block = LLVMGetInsertBlock(builder);
	}

	LLVMPositionBuilderAtEnd(builder, merge_block);
	if (!BlockhasNPredecessorsOrMore(merge_block, 1))
		LLVMDeleteBasicBlock(merge_block);
	return then_value;
}

LLVMValueRef codegen_block(ExprAST* Expr, LLVMModuleRef module,
	LLVMBuilderRef builder, LLVMBasicBlockRef block)
{
	LLVMPositionBuilderAtEnd(builder, block);
	unsigned int i = 0;
	LLVMValueRef bodyexpr = NULL;
	while (1) {
		if (((BlockExprAST*)Expr->pointer)->size > i * sizeof(ExprAST)) {
			bodyexpr = codegen(&((BlockExprAST*)Expr->pointer)->Expressions[i],
				module, builder, block, NULL);
			if (!bodyexpr) {
				fprintf(stderr, "Failed to codegen expression in block\n");
				printf("Type: %d\n",
					((BlockExprAST*)Expr->pointer)->Expressions[i].type);
				return NULL;
			}
			if (((BlockExprAST*)Expr->pointer)->Expressions[i].type == Return)
				return bodyexpr;
			i++;
			continue;
		}
		return bodyexpr;
	}
}

LLVMValueRef codegen_return(ExprAST* Expr, LLVMModuleRef module,
	LLVMBuilderRef builder, LLVMBasicBlockRef block)
{
	LLVMValueRef return_value =
		codegen(&((ReturnExprAST*)(Expr->pointer))->Expression, module, builder,
			block, NULL);
	if (((ReturnExprAST*)(Expr->pointer))->Expression.type == Void) {
		LLVMBuildRetVoid(builder);
		return return_value;
	}
	getvalue(((ReturnExprAST*)(Expr->pointer))->Expression, return_value);
	LLVMOpcode cast = get_cast_opcode(LLVMTypeOf(return_value),
		GetReturnTypeOfBlockParent(LLVMGetInsertBlock(builder)));
	if (cast)
		return_value = LLVMBuildCast(builder, cast, return_value,
			GetReturnTypeOfBlockParent(LLVMGetInsertBlock(builder)), "casttmp");
	LLVMBuildRet(builder, return_value);
	return return_value;
}

LLVMValueRef codegen_goto(
	ExprAST* Expr, LLVMBuilderRef builder, LLVMBasicBlockRef block)
{
	NamedValue* value = NULL;
	HASH_FIND_STR(named_values, ((GotoExprAST*)Expr->pointer)->Name, value);
	if (value == NULL)
		return NULL;
	if (LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(builder)) == NULL) {
		LLVMPositionBuilderAtEnd(builder, LLVMGetInsertBlock(builder));
		LLVMBuildBr(builder, LLVMValueAsBasicBlock(value->value));
	}
	else {
		printf("Terminator: ");
		LLVMDumpValue(LLVMGetBasicBlockTerminator(block));
		printf("\n");
	}
	return value->value;
}

LLVMValueRef codegen_label(ExprAST* Expr, LLVMBuilderRef builder)
{
	LLVMValueRef parent = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
	LLVMBasicBlockRef label =
		LLVMAppendBasicBlock(parent, ((GotoExprAST*)Expr->pointer)->Name);
	LLVMBuildBr(builder, label);
	LLVMPositionBuilderAtEnd(builder, label);
	NamedValue* val = malloc(sizeof(NamedValue));
	val->name = strdup(((GotoExprAST*)Expr->pointer)->Name);
	val->value = LLVMBasicBlockAsValue(label);
	val->isparam = false;
	HASH_ADD_KEYPTR(hh, named_values, val->name, strlen(val->name), val);
	return val->value;
}

LLVMValueRef codegen_declaration(
	ExprAST* Expr, LLVMModuleRef module, LLVMBuilderRef builder)
{

	if (((DeclareExprAST*)Expr->pointer)->Identity.type == Variable) {
		LLVMValueRef lhs =
			codegen_variable(&((DeclareExprAST*)Expr->pointer)->Identity);
		NamedValue* value = NULL;
		HASH_FIND_STR(named_values,
			((VariableExprAST*)((DeclareExprAST*)Expr->pointer)
					->Identity.pointer)
				->Name,
			value);
		if (value) {
			lhs =
				LLVMBuildAlloca(builder, get_data_type(Expr->pointer), "alloc");
			NamedValue* value = NULL;
			HASH_FIND_STR(named_values,
				((VariableExprAST*)((DeclareExprAST*)Expr->pointer)
						->Identity.pointer)
					->Name,
				value);
			value->isparam = false;
			value->value = lhs;
		}
		else {
			value = malloc(sizeof(NamedValue));
			value->name =
				strdup(((VariableExprAST*)((DeclareExprAST*)Expr->pointer)
							->Identity.pointer)
						   ->Name);
			value->isparam = false;
			value->value =
				LLVMBuildAlloca(builder, get_data_type(Expr->pointer), "alloc");
			HASH_ADD_KEYPTR(
				hh, named_values, value->name, strlen(value->name), value);
			lhs = value->value;
		}
		return lhs;
	}
	if (((DeclareExprAST*)Expr->pointer)->Identity.type == Function)
		return codegen_function(&((DeclareExprAST*)Expr->pointer)->Identity,
			module, builder, get_data_type(Expr->pointer));
	if (((DeclareExprAST*)Expr->pointer)->Identity.type == Prototype)
		return codegen_prototype(&((DeclareExprAST*)Expr->pointer)->Identity,
			module, get_data_type(Expr->pointer));
	return NULL;
}

LLVMValueRef codegen_unary(
	ExprAST* Expr, LLVMModuleRef module, LLVMBuilderRef builder)
{
	LLVMValueRef Value = codegen(
		&((UnaryExprAST*)Expr->pointer)->Value, module, builder, NULL, NULL);
	if (Value == NULL)
		fprintf(stderr, "Failed to codgen value to apply unary operator on\n");
	switch (((UnaryExprAST*)Expr->pointer)->Op) {
		case '&':
			return Value;
		case '*':
			getvalue(((UnaryExprAST*)Expr->pointer)->Value, Value);
			if (LLVMGetTypeKind(LLVMTypeOf(Value)) != LLVMPointerTypeKind)
				return NULL;
			return LLVMBuildLoad(builder, Value, "depoint");
		default:
			return NULL;
	}
}

LLVMValueRef codegen_cast(ExprAST* Expr, LLVMModuleRef module, LLVMBuilderRef builder)
{
	LLVMValueRef Value = codegen(&(((CastExprAST*)Expr->pointer)->Value), module, builder, NULL, NULL);
	return LLVMBuildCast(builder, get_cast_opcode(LLVMTypeOf(Value),get_data_type(Expr->pointer)), Value, get_data_type(Expr->pointer), "casttmp");
}

LLVMValueRef codegen(ExprAST* Expr, LLVMModuleRef module,
	LLVMBuilderRef builder, LLVMBasicBlockRef block, ExprAST* Parent)
{
	// Recursively free dependent data.
	switch (Expr->type) {
		case Number:
			return codegen_number(Expr, Parent);
		case Variable:
			return codegen_variable(Expr);
		case Binary:
			return codegen_binary_expr(Expr, module, builder);
		case Call:
			return codegen_call(Expr, module, builder);
		case Prototype:
			return codegen_prototype(Expr, module, LLVMDoubleType());
		case Function:
			return codegen_function(Expr, module, builder, LLVMDoubleType());
		case If_Statement:
			return codegen_if_expr(Expr, module, builder, block);
		case Block:
			return codegen_block(Expr, module, builder, block);
		case Return:
			return codegen_return(Expr, module, builder, block);
		case Goto:
			return codegen_goto(Expr, builder, block);
		case Label:
			return codegen_label(Expr, builder);
		case Declaration:
			return codegen_declaration(Expr, module, builder);
		case Void:
			// This shouldn't actually be used it is just to get past
			// error detection alhough if the user is dumb and tries
			// to apply arithmetic on void it will be substituted with
			// a zero
			return LLVMConstInt(LLVMInt1Type(), 0, 0);
		case Unary:
			return codegen_unary(Expr, module, builder);
		case Cast:
			return codegen_cast(Expr, module, builder);
		default:
			return NULL;
	}
}

// Handlers //
static void HandleDefinition(LLVMModuleRef module, LLVMBuilderRef builder,
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

static void HandleExtern(LLVMModuleRef module, LLVMBuilderRef builder)
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

static void HandleTopLevelExpression(LLVMModuleRef module,
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

static int MainLoop(LLVMModuleRef module, LLVMBuilderRef builder,
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

int main(int argc, char** argv)
{
	char* output_filename = NULL;
	if (argc < 2) {
		fprintf(stderr, "ERROR: No arguments given to compiler\n");
		exit(-1);
	}
	else {
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
