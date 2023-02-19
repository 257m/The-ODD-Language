#ifndef PARSER_H
#define PARSER_H

#include "ast.h"

ExprAST ParseNumberExpr();
ExprAST ParseParenExpr();
ExprAST ParseIdentifierExpr();
ExprAST ParseIfExpr();
ExprAST ParseBlockExpr();
ExprAST ParseReturnExpr();
ExprAST ParseGotoExpr();
ExprAST ParseLabelExpr();
PrototypeAST* ParsePrototype();
FunctionAST* ParseFunction(PrototypeAST* Proto);
ExprAST ParseDeclaration();
ExprAST ParseUnaryExpr();
ExprAST ParsePrimary();
ExprAST ParseBinOpRHS(int ExprPrec, ExprAST LHS);
ExprAST ParseExpression();
FunctionAST* ParseTopLevelExpr();
ExprAST ParseExtern();

#endif /* PARSER_H */