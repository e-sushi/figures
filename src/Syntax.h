#ifndef SUUGU_SYNTAX_H
#define SUUGU_SYNTAX_H

#include "defines.h"
#include "utils/string.h"
#include "utils/array.h"


//this file currently stores structures defining various syntax types
//until we decide if we want them separated or not

/*
here im going to write out some examples of what our trees look like
see suugu.cpp for our brackus naur grammar, that defines how these trees are made up

this was written 08/08/2021, so it could be outdated

an AST for something like 2 + 2 would be 
					
					statement (the user's input)
						|
				    expression
						|
					  bitOR
					    |
					 bit XOR
					    |
					 bit AND
					    |
					 equality
					    |
					relational
					    |
					bit shift
					    |
					 additive
					/   |   \
 				term  binop  term
                 /		|      \			
             factor     +     factor
			  /                   \
		  literal               literal

which is very long, but its necessary we have these many steps
to ensure we evaluate expressions according to precedence and associativity

the way we search this tree to evaluate it is through a depth-first search (DFS)

im going to write out other examples so i can reference what the tree should look like

------  (3 - 2) * 2  ------
   this ex shows how incredibly deep these trees can get with just one set of parenthesis
            
                    statement (the user's input)
                        |
                       exp
                        |
                      bitOR
                        |
                     bit XOR
                        |
                     bit AND
                        |
                     equality
                        |
                    relational
                        |
                    bit shift
                        |
                     additive
                        |   
            -----------term-----------
            |           |            |
            |         binop          |
          factor        |         factor
         /  |  \        *            |   
       (   exp   )                literal
            |                        |
          bitOR                      2
            |
         bit XOR
            |
         bit AND
            |
         equality
            |
        relational
            |
        bit shift
            |
         additive
            |
          binop
         /  |  \
   literal  -  literal
      |           |
	  3           2

the branches from factor containing the ( and ) are only to indicate that the factor decided to have that 
next node because it found parenthesis, in the actual tree those nodes wouldn't be there

------  2 * 2 * 2 * 2  ------


                              statement (the user's input)
                                  |
                                 exp
                                  |
                                bitOR
                                  |
                               bit XOR
                                  |
                               bit AND
                                  |
                               equality
                                  |
                              relational
                                  |
                              bit shift
                                  |
                               additive
                                  |
         ------------------------term-----------------------
         |       |        |       |       |        |       |
      factor   binop   factor   binop   factor   binop   factor
         |       |        |       |       |        |       |
      literal    *     literal    *    literal     *    literal
         |                |               |                |
         2                2               2                2


------  1 - 2 * !3 & 4  ------

		
                              statement (the user's input)
                                  |
                                 exp
                                  |
                                bitOR
                                  |
                               bit XOR
                                  |
                  -------------bit AND-------------
                  |               |               |              
              equality            &           equality   
                  |                               |
              relational                      relational
                  |                               |
              bit shift                       bit shift
                  |                               |
              additive----                     additive
             /    |        \                      |
          term    -    ----term-----             term
           |           |     |     |              |
        factor     factor  binop  factor        factor
           |         |       |   /   |            |
        literal   literal    *  !    |         literal
           |         |            factor          |
           1         2               |            4
                                  literal
                                     |
                                     3

its probably worth explaining how we'll step through a complex tree like this
remember that we're doing DFS

imagine we have 2 registers whose purpose is to store a value and 
a stack whose purpose is to store several valuies for later use

[1, 2, ...] represents the stack 
{a, b}      represents registers a and b's values

reg a is our MAIN register and b is a helper who helps with binary op operations

->  indicates going down the tree into the specified node
-^  indicates going up the tree to the specified node
... indicates skipping through nodes we dont need to worry about

    statement -> ... -> bit AND -> LEFT equality -> ... -> additive -> LEFT term -> factor -> literal
    
----we create the value 1 and put it in reg a
    {1, 0} [] 
    
    literal -^ ... -^ additive -> -

----we see that we have to do a subtraction so we store reg a (left nodes 'result') on the stack
    {1, 0} [1]

    - -^ additive -> RIGHT term -> LEFT factor -> literal

----we create the value 2 and put it in reg a
    {2, 0} [1]

    literal -^ factor -^ term -> *

----we see that we have to do a subtraction with the last result so we store it
    {2, 0} [2, 1]

    * -^ term -> factor -> unaryop!

----we see that we are going to do a unaryop NOT, but unary ops dont need to store anything, they just evaluate
    their child node and do their operation when its done
    {2, 0} [2, 1]

    unaryop! -^ factor -> factor -> literal

----we create the value 3 and put it in reg a
    {3, 0} [2, 1]

     literal -^ factor -^ factor 

----this factor knows to evalute the unaryop NOT and does that to reg a
    {0, 0} [2, 1]

     factor -^ term 

----this term knows to evalute * between the reg and the last stored value so we pop 2 into reg b and multiply a and b
    then store the result in a
    {0, 2} [1]
     
     term -^ additive

----this additive node knows to evaluate its left term minus its right term in ASM this is a little more complex
    but here we'll say we pop 1 into reg b, subtract a from b and store the result in reg a
    {0, 1} []

    additive -^ ... -^ bit AND -> &

----from here on its pretty simple, we go down the right side, and when we return we AND the left result
    result with the right result
*/




enum ExpressionType : u32 {
	Expression_IdentifierLHS,
	Expression_IdentifierRHS,

	//Types
	Expression_Literal,

	//Unary Operators
	Expression_UnaryOpBitComp,
	Expression_UnaryOpLogiNOT,
	Expression_UnaryOpNegate,

	//Binary Operators
	Expression_BinaryOpPlus,
	Expression_BinaryOpMinus,
	Expression_BinaryOpMultiply,
	Expression_BinaryOpDivision,
	Expression_BinaryOpAND,
	Expression_BinaryOpBitAND,
	Expression_BinaryOpOR,
	Expression_BinaryOpBitOR,
	Expression_BinaryOpLessThan,
	Expression_BinaryOpGreaterThan,
	Expression_BinaryOpLessThanOrEqual,
	Expression_BinaryOpGreaterThanOrEqual,
	Expression_BinaryOpEqual,
	Expression_BinaryOpNotEqual,
	Expression_BinaryOpModulo,
	Expression_BinaryOpXOR,
	Expression_BinaryOpBitShiftLeft,
	Expression_BinaryOpBitShiftRight,
	Expression_BinaryOpAssignment,

	//Expression Guards
	ExpressionGuard_Assignment,
	ExpressionGuard_Conditional,
	ExpressionGuard_LogicalOR,
	ExpressionGuard_LogicalAND,
	ExpressionGuard_BitOR,
	ExpressionGuard_BitXOR,
	ExpressionGuard_BitAND,
	ExpressionGuard_Equality,
	ExpressionGuard_Relational,
	ExpressionGuard_BitShift,
	ExpressionGuard_Additive,
	ExpressionGuard_Term,
	ExpressionGuard_Factor,
};

//defines arithmatic
struct Expression {
	string expstr;
	ExpressionType type;

	array<Expression*> expressions;

	f32 literalValue; //for storing a literals value if thats what this expression defines

	Expression(string expstr, ExpressionType type) {
		this->type = type;
		this->expstr = expstr;
	}
};

//holds expressions
struct Statement {
	array<Expression*> expressions;

	Statement() {};
};




#endif