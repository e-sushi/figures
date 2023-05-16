/* suugu solver.cpp
Notes:


Index:
@solver_equation
@solver_unknowns

References:


TODO:
history of steps the solver took (make this toggleable so it isnt used if doing heavy stuff)
unknown variable deep search (check that variables with expressions don't contain unknown variables)
turn solver variables into a struct so that solving can be multithreaded (threading by expression seems the easiest)
multi variable solve
*/
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


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

b32 solver_has_error = false;
s32 solver_error_code = SolverError_None;
Term* solver_error_term = 0;
Expression* solver_expression = 0;
Term** solver_unknown_variables = 0;
u32 solver_unknown_variables_count_left = 0;
u32 solver_unknown_variables_count_right = 0;

#define SOLVER_ERROR_VALUE MAX_F64
#define SOLVER_ERROR_PASSTHRU(var) if(var == SOLVER_ERROR_VALUE) return SOLVER_ERROR_VALUE
#define SOLVER_ERROR(error_code) (solver_error_code = error_code, solver_error_term = term, SOLVER_ERROR_VALUE)


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @solver_equation


void solve_unknowns(Expression* expr);

f64 solve(Term* term){
	if(solver_error_code != SolverError_None) return SOLVER_ERROR_VALUE;
	
	switch(term->type){
		case TermType_Expression:{
			Expression* expr = ExpressionFromTerm(term);
			if(!expr->valid) return SOLVER_ERROR(SolverError_InvalidExpression);
			
			solver_error_code = SolverError_None;
			solver_error_term = 0;
			solver_expression = expr;
			
			//either solve for the equation or solve for variables
			if(expr->unknown_vars){
				solve_unknowns(expr);
			}else{
				expr->solution = solve(term->first_child);
			}
			
			solver_expression = 0;
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
				
				case OpType_ExpressionEquals:{
					Assert(solver_expression->unknown_vars == 0, "this part of solve() should not be reached when there are unknown vars in the expression");
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
			if(term->var.expr){
				Expression* temp = solver_expression;
				f64 a = solve(&term->var.expr->term); SOLVER_ERROR_PASSTHRU(a);
				solver_expression = temp;
				return a;
			}else{
				Assert(!"this part of solve() should not be reached when there are unknown vars in the expression");
				return SOLVER_ERROR(SolverError_InvalidExpression);
			}
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
			Assert(!"solving not setup yet for this term");
			return SOLVER_ERROR(SolverError_NotImplemented);
		}break;
	}
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @solver_unknowns


//TODO rather than solve_unknowns, this should be solve_for with a specific variable to solve for rather than assuming numeric single solve or multivariable solve
//     (figure out how multivariable solving fits into solve_for, i dont even remember how to do multivariable atm)
void solve_unknowns(Expression* expr){
	Assert(expr->equals, "the expression must have an equals operator in order to solve for unknowns");
	
	//deep copy the expression so it can be reordered for solving without affecting the original expression
	//NOTE one extra term as an empty slot for easier reordering
	Term* terms = (Term*)memory_talloc((expr->terms.count + 1) * sizeof(Term));
	Term* equals = &terms[expr->equals - expr->terms.data];
	Term* extra = terms + expr->terms.count;
	CopyMemory(terms, expr->terms.data, expr->terms.count * sizeof(Term));
	forI(expr->terms.count){
		terms[i].prev        = (expr->terms[i].prev)        ? &terms[expr->terms[i].prev        - expr->terms.data] : 0;
		terms[i].next        = (expr->terms[i].next)        ? &terms[expr->terms[i].next        - expr->terms.data] : 0;
		terms[i].parent      = (expr->terms[i].parent)      ? &terms[expr->terms[i].parent      - expr->terms.data] : 0;
		terms[i].first_child = (expr->terms[i].first_child) ? &terms[expr->terms[i].first_child - expr->terms.data] : 0;
		terms[i].last_child  = (expr->terms[i].last_child)  ? &terms[expr->terms[i].last_child  - expr->terms.data] : 0;
	}
	
	//reset unknown vars array
	if(solver_unknown_variables && (solver_unknown_variables_count_left + solver_unknown_variables_count_right >= expr->unknown_vars)){
		ZeroMemory(solver_unknown_variables, expr->unknown_vars*sizeof(Term*));
	}else{
		memory_zfree(solver_unknown_variables);
		solver_unknown_variables = (Term**)memory_alloc(expr->unknown_vars*sizeof(Term*));
	}
	solver_unknown_variables_count_left = 0;
	solver_unknown_variables_count_right = 0;
	
	//collect all unknown vars and track which side they are on
	u32 tracked_unknown_count = 0;
	For(terms,expr->terms.count){
		if(it->type == TermType_Variable && it->var.expr == 0){
			solver_unknown_variables[tracked_unknown_count] = it;
			if(it->var.right_of_equals){
				solver_unknown_variables_count_right += 1;
			}else{
				solver_unknown_variables_count_left += 1;
			}
			tracked_unknown_count += 1;
		}
	}
	
	//TODO assuming very simple, one-op equations for now
	if(expr->unknown_vars == 1){
		//single variable solve by isolating the unknown variable on one side of the equals
		Term* unknown = solver_unknown_variables[0];
		
		if(unknown == equals->first_child){
			expr->solution = solve(equals->last_child);
		}else if(unknown == equals->last_child){
			expr->solution = solve(equals->first_child);
		}else{
			//TODO heuristic for choosing which side of the equation to isolate the unknown (opposite of side which is more complex?)
			Term* op = (unknown->var.right_of_equals) ? equals->last_child : equals->first_child;
			switch(op->op_type){
				case OpType_Addition:{
					//5 = 1 + x    1 + x = 5
					
					//create a subtraction operator as the parent on the other side
					//5 - = 1 + x    1 + x = 5 -
					ZeroMemory(extra, sizeof(Term));
					extra->type = TermType_Operator;
					extra->raw = str8_lit("-");
					extra->op_type = OpType_Subtraction;
					if(unknown->var.right_of_equals){
						ast_change_parent_insert_first(extra, equals->first_child);
						ast_insert_first(equals, extra);
					}else{
						ast_change_parent_insert_first(extra, equals->last_child);
						ast_insert_last(equals, extra);
					}
					
					//change parent of the unknown's sibling to the subtraction operator
					//5 - 1 = + x    + x = 5 - 1
					if(unknown == op->first_child){
						ast_change_parent_insert_last(extra, op->last_child);
					}else{
						ast_change_parent_insert_last(extra, op->first_child);
					}
					
					//elevate the unknown to be a child of the equals
					//5 - 1 = x_+    +_x = 5 - 1
					if(unknown->var.right_of_equals){
						ast_change_parent_insert_last(equals, unknown);
					}else{
						ast_change_parent_insert_first(equals, unknown);
					}
					
					//convert the addition operator to be the extra for later reuse
					//5 - 1 = x     x = 5 - 1
					ast_remove_from_parent(op);
					ast_remove_horizontally(op);
					Swap(extra, op);
				}break;
				
				case OpType_Subtraction:{
					if(unknown == op->first_child){
						//5 = x - 1    x - 1 = 5
						
						//create an addition operator as the parent on the other side
						//5 + = x - 1    x - 1 = 5 +
						ZeroMemory(extra, sizeof(Term));
						extra->type = TermType_Operator;
						extra->raw = str8_lit("+");
						extra->op_type = OpType_Addition;
						if(unknown->var.right_of_equals){
							ast_change_parent_insert_first(extra, equals->first_child);
							ast_insert_first(equals, extra);
						}else{
							ast_change_parent_insert_first(extra, equals->last_child);
							ast_insert_last(equals, extra);
						}
						
						//change parent of the unknown's sibling to the new addition operator
						//5 + 1 = x -    x - = 5 + 1
						ast_change_parent_insert_last(extra, op->last_child);
						
						//elevate the unknown to be a child of the equals
						//5 + 1 = x_-    x_- = 5 + 1
						if(unknown->var.right_of_equals){
							ast_change_parent_insert_last(equals, unknown);
						}else{
							ast_change_parent_insert_first(equals, unknown);
						}
						
						//convert the original subtraction operator to be the extra for later reuse
						//5 + 1 = x    x = 5 + 1
						ast_remove_from_parent(op);
						ast_remove_horizontally(op);
						Swap(extra, op);
					}else{
						//5 = 1 - x    1 - x = 5
						Term* other_side = (unknown->var.right_of_equals) ? equals->first_child : equals->last_child;
						
						//elevate the unknown to be a child of the equals on the other side
						//5_x = 1 -    1 - = 5_x
						ast_change_parent_insert_last(equals, unknown);
						
						//change parent of the the other side to the original subtraction
						//x = 1 - 5    1 - 5 = x
						ast_change_parent_insert_last(op, other_side);
					}
				}break;
				
				default:{
					Assert(!"unknown solving no setup yet for this operator");
				}break;
			}
			
			expr->solution = solve(op);
		}
		debug_print_term(equals);
	}else{
		//TODO multi variable solve
		
		LogE("suugu-solver", "Solving for multiple variables not implemented yet.");
		Assert(!"solving for multiple variables not implemented yet");
	}
}
