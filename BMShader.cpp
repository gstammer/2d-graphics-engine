/*
 *  Copyright 2023 Georgie Stammer
 */

#include "include/GMatrix.h"
#include "include/GShader.h"
#include "include/GBitmap.h"
#include <algorithm>

void printMatrix(GMatrix* mx) {
    printf("Matrix:\n");
    printf("%f %f %f\n", (*mx)[0], (*mx)[1], (*mx)[2]);
    printf("%f %f %f\n", (*mx)[3], (*mx)[4], (*mx)[5]);
}

class BMShader : public GShader {
    const GBitmap fBM;
    const GMatrix fLocalMatrix;
    GMatrix fInverse;

public:
    BMShader(const GBitmap& bm, const GMatrix& localInverse) 
        : fBM(bm), fLocalMatrix(localInverse) {}

    // Return true iff all of the GPixels that may be returned by this shader will be opaque.
    bool isOpaque() {
        return fBM.isOpaque();
    }

    // The draw calls in GCanvas must call this with the CTM before any calls to shadeSpan().
    bool setContext(const GMatrix& ctm) {
        GMatrix invCTM;
        bool invExists = ctm.invert(&invCTM);
        if (!invExists) return false;

        // fInv = fLM * inv(CTM)
        fInverse = GMatrix::Concat(fLocalMatrix, invCTM);
        // printMatrix(&fInverse);
        return true;
    }

    /**
     *  Given a row of pixels in device space [x, y] ... [x + count - 1, y], return the
     *  corresponding src pixels in row[0...count - 1]. The caller must ensure that row[]
     *  can hold at least [count] entries.
     */
    void shadeRow(int x, int y, int count, GPixel row[]) {
        // undo the transforming matrix to find x1, y1 in local coords
        GPoint startPt =  {x+0.5, y+0.5};
        GPoint localPtStart = fInverse * startPt;
        float A = fInverse[0];
        float D = fInverse[3];
        GPoint localPt = localPtStart;
        
        for (int i = 0; i < count; i++) {
            // find the bitmap coord that the new pixel center is inside
            int x1 = floor(localPt.fX);
            int y1 = floor(localPt.fY);
            // handle if outside bounds of shader’s bitmap
            int x2 = x1 <= 0 ? 0 : x1 >= fBM.width() ? fBM.width()-1 : x1;
            int y2 = y1 <= 0 ? 0 : y1 >= fBM.height() ? fBM.height()-1 : y1;
            // retrieve the src pixel from the shader bitmap & put into the row
            row[i] = *fBM.getAddr(x2, y2);

            // update localPt using A & D
            localPt.fX += A;
            localPt.fY += D;
        }
    }
};

std::unique_ptr<GShader> GCreateBitmapShader(const GBitmap& bm, const GMatrix& localInverse) {
    // std::unique_ptr<GShader> ret = MyShader(bm, localInverse);
    return std::unique_ptr<GShader>(new BMShader(bm, localInverse));
}
