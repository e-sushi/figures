enum{
	SolverError_None = 0,
	SolverError_NotImplemented,
	SolverError_InvalidExpression,
	SolverError_DivideByZero,
	SolverError_WrongNumberOfFunctionArguments,
	SolverError_LogarithmBaseZero,
	SolverError_LogarithmBaseOne,
	SolverError_LogarithmOfZero,
};

//TODO solver steps: history of steps the solver took (NOTE make this toggleable so it isnt used if doing heavy stuff)

#define SOLVER_ERROR_VALUE MAX_F64
#define SOLVER_ERROR_PASSTHRU(var) if(var == MAX_F64) return MAX_F64
#define SOLVER_ERROR(code) (solver_error_code = code, solver_error_term = term, SOLVER_ERROR_VALUE)
s32 solver_error_code = SolverError_None;
Term* solver_error_term = 0;
b32 solver_has_error = false;
f64 solve(Term* term){
	if(solver_has_error) return SOLVER_ERROR_VALUE;
	
	switch(term->type){
		case TermType_Expression:{
			Expression* expr = ExpressionFromTerm(term);
			if(!expr->valid){
				solver_has_error = false;
				solver_error_code = SolverError_InvalidExpression;
				return SOLVER_ERROR_VALUE;
			}
			
			expr->solution = solve(term->first_child);
			solver_has_error = false;
			if(expr->solution != SOLVER_ERROR_VALUE){
				solver_error_code = SolverError_None;
				solver_error_term = 0;
			}
			return expr->solution;
		}break;
		
		case TermType_Operator:{
			switch(term->op_type){
				case OpType_Parentheses:{
					return solve(term->first_child);
				}break;
				
				case OpType_Exponential:{
					f64 a = solve(term->first_child); SOLVER_ERROR_PASSTHRU(a);
					f64 b = solve(term->last_child); SOLVER_ERROR_PASSTHRU(b);
					if(b < 0){ //NOTE if the exponent b is negative, it acts as 1 / a^b
						f64 c = pow(a, abs(b));
						if(c == 0) return SOLVER_ERROR(SolverError_DivideByZero);
						return 1.0 / c;
					}else{
						return pow(a, b);
					}
				}break;
				
				case OpType_Negation:{
					f64 a = solve(term->first_child); SOLVER_ERROR_PASSTHRU(a);
					return -a;
				}break;
				
				case OpType_ImplicitMultiplication:
				case OpType_ExplicitMultiplication:{
					f64 a = solve(term->first_child); SOLVER_ERROR_PASSTHRU(a);
					f64 b = solve(term->last_child); SOLVER_ERROR_PASSTHRU(b);
					return a * b;
				}break;
				case OpType_Division:{
					f64 a = solve(term->first_child); SOLVER_ERROR_PASSTHRU(a);
					f64 b = solve(term->last_child); SOLVER_ERROR_PASSTHRU(b);
					if(b == 0) return SOLVER_ERROR(SolverError_DivideByZero);
					return a / b;
				}break;
				case OpType_Modulo:{
					f64 a = solve(term->first_child); SOLVER_ERROR_PASSTHRU(a);
					f64 b = solve(term->last_child); SOLVER_ERROR_PASSTHRU(b);
					if(b == 0) return SOLVER_ERROR(SolverError_DivideByZero);
					return fmod(a, b);
				}break;
				
				case OpType_Addition:{
					f64 a = solve(term->first_child); SOLVER_ERROR_PASSTHRU(a);
					f64 b = solve(term->last_child); SOLVER_ERROR_PASSTHRU(b);
					return a + b;
				}break;
				case OpType_Subtraction:{
					f64 a = solve(term->first_child); SOLVER_ERROR_PASSTHRU(a);
					f64 b = solve(term->last_child); SOLVER_ERROR_PASSTHRU(b);
					return a - b;
				}break;
				
				case OpType_ExpressionEquals:{ //TODO variable solving
					return solve(term->first_child);
				}break;
				
				default:{
					Assert(!"solving not setup for this operator yet");
					return SOLVER_ERROR(SolverError_NotImplemented);
				}break;
			}
		}break;
		
		case TermType_Literal:{
			return term->lit_value;
		}break;
		
		case TermType_Variable:{
			solver_has_error = true;
			return SOLVER_ERROR_VALUE; //TODO variable solving
		}break;
		
		case TermType_FunctionCall:{
			switch(term->func->args){
				case 1:{
					return ((Function1Arg)term->func->ptr)(solve(term->first_child));
				}break;
				default:{
					return SOLVER_ERROR(SolverError_WrongNumberOfFunctionArguments);
				}break;
			};
		}break;
		
		case TermType_Logarithm:{
			f64 a = solve(term->first_child); SOLVER_ERROR_PASSTHRU(a);
			if(term->log_base == 0) return SOLVER_ERROR(SolverError_LogarithmBaseZero);
			if(term->log_base == 1) return SOLVER_ERROR(SolverError_LogarithmBaseOne);
			if(term->log_base == a) return 1;
			if(a == 0) return SOLVER_ERROR(SolverError_LogarithmOfZero);
			if(a == 1) return 0;
			return log2(a) / log2(term->log_base);
		}break;
		
		default:{
			Assert(!"solving not setup for this term yet");
			return SOLVER_ERROR(SolverError_NotImplemented);
		}break;
	}
}

