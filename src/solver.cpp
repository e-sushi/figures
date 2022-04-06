b32 solve_error = false;
f64 solve(Term* term){
	if(solve_error) return MAX_F64;
	
	switch(term->type){
		case TermType_Expression:{ //TODO expression math
			Expression* expr = ExpressionFromTerm(term);
			if(expr->valid){
				expr->solution = solve(term->first_child);
				solve_error = false;
				return expr->solution;
			}else{
				solve_error = false;
				return MAX_F64;
			}
		}break;
		
		case TermType_Operator:{
			switch(term->op_type){
				case OpType_Parentheses:{
					return solve(term->first_child);
				}break;
				
				case OpType_Exponential:{
					f64 a = solve(term->first_child);
					f64 b = solve(term->last_child);
					if(a == MAX_F64 || b == MAX_F64) return MAX_F64;
					if(b < 0) return 1.0 / pow(a, abs(b));
					return pow(a, b);
				}break;
				
				case OpType_Negation:{
					f64 a = solve(term->first_child);
					if(a == MAX_F64) return MAX_F64;
					return -a;
				}break;
				
				case OpType_ExplicitMultiplication:{
					f64 a = solve(term->first_child);
					f64 b = solve(term->last_child);
					if(a == MAX_F64 || b == MAX_F64) return MAX_F64;
					return a * b;
				}break;
				case OpType_Division:{
					f64 a = solve(term->first_child);
					f64 b = solve(term->last_child);
					if(a == MAX_F64 || b == MAX_F64) return MAX_F64;
					if(b == 0) return MAX_F64;
					return a / b;
				}break;
				case OpType_Modulo:{
					f64 a = solve(term->first_child);
					f64 b = solve(term->last_child);
					if(a == MAX_F64 || b == MAX_F64) return MAX_F64;
					if(b == 0) return MAX_F64;
					return fmod(a, b);
				}break;
				
				case OpType_Addition:{
					f64 a = solve(term->first_child);
					f64 b = solve(term->last_child);
					if(a == MAX_F64 || b == MAX_F64) return MAX_F64;
					return a + b;
				}break;
				case OpType_Subtraction:{
					f64 a = solve(term->first_child);
					f64 b = solve(term->last_child);
					if(a == MAX_F64 || b == MAX_F64) return MAX_F64;
					return a - b;
				}break;
				
				case OpType_ExpressionEquals:{ //TODO variable solving
					return solve(term->first_child);
				}break;
				
				default:{
					LogE("solver","Unknown operator type: ", term->op_type);
					solve_error = true;
					return MAX_F64;
				}break;
			}
		}break;
		
		case TermType_Literal:{
			return term->lit_value;
		}break;
		
		case TermType_Variable:{
			solve_error = true;
			return MAX_F64; //TODO variable solving
		}break;
		
		default:{
			LogE("solver","Unknown term type: ", term->type);
			solve_error = true;
			return MAX_F64;
		}break;
	}
}

