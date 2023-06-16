/*
    A collection of headers that almost never change and take a decent amount of time to compile
    we generate a pch with clang and then use it in compiling to speed up compile times a good bit,
    for example, without pch, suugu compiles in about 1.5 seconds, but with, it compiles in about
    0.5 seconds.

    Combining this with only compiling deshi when we need to, we can get compile times of under a 
    second when only working with suugu, massively better than any times I've seen with this project
    before.

*/

#include "deshi/src/kigu/common.h"
#include "deshi/src/kigu/arrayT.h"
#include "deshi/src/kigu/array_utils.h"
#include "deshi/src/core/memory.h"
#include "deshi/src/core/render.h"
#include "deshi/src/math/math.h"
#include "deshi/src/kigu/color.h"
#include "deshi/src/external/stb/stb_image.h"
#include "deshi/src/external/stb/stb_truetype.h"
