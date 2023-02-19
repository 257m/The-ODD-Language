#include "../include/codegen.h"

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

inline bool IsFloatVariant(LLVMTypeKind typekind)
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