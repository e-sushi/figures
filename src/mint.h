/* suugu multi-precision integer lib

	This library provides a mutiprecision integer, aka bigint for use in suugu.

*/
#ifndef SUUGU_MINT_H
#define SUUGU_MINT_H
#include "kigu/common.h"
#include "memory.h"
StartLinkageC();


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @mint


// multiprecision integer, or big int
struct mint{
	// the sign of this mint, -1 for negative, 0 for zero, and 1 for positive
	// if this is 0, then the entire struct is 0! a mint with no sign has nothing allocated.
	s8 sign; 
	u8* arr; // array of integers representing this mint
	u64 count; 
};

// initializes a mint object
mint mint_init(s8 init);

// deinitializes a mint object
void mint_deinit(mint* m);

// allocates a new mint and copies the given mint to it
mint mint_copy(mint m);

// compares two mints.
// returns 
//	 1 if a > b
//   0 if a == b
//  -1 if a < b 
s8 mint_compare(mint a, mint b);

// compares two mints, ignoring sign
// returns 
//	 1 if a > b
//   0 if a == b
//  -1 if a < b 
s8 mint_compare_mag(mint a, mint b);

// tests if 'a' is less than 'b'
b32 mint_less_than(mint a, mint b);

// tests if 'a' is less than or equal to 'b'
b32 mint_less_than_or_equal(mint a, mint b);

// tests if 'a' is greater than 'b'
b32 mint_greater_than(mint a, mint b);

// tests if 'a' is greater than or equal to 'b'
b32 mint_greater_than_or_equal(mint a, mint b);

// negates the given mint and returns it
mint* mint_negate(mint* m);

// allocates a copy of the given mint and negates it
mint mint_negate_new(mint m);

// computes log2 of 'a', stores the result in 'a', then returns a pointer to 'a'
mint* mint_log2(mint* a);

// returns a newly allocated mint equal to log2(a)
mint mint_log2_new(mint a);

// shifts 'a' left by 'b', then returns a pointer to 'a'
mint* mint_shift_left(mint* a, mint b);

// adds 'b' to 'a', stores the result in 'a', then returns a pointer to 'a'
mint* mint_add(mint* a, mint b);

// allocates a copy of 'a' then adds 'b' to the copy
mint mint_add_new(mint a, mint b);

mint mint_add_s64(mint a, s64 b);

// subtracts 'b' from 'a', then returns the pointer to 'a'
mint* mint_sub(mint* a, mint b);

// allocates a copy of 'a', then subtracts 'b' from it
mint mint_sub_new(mint a, mint b);

// multiplies two mints
// allocates a copy of the largest mint to modify and return
mint mint_mul(mint a, mint b);

void mint_div(mint* a, mint b);

mint mint_div_new(mint a, mint b);

void mint_print(mint a);


EndLinkageC();
#endif //#ifndef SUUGU_MINT_H
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#if defined(SUUGU_IMPLEMENTATION) && !defined(SUUGU_MINT_IMPL)
#define SUUGU_MINT_IMPL


#define sign(x) (s8)x<0

mint mint_init(s8 init){
	mint m = {0};
	if(init){
		m.count = 1;
		m.arr = (u8*)memalloc(sizeof(s8));
		m.arr[0] = init;
		m.sign = (init < 0 ? -1 : 1);
		// if the given number is negative we need to perform two's complement to get the actual magnitude
		if(m.sign == -1) m.arr[0] = ~m.arr[0] + 1;
	}
	return m;
}

// deletes the given mint
void mint_delete(mint m){
	memzfree(m.arr);
	m.count = 0;
}

// resizes a mint to hold n integers. this is for internal use to abstract the allocations
// a user will never use this because mints manage their own sizes.
void mint_resize(mint* a,u64 n){
	a->count = n;
	a->arr = (u8*)memrealloc(a->arr, n*sizeof(u8));
}

// allocates a new mint and copies the given mint to it
mint mint_copy(mint m){
	mint out = {0};
	out.sign = m.sign;
	out.arr = (u8*)memalloc(sizeof(s8)*m.count);
	out.count = m.count;
	memcpy(out.arr, m.arr, m.count*sizeof(s8));
	return out;
}

s8 mint_compare(mint a, mint b){
	if(a.sign == b.sign) {
		switch(a.sign){
			case  1: return mint_compare_mag(a,b);
			case  0: return 0; 
			case -1: return mint_compare_mag(b,a);
		}
	}
	return a.sign > b.sign ? 1 : -1;
}

s8 mint_compare_mag(mint a, mint b){
	if(a.count > b.count) return  1;
	if(a.count < b.count) return -1;
	forI(a.count){
		if(a.arr[i] != b.arr[i]){
			return (a.arr[i] < b.arr[i] ? -1 : 1);
		}
	}
	return 0;
}

b32 mint_less_than(mint a, mint b)            { return mint_compare(a,b) <  0; }
b32 mint_less_than_or_equal(mint a, mint b)   { return mint_compare(a,b) <= 0; }
b32 mint_greater_than(mint a, mint b)         { return mint_compare(a,b) >  0; }
b32 mint_greater_than_or_equal(mint a, mint b){ return mint_compare(a,b) >= 0; }

// negates the given mint
mint* mint_negate(mint* a){
	a->sign = !a->sign;
	return a;
}

// allocates a copy of the given mint and negates it
mint mint_negate_new(mint a){
	mint out = mint_copy(a);
	mint_negate(&out);
	return out;
}

// computes log2 of 'a' and stores the result in 'a'
mint* mint_log2(mint* a){
	s32 count = 0;
	forI_reverse(a->count){
		if(!a->arr[i]) continue;
		while((a->arr[i]<<=1) > 0){
			count++;
		}
		count = 7-count;
	}
	return a;
}

mint mint_log2_new(mint a){
	mint m = mint_copy(a);
	mint_log2(&m);
	return m;
}

mint* mint_shift_left(mint* a, mint b){	
	u32 count = 0;
	forI_reverse(a->count){
		if(!a->arr[i]) continue;
		while((a->arr[i] <<= 1) > 0) count++;
		break;
	}
	count = 7-count;
	
	mint_add_s64(b, count);
	return a;
}

//TODO(sushi) this needs a lot more testing and there are probably several optimizations we can do as well
mint* mint_add(mint* a, mint b){
	if(!a->sign){ // a is 0
		*a = mint_copy(b);
		return a;
	}
	if(!b.sign){ // b is 0
		return a;
	}
	if(b.sign < a->sign){ // b is negative
		return mint_sub(a, b);
	}
	if(a->sign < b.sign){ // here, we do -(a-b), because mint_sub stores the result in a, so we can't just flip the order of operands
		return mint_negate(mint_sub(mint_negate(a), *mint_negate(&b)));
	}
	
	//special case where we are adding a mint with itself
	if(a->arr == b.arr){
		NotImplemented;
		//TODO(sushi) handle this
	}
	
	// if a is smaller in memory than b, we need to allocate enough space for it
	if(a->count < b.count) mint_resize(a, b.count);
	
	// add corresponding parts of the numbers 
	s32 carry = 0;
	forI(b.count){
		u8 res = a->arr[i] + b.arr[i] + carry;
		carry = a->arr[i] > res;
		a->arr[i] = res;
	}
	
	// propagate carries through for as long as needed
	for(s32 i = b.count; i < a->count && carry; i++){
		u8 res = a->arr[i] + carry;
		carry = a->arr[i] > res;
		a->arr[i] = res;
	}
	
	// if we still need to carry, we need to add one more integer to a and add one to it.
	if(carry){
		mint_resize(a, a->count+1);
		a->arr[a->count-1] += 1;
	}
	return a;
}

// allocates a copy of 'a' then adds 'b' to the copy
mint mint_add_new(mint a, mint b){
	mint m = mint_copy(a);
	mint_add(&m, b);
	return m;
}

mint mint_add_s64(mint a, s64 b){
	mint m = mint_copy(a);
	m.arr[0] += b;
	
	s32 carry = (u8)m.arr[0] < (u8)a.arr[0];
	if(b<0) forI(a.count-1) {
		m.arr[i+1] += carry - 1;
		carry = sign(m.arr[i+1]) != sign(a.arr[i+1]);
	}else if(carry) forI(a.count-1) {
		m.arr[i+1] += 1;
		carry = sign(m.arr[i+1]) != sign(a.arr[i+1]);
		if(!carry) break;
	}
	
	if(carry){
		m.arr = (u8*)memrealloc(m.arr, sizeof(s8) * ++m.count);
		if(m.arr[m.count-1] > 0){
			m.arr[m.count-1] = 1;
		}else{
			m.arr[m.count-1] = -1;
		}
	}
	
	return m;
}

mint* mint_sub(mint* a, mint b){
	if(!b.sign) return a;
	if(!a->sign){
		// if a is zero, just take whatever b is and make it negative
		*a = mint_copy(b);
		a->sign = -1;
		return a;
	}	
	if(a->sign == b.sign) return mint_add(a, b);
	
	s8 compare = mint_compare_mag(*a, b);
	switch(compare){
		case 0:{ // the numbers are equal, so a becomes 0
			memzfree(a->arr);
			*a = {0};
			return a;
		}
		case 1: break; //no need to do anything maybe
		case -1: a->sign = !a->sign; break;
	}
	
	// if a is smaller in memory than b, we need to allocate enough space for it
	if(a->count < b.count) mint_resize(a, b.count);
	
	// subtract corresponding parts of the numbers
	s32 borrow = 0;
	forI(b.count){
		u8 res = a->arr[i] - b.arr[i] - borrow;
		borrow = sign(a->arr[i]) != sign(res);
		a->arr[i] = res;
	}
	
	// propagate borrows as much as needed
	for(s32 i = b.count; i < a->count && borrow; i++){
		u8 res = a->arr[i] - borrow;
		borrow = sign(a->arr[i]) != sign(res);
		if(!borrow) break;
	}
	
	if(borrow){
		DebugBreakpoint;
	}
	
	return a;
}

mint mint_sub_new(mint a, mint b){
	mint m = mint_copy(a);
	mint_sub(&m,b);
	return m;
}

// multiplies two mints
// allocates a copy of the largest mint to modify and return
mint mint_mul(mint a, mint b){
	mint min, max;
	if(a.count < b.count){
		min = a, max = b;
	}else{
		min = b, max = a;
	}
	mint m = mint_copy(max);
	mint save = m;
	mint iter = mint_init(0);
	while(mint_less_than(iter, b)){
		
	}
	
	return mint_init(0);
}

void mint_div(mint* a, mint b){
	
}

mint mint_div_new(mint a, mint b){
	mint m = mint_copy(a);
	mint_div(&m, b);
	return m;
}

void mint_print(mint a){
	dstr8 out;
	dstr8_init(&out, {0}, deshi_temp_allocator);
	mint qtrack = mint_init(0);
	mint rtrack = mint_init(0);
	forI(a.count){
		u8 chunk = a.arr[i];
		u8 q = chunk/10, r = chunk%10;
		//qtrack = mint_add_new(qtrack, mint_init(q));
		//rtrack = mint_add_new((rtrack << 4*i)
	}
}


#endif //#if defined(SUUGU_IMPLEMENTATION) && !defined(SUUGU_MINT_IMPL)