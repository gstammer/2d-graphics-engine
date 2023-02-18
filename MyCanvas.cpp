/*
 *  Copyright 2023 Georgie Stammer
 */

#include "include/GCanvas.h"
#include "include/GRect.h"
#include "include/GColor.h"
#include "include/GBitmap.h"

#include "BlendFunctions.h"
#include "Edge.h"
#include <iostream>
// #include <vector>
#include <algorithm>

// Helper functions below!

// Converts a Gcolor into a GPixel W/ NO BLENDING
static GPixel ColorToPixel(const GColor& color) {
    int newR = GRoundToInt(color.r * 255 * color.a);
    int newG = GRoundToInt(color.g * 255 * color.a);
    int newB = GRoundToInt(color.b * 255 * color.a);
    int newA = GRoundToInt(color.a * 255);

    GPixel outPixel = GPixel_PackARGB(newA, newR, newG, newB);
    return outPixel;
}

// Returns GPixel address in mem, given the xy coords
static GPixel* get_pixel_addr(const GBitmap& bitmap, int x, int y) {
    GPixel* startingAddr = bitmap.pixels();
    char* row = (char*)startingAddr + y * bitmap.rowBytes();
    return (GPixel*)row + x;
}

// Clip edges of convex polygon, making new array outEdgeArray
// Return the count of used elements of the array
static int clipEdges(Edge edgeArray[], int count, int fWidth, int fHeight, Edge* outEdgeArray) {
    
    int trackEdgeArray = 0;

    // loop thru array to clip each edge
    for (int i = 0; i < count; i++) {
        Edge currEdge = edgeArray[i];
        GPoint p1 = currEdge.p1;
        GPoint p2 = currEdge.p2;

        // check vertical clipping first
        if (p1.fY > p2.fY) std::swap(p1, p2);   // ensures p1 is the higher point
        if (p2.fY <= 0 || p1.fY >= fHeight) continue;   // discard edge if completely too high or low
        // clip top
        if (p1.fY < 0) {
            p1 = {currEdge.xIntersect(0), 0};
        }
        // clip bottom
        if (p2.fY > fHeight) {
            p2 = {currEdge.xIntersect(fHeight), float(fHeight)};
        }

        // check horizontal clipping
        bool extraEdge = false;
        Edge sideEdge;
        if (p1.fX > p2.fX) std::swap(p1, p2);   // ensures p1 is the leftmost point
        if (p2.fX <= 0) {       // if complpetely too left, just make projection
            p1 = {0, p1.fY};
            p2 = {0, p2.fY};
        }
        else if (p1.fX >= fWidth) {    // if completely too right, make projection
            p1 = {float(fWidth), p1.fY};
            p2 = {float(fWidth), p2.fY};
        }
        // clip left
        if (p1.fX < 0) {
            extraEdge = true;
            // replace p1 with intersection pt at left side
            GPoint sidePoint = {0, p1.fY};
            p1 = {0, currEdge.yIntersect(0)};
            if (p1.fY < 0) p1.fY = 0;
            if (p1.fY >= fWidth) p1.fY = float(fWidth);
            // & add edge going from intersection to y-value of p1
            sideEdge = {p1, sidePoint};
        }
        // clip right
        if (p2.fX > fWidth) {
            extraEdge = true;
            // replace p2 with intersection pt at right side
            GPoint sidePoint = {float(fWidth), p2.fY};
            p2 = {float(fWidth), currEdge.yIntersect(fWidth)};
            if (p2.fY < 0) p2.fY = 0;
            if (p2.fY >= fWidth) p2.fY = float(fWidth);
            // & add edge going from intersection to y-value of p2
            sideEdge = {p2, sidePoint};
        }

        // put clipped edge into the vector
        Edge newEdge = Edge(p1, p2);
        // discard new edge if it is horizontal
        if (newEdge.yTop == newEdge.yBottom) continue;
        outEdgeArray[trackEdgeArray] = newEdge;
        trackEdgeArray++;
        // if clipped horizontally, add in extra edge
        if (extraEdge) {
            outEdgeArray[trackEdgeArray] = sideEdge;
            trackEdgeArray++;
        }
    }

    // return the last filled element of the array
    return trackEdgeArray;
}

// comparator to sort Edges
bool compareEdges(Edge& e1, Edge& e2) {
    if (e1.yTop == e2.yTop) return (e1.xLeft < e2.xLeft);
    return (e1.yTop < e2.yTop);
}

// prints Edge array for debugging
void printEdgeArray(Edge* edgesArrayPtr, int count) {
    for (int i = 0; i < count; i++) {
        Edge x = edgesArrayPtr[i];
        printf("edge (%f, %f) - (%f, %f)\n", x.p1.fX, x.p1.fY, x.p2.fX, x.p2.fY);
    }
}


class MyCanvas : public GCanvas {
public:
    MyCanvas(const GBitmap& device) : fDevice(device) {}

    /**
     *  Template to loop thru rows & replace each pixel with a new blended pixel
     *  For filling entire device screen, rectPtr is nullptr
     */
    template <typename Method>
    void drawRows(GPixel src, GIRect* rectPtr, Method bl) {
        // if no rect, fill whole device screen
        if (rectPtr == nullptr) {
            int x = fDevice.width();
            int y = fDevice.height();
            int numPixels = x * y;
            int pixelCount = 0;
            GPixel* dstPtr = fDevice.pixels();
            while (pixelCount < numPixels) {
                GPixel blendedPixel = bl(src, *dstPtr);
                *dstPtr = blendedPixel;
                dstPtr++;
                pixelCount++;
            }
            return;
        }
        
        GIRect rect = *rectPtr;
        // otherwise, loop thru rows of rectangle
        for (int y = rect.fTop; y < rect.fBottom; y++) {
            for (int x = rect.fLeft; x < rect.fRight; x++) {
                GPixel* dstPtr = get_pixel_addr(fDevice, x, y);
                GPixel blendedPixel = bl(src, *dstPtr);
                *dstPtr = blendedPixel;
            }
        }
    }

    /**
     *  Helper function that handles the switch case for choosing a blend mode
     *  & then calls drawRows to draw
     */
    void blendAndDraw(GBlendMode mode, GPixel src, GIRect* rectPtr) {
        switch (mode) {
            case GBlendMode::kClear:
                {
                    drawRows(src, rectPtr, blendClear);
                }
                break;
            case GBlendMode::kSrc:
                {
                    drawRows(src, rectPtr, blendSrc);
                }
                break;
            case GBlendMode::kDst:
                // nothing changes!
                break;
            case GBlendMode::kSrcOver:
                {
                    drawRows(src, rectPtr, blendSrcOver);
                }
                break;
            case GBlendMode::kDstOver:
                {
                    drawRows(src, rectPtr, blendDstOver);
                }
                break;
            case GBlendMode::kSrcIn:
                {
                    drawRows(src, rectPtr, blendSrcIn);
                }
                break;
            case GBlendMode::kDstIn:
                {
                    drawRows(src, rectPtr, blendDstIn);
                }
                break;
            case GBlendMode::kSrcOut:
                {
                    drawRows(src, rectPtr, blendSrcOut);
                }
                break;
            case GBlendMode::kDstOut:
                {
                    drawRows(src, rectPtr, blendDstOut);
                }
                break;
            case GBlendMode::kSrcATop:
                {
                    drawRows(src, rectPtr, blendSrcATop);
                }
                break;
            case GBlendMode::kDstATop:
                {
                    drawRows(src, rectPtr, blendDstATop);
                }
                break;
            case GBlendMode::kXor:
                {
                    drawRows(src, rectPtr, blendXor);
                }
                break;
        }
    }

    /**
     *  Fill the entire canvas with the specified color, using the specified blendmode.
     */
    void drawPaint(const GPaint& paint) {
        
        // establish pixel & blend mode to paint with
        GPixel newPixel = ColorToPixel(paint.getColor());
        GBlendMode mode = paint.getBlendMode();

        // optimize when alpha is 0
        if (paint.getAlpha() == 0) {
            if (mode == GBlendMode::kSrcOver || mode == GBlendMode::kDstOver ||
                mode == GBlendMode::kDstOut || mode == GBlendMode::kSrcATop) return;
            if (mode == GBlendMode::kSrcIn || mode == GBlendMode::kDstIn ||
                mode == GBlendMode::kSrcOut || mode == GBlendMode::kDstATop) {
                    mode = GBlendMode::kClear;
                }
        }

        // optimize when alpha is 1
        if (paint.getAlpha() == 1) {
            if (mode == GBlendMode::kSrcOver) mode = GBlendMode::kSrc;
            if (mode == GBlendMode::kDstIn) mode = GBlendMode::kDst;
            if (mode == GBlendMode::kDstOut) mode = GBlendMode::kClear;
            if (mode == GBlendMode::kSrcATop) mode = GBlendMode::kSrcIn;
            if (mode == GBlendMode::kDstATop) mode = GBlendMode::kDstOver;
            if (mode == GBlendMode::kXor) mode = GBlendMode::kSrcOut;
        }

        // loop thru canvas based on which blend mode is being used
        blendAndDraw(mode, newPixel, nullptr);

    }
    
    /**
     *  Fill the rectangle with the color, using the specified blendmode.
     *
     *  The affected pixels are those whose centers are "contained" inside the rectangle:
     *      e.g. contained == center > min_edge && center <= max_edge
     */
    void drawRect(const GRect& rect, const GPaint& paint) {
        
        // establish pixel & blend mode to paint with
        GPixel srcPixel = ColorToPixel(paint.getColor());
        GBlendMode mode = paint.getBlendMode();

        // round rectangle into GIRect
        GIRect roundedRect = rect.round();

        // clip areas that are outside canvas
        int canvasX = fDevice.width();
        int canvasY = fDevice.height();
        if (roundedRect.fLeft < 0) roundedRect.fLeft = 0;
        if (roundedRect.fTop < 0) roundedRect.fTop = 0;
        if (roundedRect.fRight > canvasX) roundedRect.fRight = canvasX;
        if (roundedRect.fBottom > canvasY) roundedRect.fBottom = canvasY;

        // check if rect shouldn't be drawn at all
        if (roundedRect.fLeft >= roundedRect.fRight) return;
        if (roundedRect.fTop >= roundedRect.fBottom) return;
        
        // optimize when alpha is 0
        if (paint.getAlpha() == 0) {
            if (mode == GBlendMode::kSrcOver || mode == GBlendMode::kDstOver ||
                mode == GBlendMode::kDstOut || mode == GBlendMode::kSrcATop) return;
            if (mode == GBlendMode::kSrcIn || mode == GBlendMode::kDstIn ||
                mode == GBlendMode::kSrcOut || mode == GBlendMode::kDstATop) {
                    mode = GBlendMode::kClear;
                }
        }

        // optimize when alpha is 1
        if (paint.getAlpha() == 1) {
            if (mode == GBlendMode::kSrcOver) mode = GBlendMode::kSrc;
            if (mode == GBlendMode::kDstIn) mode = GBlendMode::kDst;
            if (mode == GBlendMode::kDstOut) mode = GBlendMode::kClear;
            if (mode == GBlendMode::kSrcATop) mode = GBlendMode::kSrcIn;
            if (mode == GBlendMode::kDstATop) mode = GBlendMode::kDstOver;
            if (mode == GBlendMode::kXor) mode = GBlendMode::kSrcOut;
        }

        blendAndDraw(mode, srcPixel, &roundedRect);
    }

    /**
     *  Fill the convex polygon with the color and blendmode,
     *  following the same "containment" rule as rectangles.
     */
    void drawConvexPolygon(const GPoint points[], int count, const GPaint& paint) {
        
        if (count <= 2) return;
        
        // // get paint info
        GPixel srcPixel = ColorToPixel(paint.getColor());
        GBlendMode mode = paint.getBlendMode();

        // optimize when alpha is 0
        if (paint.getAlpha() == 0) {
            if (mode == GBlendMode::kSrcOver || mode == GBlendMode::kDstOver ||
                mode == GBlendMode::kDstOut || mode == GBlendMode::kSrcATop) return;
            if (mode == GBlendMode::kSrcIn || mode == GBlendMode::kDstIn ||
                mode == GBlendMode::kSrcOut || mode == GBlendMode::kDstATop) {
                    mode = GBlendMode::kClear;
                }
        }

        // optimize when alpha is 1
        if (paint.getAlpha() == 1) {
            if (mode == GBlendMode::kSrcOver) mode = GBlendMode::kSrc;
            if (mode == GBlendMode::kDstIn) mode = GBlendMode::kDst;
            if (mode == GBlendMode::kDstOut) mode = GBlendMode::kClear;
            if (mode == GBlendMode::kSrcATop) mode = GBlendMode::kSrcIn;
            if (mode == GBlendMode::kDstATop) mode = GBlendMode::kDstOver;
            if (mode == GBlendMode::kXor) mode = GBlendMode::kSrcOut;
        }

        // contruct all edges
        Edge allEdges[count];
        for (int i = 0; i < count - 1; i++) {
            allEdges[i] = Edge(points[i], points[i+1]);
        }
        // add last edge
        allEdges[count - 1] = Edge(points[count - 1], points[0]);

        // do clipping function to get new list of edges
        Edge edgesArrayPtr[count*3];
        int newEdgeCount = clipEdges(allEdges, count, fDevice.width(), fDevice.height(), edgesArrayPtr);

        // sort by topY w/ leftmost X as tiebreaker
        std::sort(edgesArrayPtr, edgesArrayPtr+newEdgeCount, compareEdges);

        // draw
        int trackEdges = 2;
        int yStart = edgesArrayPtr[0].yTop;
        for (int y = yStart; y < fDevice.height(); y++) {

            float hit = edgesArrayPtr[0].xIntersect(y + 0.5f);
            int x1 = GRoundToInt(hit);
            float hit2 = edgesArrayPtr[1].xIntersect(y + 0.5f);
            int x2 = GRoundToInt(hit2);
            int xStart = std::min(x1, x2);
            int xEnd = std::max(x1, x2);

            // draw each row using drawRect w/ height 1
            if (xStart != xEnd) {
                GIRect rowRect = {xStart, y, xEnd, y+1};
                blendAndDraw(mode, srcPixel, &rowRect);
            }

            // when y passes yBottom of an edge, swap it w/ next highest edge
            if (y+1 >= edgesArrayPtr[0].yBottom) {
                edgesArrayPtr[0] = edgesArrayPtr[trackEdges];
                trackEdges++;
            }
            if (y+1 >= edgesArrayPtr[1].yBottom) {
                edgesArrayPtr[1] = edgesArrayPtr[trackEdges];
                trackEdges++;
            }
            if (trackEdges - 2 >= newEdgeCount - 1) break;

        }
    }

private:
    // Note: we store a copy of the bitmap
    const GBitmap fDevice;
};

std::unique_ptr<GCanvas> GCreateCanvas(const GBitmap& device) {
    return std::unique_ptr<GCanvas>(new MyCanvas(device));
}

/* my own drawing, using functions like this: canvas->func()
 * returning title of the artwork
 * dimensions are 256 x 256
 */
std::string GDrawSomething(GCanvas* canvas, GISize dim) {

    GColor color = {0.1, 0, 0, 1};
    GPaint bckgd = GPaint(color);
    bckgd.setBlendMode(GBlendMode::kSrc);
    canvas->drawPaint(bckgd);

    float distDelta = 0.2;
    float timesAround = 20;
    float xCenter = 255/2;
    float yCenter = 255/2;

    // red->yellow spiral
    float dist = 0;
    color = {1, 0, 0, 0};
    GPaint paint = GPaint(color);
    paint.setBlendMode(GBlendMode::kSrcOver);

    // red->yellow spiral
    dist = 0;
    color = {1, 0, 0, 0.2};
    paint.setColor(color);
    paint.setBlendMode(GBlendMode::kSrcOver);
    for (int angle = 360 * timesAround; angle > 0; angle -= 5) {
        float x = xCenter + cos(M_PI * angle/180) * dist;
        float y = yCenter + sin(M_PI * angle/180) * dist;
        // GRect newRect = {x-8, y-8, x+8, y+8};
        // canvas->drawRect(newRect, paint);
        GPoint shape[3] = {{xCenter, yCenter}, {x, y}, {x + 10, y + 10}};
        canvas->drawConvexPolygon(shape, 3, paint);
        dist += distDelta;
        // increase opacity
        color.a = 2 * angle / (3600 * timesAround);
        // shift hue
        color.g = angle / (360 * timesAround);
        paint.setColor(color);
    }

    return "sunny";
}

