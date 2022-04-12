f64 builtin_abs(f64 a){ return abs(a); };
f64 builtin_sin(f64 a){ return sin(a); };
f64 builtin_cos(f64 a){ return cos(a); };
f64 builtin_tan(f64 a){ return tan(a); };
f64 builtin_asin(f64 a){ return asin(a); };
f64 builtin_acos(f64 a){ return acos(a); };
f64 builtin_atan(f64 a){ return atan(a); };
f64 builtin_log_e(f64 a){ return log(a); };
f64 builtin_log_10(f64 a){ return log10(a); };

Function builtin_functions[] = {
	{cstr_lit("abs"), builtin_abs, 1},
	{cstr_lit("sin"), builtin_sin, 1},
	{cstr_lit("cos"), builtin_cos, 1},
	{cstr_lit("tan"), builtin_tan, 1},
	{cstr_lit("asin"), builtin_asin, 1},
	{cstr_lit("acos"), builtin_acos, 1},
	{cstr_lit("atan"), builtin_atan, 1},
	{cstr_lit("sin^-1"), builtin_asin, 1},
	{cstr_lit("cos^-1"), builtin_acos, 1},
	{cstr_lit("tan^-1"), builtin_atan, 1},
	{cstr_lit("ln"),  builtin_log_e,  1},
	{cstr_lit("log"), builtin_log_10, 1},
};
