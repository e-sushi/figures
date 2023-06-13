f64 builtin_abs(f64 a){ return abs(a); };
f64 builtin_sin(f64 a){ return sin(a); };
f64 builtin_cos(f64 a){ return cos(a); };
f64 builtin_tan(f64 a){ return tan(a); };
f64 builtin_asin(f64 a){ return asin(a); };
f64 builtin_acos(f64 a){ return acos(a); };
f64 builtin_atan(f64 a){ return atan(a); };
f64 builtin_log_e(f64 a){ return log(a); };
f64 builtin_log_10(f64 a){ return log10(a); };

// Function builtin_functions[] = {
// 	{str8_lit("abs"), (void*)builtin_abs, 1},
// 	{str8_lit("sin"), (void*)builtin_sin, 1},
// 	{str8_lit("cos"), (void*)builtin_cos, 1},
// 	{str8_lit("tan"), (void*)builtin_tan, 1},
// 	{str8_lit("asin"), (void*)builtin_asin, 1},
// 	{str8_lit("acos"), (void*)builtin_acos, 1},
// 	{str8_lit("atan"), (void*)builtin_atan, 1},
// 	{str8_lit("sin^-1"), (void*)builtin_asin, 1},
// 	{str8_lit("cos^-1"), (void*)builtin_acos, 1},
// 	{str8_lit("tan^-1"), (void*)builtin_atan, 1},
// 	{str8_lit("ln"),  (void*)builtin_log_e,  1},
// 	{str8_lit("log"), (void*)builtin_log_10, 1},
// };
