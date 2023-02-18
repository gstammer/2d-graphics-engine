/**
 *  Copyright 2023 Georgie Stammer
 */

#ifndef Edge_DEFINED
#define Edge_DEFINED

#include "include/GMath.h"
#include "include/GPoint.h"
#include <algorithm>

class Edge {
public:

    GPoint p1;
    GPoint p2;
    int yTop;
    int yBottom;
    float xLeft;

    // x = my + b line connecting p1 & p2
    float m;
    float b;

    // constructor
    Edge(GPoint s = {0, 0}, GPoint t = {0, 0}) {
        p1 = s;
        p2 = t;
        yTop = GRoundToInt(std::min(p1.fY, p2.fY));
        yBottom = GRoundToInt(std::max(p1.fY, p2.fY));
        xLeft = std::min(p1.fX, p2.fX);

        // x = my + b line connecting p1 & p2
        m = (p2.fX - p1.fX) / (p2.fY - p1.fY);
        // b = x - my
        b = p1.fX - m * p1.fY;
    }

    float xIntersect(float y) {
        // x = my + b
        float x = m * y + b;
        return x;
    }

    float yIntersect(float x) {
        // my = x - b
        // y = (x - b) / m
        float y = (x - b) / m;
        return y;
    }

};

#endif
