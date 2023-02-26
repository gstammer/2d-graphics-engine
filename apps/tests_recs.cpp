/**
 *  Copyright 2015 Mike Reed
 */

#include "../include/GCanvas.h"
#include "../include/GBitmap.h"
#include "../include/GColor.h"
#include "../include/GPoint.h"
#include "../include/GRect.h"
#include "tests.h"

#include "tests_pa1.cpp"
#include "tests_pa2.cpp"
#include "tests_pa3.cpp"

const GTestRec gTestRecs[] = {
    { test_clear,       "clear"         },
    { test_rect_nodraw, "rect_nodraw"   },

    { test_poly_nodraw, "poly_nodraw"   },

    { test_matrix,       "matrix_setters"    },
    { test_matrix_inv,   "matrix_inv"        },
    { test_matrix_map,   "matrix_map"        },
    { test_clamp_shader, "shader_clamp"      },

    { nullptr, nullptr },
};

bool gTestSuite_Verbose;
bool gTestSuite_CrashOnFailure;
