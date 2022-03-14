f64      add(f64 a, f64 b){return a+b;}
f64 subtract(f64 a, f64 b){return a-b;}
f64 multiply(f64 a, f64 b){return a*b;}
f64   divide(f64 a, f64 b){return a/b;}

//TODO remove duplication
f64 solve(TNode* term){
	switch(term->type){
		case TermType_Expression:{ //TODO expression math
			Expression2* expr = TermNodeToExpression(term);
			if(expr->valid){
				expr->solution = solve(term->first_child);
				return expr->solution;
			}else{
				return MAX_F32;
			}
		}break;
		
		case TermType_Operator:{
			Operator* op = TermNodeToOperator(term);
			switch(op->type){
				case OpType_ExplicitMultiplication:{
					return multiply(solve(term->first_child), solve(term->last_child));
				}break;
				case OpType_Division:{
					return divide(solve(term->first_child), solve(term->last_child));
				}break;
				
				case OpType_Addition:{
					return add(solve(term->first_child), solve(term->last_child));
				}break;
				case OpType_Subtraction:{
					return subtract(solve(term->first_child), solve(term->last_child));
				}break;
				
				case OpType_ExpressionEquals:{ //TODO variable solving
					return solve(term->first_child);
				}break;
				
				default:{
					LogE("solver","Unknown operator type: ", term->type);
					return MAX_F32;
				}break;
			}
		}break;
		
		case TermType_Literal:{
			Literal* lit = TermNodeToLiteral(term);
			return lit->value;
		}break;
		
		default:{
			LogE("solver","Unknown term type: ", term->type);
			return MAX_F32;
		}break;
	}
}

