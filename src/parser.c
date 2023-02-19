#include "../include/parser.h"

ExprAST ParseNumberExpr()
{
	ExprAST Result = {.pointer = malloc(sizeof(NumberExprAST)), .type = Number};
	((NumberExprAST*)Result.pointer)->Value = NumVal;
	getNextToken(); // consume the number
	return Result;
}

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

ExprAST ParseReturnExpr()
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

ExprAST ParseGotoExpr()
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

ExprAST ParseLabelExpr()
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

PrototypeAST* ParsePrototype()
{
	PrototypeAST* returnProto = malloc(sizeof(PrototypeAST));
	strcpy(returnProto->Name, IdentifierStr);
	returnProto->Args.data = NULL;
	returnProto->Args.size = 0;

	getNextToken(); // eat prototype name.

	if (CurTok == '=' || CurTok == ';' || CurTok == ',' || CurTok == ')' 
		|| CurTok == '[' || CurTok == tok_identifier) {
		// Just a variable declaration.
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

FunctionAST* ParseFunction(PrototypeAST* Proto)
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

ExprAST ParseDeclaration()
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

ExprAST ParsePrimary()
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

ExprAST ParseBinOpRHS(int ExprPrec, ExprAST LHS)
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

FunctionAST* ParseTopLevelExpr()
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

ExprAST ParseExtern()
{
	getNextToken(); // eat the extern word
	// printf("%s\n",IdentifierStr);
	return ParseDeclaration(); // return the function name and args to handler
}