/*
 *  Copyright 2023 Georgie Stammer
 */

#include "include/GMatrix.h"
#include "math.h"

// initialize to identity matrix
GMatrix::GMatrix() {
    fMat[0] = 1;    fMat[1] = 0;    fMat[2] = 0;
    fMat[3] = 0;    fMat[4] = 1;    fMat[5] = 0;
}

/**
 *  Return translation matrix
 *  (make identity mx & add to 3rd column)
 */
GMatrix GMatrix::Translate(float tx, float ty) {
    GMatrix mx = GMatrix(1, 0, tx, 0, 1, ty);
    return mx;
}

/**
 *  Return scaling matrix
 *  (make identity mx & multiply a * sx and e * sy)
 */
GMatrix GMatrix::Scale(float sx, float sy) {
    GMatrix mx = GMatrix(sx, 0, 0, 0, sy, 0);
    return mx;
}

/**
 *  Return rotation matrix
 *  (make matrix with:  cos sin 0
 *                     -sin cos 0)
 */
GMatrix GMatrix::Rotate(float radians) {
    float cosX = cos(radians);
    float sinX = sin(radians);
    GMatrix mx = GMatrix(cosX, -sinX, 0, sinX, cosX, 0);
    return mx;
}

/**
 *  Return the product of two matrices: a * b
 */
GMatrix GMatrix::Concat(const GMatrix& a, const GMatrix& b) {
    float m0 = a[0] * b[0] + a[1] * b[3];
    float m1 = a[0] * b[1] + a[1] * b[4];
    float m2 = a[0] * b[2] + a[1] * b[5] + a[2];

    float m3 = a[3] * b[0] + a[4] * b[3];
    float m4 = a[3] * b[1] + a[4] * b[4];
    float m5 = a[3] * b[2] + a[4] * b[5] + a[5];

    return GMatrix(m0, m1, m2, m3, m4, m5);
}

// helper for invert
static inline float dcross(float a, float b, float c, float d) {
    return a * b - c * d;
}

/*
 *  Compute the inverse of this matrix, and store it in the "inverse" parameter, being
 *  careful to handle the case where 'inverse' might alias this matrix.
 *
 *  If this matrix is invertible, return true. If not, return false, and ignore the
 *  'inverse' parameter.
 * 
 *  false if determinant is 0
 */
bool GMatrix::invert(GMatrix* inverse) const {
    float det = dcross(fMat[0], fMat[4], fMat[3], fMat[1]);
    if (det == 0) return false;
    float idet = 1 / det;
    
    float a =  fMat[4] * idet;
    float b = -fMat[1] * idet;
    float c = dcross(fMat[1], fMat[5], fMat[4], fMat[2]) * idet;
    
    float d = -fMat[3] * idet;
    float e =  fMat[0] * idet;
    float f = dcross(fMat[3], fMat[2], fMat[0], fMat[5]) * idet;

    *inverse = GMatrix(a, b, c, d, e, f);
    return true;
}

/**
 *  Transform the set of points in src, storing the resulting points in dst, by applying this
 *  matrix. It is the caller's responsibility to allocate dst to be at least as large as src.
 *
 *  [ a  b  c ] [ x ]     x' = ax + by + c
 *  [ d  e  f ] [ y ]     y' = dx + ey + f
 *  [ 0  0  1 ] [ 1 ]
 *
 *  Note: It is legal for src and dst to point to the same memory (however, they may not
 *  partially overlap). Thus the following is supported.
 *
 *  GPoint pts[] = { ... };
 *  matrix.mapPoints(pts, pts, count);
 */
void GMatrix::mapPoints(GPoint dst[], const GPoint src[], int count) const {
    for (int i = 0; i < count; i++) {
        GPoint pt = src[i];
        float newX = fMat[0] * pt.fX + fMat[1] * pt.fY + fMat[2];
        float newY = fMat[3] * pt.fX + fMat[4] * pt.fY + fMat[5];
        dst[i] = {newX, newY};
    }
}