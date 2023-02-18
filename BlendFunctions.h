/*
 *  Copyright 2023 Georgie Stammer
 */

#ifndef BlendFunctions_DEFINED
#define BlendFunctions_DEFINED

#include "include/GPaint.h"
#include "include/GPixel.h"

// divides by 255 without using / for optimization
static int Div255(int p) {
    // (p + 128) * 257 >> 16
    int ret = p + 128;
    ret *= 257;
    ret = ret >> 16;
    return ret;
}

GPixel blendClear(GPixel srcPixel, GPixel destPixel) {
    GPixel clearPixel = GPixel_PackARGB(0, 0, 0, 0);
    return clearPixel;
}

GPixel blendSrc(GPixel srcPixel, GPixel destPixel) {
    return srcPixel;
}

// Takes 2 GPixels & blends into 1 GPixel w/ srcover
GPixel blendSrcOver(GPixel srcPixel, GPixel destPixel) {
    
    // if srcPixel is opaque, don't go thru operations
    // if (GPixel_GetA(srcPixel) == 255 || GPixel_GetA(destPixel) == 0) return srcPixel;

    // extract color from both pixels
    int dR = GPixel_GetR(destPixel);
    int dG = GPixel_GetG(destPixel);
    int dB = GPixel_GetB(destPixel);
    int dA = GPixel_GetA(destPixel);

    int sR = GPixel_GetR(srcPixel);
    int sG = GPixel_GetG(srcPixel);
    int sB = GPixel_GetB(srcPixel);
    int sA = GPixel_GetA(srcPixel);

    // src over operation:
    // S + ((255 - Sa) * D + 127) / 255
    // S + Div255((255 - Sa) * D)
    int dCoef = 255 - sA;
    int newR = sR + Div255(dCoef * dR);
    int newG = sG + Div255(dCoef * dG);
    int newB = sB + Div255(dCoef * dB);
    int newA = sA + Div255(dCoef * dA);

    GPixel outPixel = GPixel_PackARGB(newA, newR, newG, newB);
    return outPixel;
}

// Takes 2 GPixels & blends into 1 GPixel w/ dstover
GPixel blendDstOver(GPixel srcPixel, GPixel destPixel) {
    
    // should I check for if destPixel is opaque?

    // extract color from both pixels
    int dR = GPixel_GetR(destPixel);
    int dG = GPixel_GetG(destPixel);
    int dB = GPixel_GetB(destPixel);
    int dA = GPixel_GetA(destPixel);

    int sR = GPixel_GetR(srcPixel);
    int sG = GPixel_GetG(srcPixel);
    int sB = GPixel_GetB(srcPixel);
    int sA = GPixel_GetA(srcPixel);

    // dst over operation:
    // D + (1 - Da)*S
    // D + Div255((255 - Da) * S)
    int dCoef = 255 - dA;
    int newR = dR + Div255(dCoef * sR);
    int newG = dG + Div255(dCoef * sG);
    int newB = dB + Div255(dCoef * sB);
    int newA = dA + Div255(dCoef * sA);

    GPixel outPixel = GPixel_PackARGB(newA, newR, newG, newB);
    return outPixel;
}

// Takes 2 GPixels & blends into 1 GPixel w/ srcin
GPixel blendSrcIn(GPixel srcPixel, GPixel destPixel) {
    
    // extract color from both pixels
    int dA = GPixel_GetA(destPixel);

    int sR = GPixel_GetR(srcPixel);
    int sG = GPixel_GetG(srcPixel);
    int sB = GPixel_GetB(srcPixel);
    int sA = GPixel_GetA(srcPixel);

    // src in operation:
    // Da * S
    int newR = Div255(dA * sR);
    int newG = Div255(dA * sG);
    int newB = Div255(dA * sB);
    int newA = Div255(dA * sA);

    GPixel outPixel = GPixel_PackARGB(newA, newR, newG, newB);
    return outPixel;
}

// Takes 2 GPixels & blends into 1 GPixel w/ dstin
GPixel blendDstIn(GPixel srcPixel, GPixel destPixel) {
    
    // extract color from both pixels
    int dR = GPixel_GetR(destPixel);
    int dG = GPixel_GetG(destPixel);
    int dB = GPixel_GetB(destPixel);
    int dA = GPixel_GetA(destPixel);

    int sA = GPixel_GetA(srcPixel);

    // dst in operation:
    // Sa * D
    int newR = Div255(sA * dR);
    int newG = Div255(sA * dG);
    int newB = Div255(sA * dB);
    int newA = Div255(sA * dA);

    GPixel outPixel = GPixel_PackARGB(newA, newR, newG, newB);
    return outPixel;
}

// Takes 2 GPixels & blends into 1 GPixel w/ srcout
GPixel blendSrcOut(GPixel srcPixel, GPixel destPixel) {
    
    // extract color from both pixels
    int dA = GPixel_GetA(destPixel);

    int sR = GPixel_GetR(srcPixel);
    int sG = GPixel_GetG(srcPixel);
    int sB = GPixel_GetB(srcPixel);
    int sA = GPixel_GetA(srcPixel);

    // src out operation:
    // (1 - Da)*S
    // Div255((255 - Da) * S)
    int dCoef = 255 - dA;
    int newR = Div255(dCoef * sR);
    int newG = Div255(dCoef * sG);
    int newB = Div255(dCoef * sB);
    int newA = Div255(dCoef * sA);

    GPixel outPixel = GPixel_PackARGB(newA, newR, newG, newB);
    return outPixel;
}

// Takes 2 GPixels & blends into 1 GPixel w/ dstout
GPixel blendDstOut(GPixel srcPixel, GPixel destPixel) {

    // extract color from both pixels
    int dR = GPixel_GetR(destPixel);
    int dG = GPixel_GetG(destPixel);
    int dB = GPixel_GetB(destPixel);
    int dA = GPixel_GetA(destPixel);

    int sA = GPixel_GetA(srcPixel);

    // src over operation:
    // (1 - Sa)*D
    // Div255((255 - Sa) * D)
    int dCoef = 255 - sA;
    int newR = Div255(dCoef * dR);
    int newG = Div255(dCoef * dG);
    int newB = Div255(dCoef * dB);
    int newA = Div255(dCoef * dA);

    GPixel outPixel = GPixel_PackARGB(newA, newR, newG, newB);
    return outPixel;
}

// Takes 2 GPixels & blends into 1 GPixel w/ srcatop
GPixel blendSrcATop(GPixel srcPixel, GPixel destPixel) {
    
    // src atop operation:
    // Da*S + (1 - Sa)*D
    // srcin + dstout

    return blendSrcIn(srcPixel, destPixel) + blendDstOut(srcPixel, destPixel);
}

// Takes 2 GPixels & blends into 1 GPixel w/ dstatop
GPixel blendDstATop(GPixel srcPixel, GPixel destPixel) {
    
    // src atop operation:
    // Sa*D + (1 - Da)*S
    // dstin + srcout

    return blendDstIn(srcPixel, destPixel) + blendSrcOut(srcPixel, destPixel);
}

// Takes 2 GPixels & blends into 1 GPixel w/ xor
GPixel blendXor(GPixel srcPixel, GPixel destPixel) {
    
    // xor operation:
    // (1 - Sa)*D + (1 - Da)*S
    // srcout + dstout

    return blendSrcOut(srcPixel, destPixel) + blendDstOut(srcPixel, destPixel);
}

#endif