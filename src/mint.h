#ifndef SUUGU_MINT_H
#define SUUGU_MINT_H
#include "kigu/common.h"
#include "memory.h"
StartLinkageC();

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @mint

// multiprecision integer, or big int
// NOTE(sushi) temporarily u8 to make testing a little easier
// because we need to use dynamic memory to implement this, you must be aware
// of what functions perform allocations and make sure you delete things 
// when you are finished!
struct mint{
	s8* arr;
	u64 count;
};

// initializes a mint object
mint mint_init(s8 init);

// deinitializes a mint object
void mint_deinit(mint* m);

// allocates a new mint and copies the given mint to it
mint mint_copy(mint m);

// tests if 'a' is less than 'b'
b32 mint_less_than(mint a, mint b);

// tests if 'a' is less than or equal to 'b'
b32 mint_less_than_or_equal(mint a, mint b);

// negates the given mint
void mint_negate(mint* m);

// allocates a copy of the given mint and negates it
mint mint_negate_new(mint m);

// increments the given mint
void mint_inc(mint* m);

// decrements the given mint
void mint_dec(mint* m);

// computes log2 of 'a' and stores the result in 'a'
void mint_log2(mint* a);

// returns a newly allocated mint equal to log2(a)
mint mint_log2_new(mint a);

// shifts 'a' left by 'b'
void mint_shift_left(mint* a, mint b);

// adds 'b' to 'a' and stores the result in 'a'
// TODO(sushi) this needs a lot more testing and there are probably several optimizations we can do as well
void mint_add(mint* a, mint b);

// allocates a copy of 'a' then adds 'b' to the copy
mint mint_add_new(mint a, mint b);

mint mint_add_s64(mint a, s64 b);

// subtracts 'b' from 'a'
// allocates a copy of the largest mint to operate on and return 
mint mint_sub(mint a, mint b);

// multiplies two mints
// allocates a copy of the largest mint to modify and return
mint mint_mul(mint a, mint b);

void mint_div(mint* a, mint b);

mint mint_div_new(mint a, mint b);

void mint_print(mint a);


EndLinkageC();
#endif // SUUGU_MINT_H

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#if defined(SUUGU_IMPLEMENTATION) && !defined(SUUGU_MINT_IMPL)
#define SUUGU_MINT_IMPL

#define sign(x) x<0

mint mint_init(s8 init){
	mint m = {0};
	m.count = 1;
	m.arr = (s8*)memalloc(sizeof(s8));
	m.arr[0] = init;
	return m;
}

// deletes the given mint
void mint_delete(mint m){
	memzfree(m.arr);
	m.count = 0;
}

// allocates a new mint and copies the given mint to it
mint mint_copy(mint m){
	mint out = {0};
	out.arr = (s8*)memalloc(sizeof(s8)*m.count);
	out.count = m.count;
	memcpy(out.arr, m.arr, m.count*sizeof(s8));
	return out;
}

// tests if 'a' is less than 'b'
// allocates nothing
b32 mint_less_than(mint a, mint b){
	forI(a.count) if(b.arr[i] > a.arr[i]) return true;
	return false;
}

// tests if 'a' is less than or equal to 'b'
// allocates nothing
b32 mint_less_than_or_equal(mint a, mint b){
	forI(a.count) if(b.arr[i] < a.arr[i]) return false;
	return false;
}

// negates the given mint
void mint_negate(mint* a){
	a->arr[0] = ~a->arr[0];
	a->arr[0] += 1;
	forI(a->count-1){
		a->arr[1+i] = ~a->arr[1+i];
	}
}

// allocates a copy of the given mint and negates it
mint mint_negate_new(mint a){
	mint out = mint_copy(a);
	mint_negate(&out);
	return out;
}


// increments a
void mint_inc(mint* a){
    if(a->arr[a->count-1] < 0) {
		a->arr[0]++;
		return;
	}
	
	s8 res = a->arr[0] + 1;
    b32 carry = sign(res) != sign(a->arr[0]);
    a->arr[0] = res;
    if(carry) forI(a->count-1){
        res = a->arr[i+1] + 1;
        carry = res < a->arr[i+1];
        a->arr[i+1] = res;
        if(!carry) break;
    }

    if(carry) a->arr = (s8*)memrealloc(a->arr, ++a->count);

}



// mint mint_log2_fast(mint* a){

// }

// computes log2 of 'a' and stores the result in 'a'
void mint_log2(mint* a){
	s32 count = 0;
	forI_reverse(a->count){
		if(!a->arr[i]) continue;
		while((a->arr[i]<<=1) > 0) 
			count++;
		count = 7-count;
	}
}

// returns a newly allocated mint equal to log2(a)
mint mint_log2_new(mint a){
	mint m = mint_copy(a);
	mint_log2(&m);
	return m;
}

void mint_add(mint* a, mint b);
void mint_add_s64(mint* a, s64 b);

// shifts 'a' left by 'b'
void mint_shift_left(mint* a, mint b){	
	u32 count = 0;
	forI_reverse(a->count){
		if(!a->arr[i]) continue;
		while((a->arr[i]<<=1)>0) count++;
		break;
	}
	count = 7-count;

	mint_add_s64(b, count);
}

// adds 'b' to 'a' and stores the result in 'a'
// TODO(sushi) this needs a lot more testing and there are probably several optimizations we can do as well
void mint_add(mint* a, mint b){

	b32 samesign = sign(a->arr[a->count-1]) == sign(b.arr[b.count-1]);

	//special case where we are adding a mint with itself
	if(a->arr == b.arr){
		NotImplemented;
		//NOTE(sushi) handle this
	}

	u32 i = 0;
	s32 carry = 0;
	// iterate over each piece of the integers
	while(1){
		if(i >= b.count && i < a->count){
			// if we have iterated over all of b, we have to propagate carries across a
			if(b.arr[b.count-1] < 0){
				// if b is negative we have to sign extend
				s8 res = a->arr[i] + carry - 1;
				carry = sign(res) != sign(a->arr[i]);
				a->arr[i] = res;
			}else if(carry){
				// otherwise, we can just carry normally
				s8 res = a->arr[i] + 1;
				carry = sign(res) != sign(a->arr[i]);
				a->arr[i] = res;
			}else break;
		}else{
			// otherwise, we just add normally, unless a's size is less than b
			if(i >= a->count && i < b.count){ 
				// if a is smaller in memory than b, we need to extend it 
				a->arr = (s8*)memrealloc(a->arr, ++a->count);
				if(a->arr[a->count-2] < 0) a->arr[a->count-1] = -1;
			}
			s8 res = a->arr[i] + b.arr[i] + carry;
			carry = sign(res) != sign(a->arr[0]);
			a->arr[i] = res;
		}
		i++;
		if(i == a->count) break;
	}


	// there is never overflow when adding 2 numbers of different signs
	if(!samesign) return;
	// somewhat of a special case where doubling a negative number causes the above algo to think it needs
	// to carry, but it doesn't
	if(a->count == b.count && a->arr[a->count-1] < 0 == b.arr[b.count-1] < 0) 
		carry = 0; 

	if(carry){
		a->arr = (s8*)memrealloc(a->arr, ++a->count);
		if(b.arr[b.count-1] < 0) a->arr[a->count-1] = -1;
		else a->arr[a->count] = 1;
	}else if(a->arr[a->count-1] < 0 != b.arr[b.count-1] < 0){
		a->arr = (s8*)memrealloc(a->arr, sizeof(u8) * ++a->count);
	}
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
		carry = sign(m.arr[i+1]) > sign(a.arr[i+1]);
	}else if(carry) forI(a.count-1) {
		m.arr[i+1] += 1;
		carry = sign(m.arr[i+1]) != sign(a.arr[i+1]);
		if(!carry) break;
	}

	if(carry){
		m.arr = (s8*)memrealloc(m.arr, sizeof(s8) * ++m.count);
		if(m.arr[m.count-1] > 0) m.arr[m.count-1] = 1;
		else m.arr[m.count-1] = -1;
	}

	return m;
}

// subtracts 'b' from 'a', and stores the result in 'a'
void mint_sub(mint* a, mint b){

}

// subtracts 'b' from 'a' and stores the result in a new mint
mint mint_sub_new(mint a, mint b){
	mint m = mint_copy(a);
	mint_sub(&m,b);
	return m;
}

// multiplies two mints
// allocates a copy of the largest mint to modify and return
mint mint_mul(mint a, mint b){
	mint min, max;
	if(a.count<b.count) 
		min = a, max = b;
	else 
		min = b, max = a;
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
	str8b out;
	str8_builder_init(&out, {0}, deshi_temp_allocator);
	mint qtrack = mint_init(0);
	mint rtrack = mint_init(0);
	forI(a.count){
		u8 chunk = a.arr[i];
		u8 q = chunk/10, r = chunk%10;
		//qtrack = mint_add_new(qtrack, mint_init(q));
		//rtrack = mint_add_new((rtrack << 4*i)
	}
}

#endif // SUUGU_IMPLEMENTATION