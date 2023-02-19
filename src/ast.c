#include "../include/ast.h"

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

int GetTokPrecedence()
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