/*
 *  Copyright 2023 Georgie Stammer
 */

#include "include/GCanvas.h"
#include "include/GRect.h"
#include "include/GColor.h"
#include "include/GBitmap.h"
#include "include/GShader.h"

#include "BlendFunctions.h"
#include "Edge.h"
#include <iostream>
#include <algorithm>
#include <stack>

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

GBlendMode optimizeMode(GShader* shaderPtr, GBlendMode mode, float alpha) {
    // optimize when alpha is 0
    if (shaderPtr==nullptr && alpha == 0) {
        if (mode == GBlendMode::kSrcIn || mode == GBlendMode::kDstIn ||
            mode == GBlendMode::kSrcOut || mode == GBlendMode::kDstATop) {
                mode = GBlendMode::kClear;
            }
    }

    // optimize when alpha is 1
    if (shaderPtr==nullptr && alpha == 1) {
        if (mode == GBlendMode::kSrcOver) mode = GBlendMode::kSrc;
        if (mode == GBlendMode::kDstIn) mode = GBlendMode::kDst;
        if (mode == GBlendMode::kDstOut) mode = GBlendMode::kClear;
        if (mode == GBlendMode::kSrcATop) mode = GBlendMode::kSrcIn;
        if (mode == GBlendMode::kDstATop) mode = GBlendMode::kDstOver;
        if (mode == GBlendMode::kXor) mode = GBlendMode::kSrcOut;
    }

    // optimize when shader is opaque
    if (shaderPtr!=nullptr && shaderPtr->isOpaque()) {
        if (mode == GBlendMode::kSrcOver) mode = GBlendMode::kSrc;
        if (mode == GBlendMode::kDstIn) mode = GBlendMode::kDst;
        if (mode == GBlendMode::kDstOut) mode = GBlendMode::kClear;
        if (mode == GBlendMode::kSrcATop) mode = GBlendMode::kSrcIn;
        if (mode == GBlendMode::kDstATop) mode = GBlendMode::kDstOver;
        if (mode == GBlendMode::kXor) mode = GBlendMode::kSrcOut;
    }

    return mode;
}


// prints Edge array for debugging
void printEdgeArray(Edge* edgesArrayPtr, int count) {
    for (int i = 0; i < count; i++) {
        Edge x = edgesArrayPtr[i];
        printf("edge (%f, %f) - (%f, %f)\n", x.p1.fX, x.p1.fY, x.p2.fX, x.p2.fY);
    }
}

// prints array of GPoints for debugging
void printPoints(GPoint pts[], int count) {
    printf("New points:\n");
    for (int i = 0; i < count; i++) {
        printf("%f, %f   ", pts[i].fX, pts[i].fY);
    }
    printf("\n");
}


class MyCanvas : public GCanvas {
public:
    MyCanvas(const GBitmap& device) : fDevice(device) {}

    // stores current transformation matrices (CTMs) in a stack
    std::stack<GMatrix> mxStack;
    // the CTM can then be referenced via mxStack.top()

    /**
     *  Save off a copy of the canvas state (CTM), to be later used if the balancing call to
     *  restore() is made. Calls to save/restore can be nested:
     *  save();
     *      save();
     *          concat(...);    // this modifies the CTM
     *          .. draw         // these are drawn with the modified CTM
     *      restore();          // now the CTM is as it was when the 2nd save() call was made
     *      ..
     *  restore();              // now the CTM is as it was when the 1st save() call was made
     */
    void save() {
        if (mxStack.empty()) {
            GMatrix id = GMatrix();
            mxStack.push(id);
        }
        GMatrix mx = GMatrix(mxStack.top());
        mxStack.push(mx);
    }

    /**
     *  Copy the canvas state (CTM) that was record in the correspnding call to save() back into
     *  the canvas. It is an error to call restore() if there has been no previous call to save().
     */
    void restore() {
        mxStack.pop();
    }

    /**
     *  Modifies the CTM by preconcatenating the specified matrix with the CTM. The canvas
     *  is constructed with an identity CTM.
     *
     *  CTM' = CTM * matrix
     */
    void concat(const GMatrix& matrix) {
        if (mxStack.empty()) {
            GMatrix id = GMatrix();
            mxStack.push(id);
        }
        GMatrix newMatrix = GMatrix::Concat(mxStack.top(), matrix);
        mxStack.pop();
        mxStack.push(newMatrix);
    }

    /**
     *  Template to loop thru rows & replace each pixel with a new blended pixel
     *  with no shader
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
     *  Template to loop thru rows & replace each pixel with a new blended pixel
     *  and using the specified shader
     *  For filling entire device screen, rectPtr is nullptr
     */
    template <typename Method>
    void drawRowsShader(GShader* shaderPtr, GIRect* rectPtr, Method bl) {
        GIRect rect;
        // if no rect, make rect of full device screen
        if (rectPtr == nullptr) {
            rect = {0, 0, fDevice.width(), fDevice.height()};
        }
        else rect = *rectPtr;

        int startX = rect.fLeft;
        int count = rect.fRight - startX;
        GPixel src[count];

        // loop thru rows of rectangle
        for (int y = rect.fTop; y < rect.fBottom; y++) {
            shaderPtr->shadeRow(startX, y, count, src);
            for (int x = rect.fLeft; x < rect.fRight; x++) {
                GPixel* dstPtr = get_pixel_addr(fDevice, x, y);
                GPixel blendedPixel = bl(src[x - startX], *dstPtr);
                *dstPtr = blendedPixel;
            }
        }
    }

    /**
     *  Helper function that handles the switch case for choosing a blend mode
     *  & then calls drawRows to draw
     */
    void blendAndDraw(GBlendMode mode, GShader* shader, GPixel src, GIRect* rectPtr) {
        switch (mode) {
            case GBlendMode::kClear:
                {
                    if (shader == nullptr) drawRows(src, rectPtr, blendClear);
                    else drawRowsShader(shader, rectPtr, blendClear);
                }
                break;
            case GBlendMode::kSrc:
                {
                    if (shader == nullptr) drawRows(src, rectPtr, blendSrc);
                    else drawRowsShader(shader, rectPtr, blendSrc);
                }
                break;
            case GBlendMode::kDst:
                // nothing changes!
                break;
            case GBlendMode::kSrcOver:
                {
                    if (shader == nullptr) drawRows(src, rectPtr, blendSrcOver);
                    else drawRowsShader(shader, rectPtr, blendSrcOver);
                }
                break;
            case GBlendMode::kDstOver:
                {
                    if (shader == nullptr) drawRows(src, rectPtr, blendDstOver);
                    else drawRowsShader(shader, rectPtr, blendDstOver);
                }
                break;
            case GBlendMode::kSrcIn:
                {
                    if (shader == nullptr) drawRows(src, rectPtr, blendSrcIn);
                    else drawRowsShader(shader, rectPtr, blendSrcIn);
                }
                break;
            case GBlendMode::kDstIn:
                {
                    if (shader == nullptr) drawRows(src, rectPtr, blendDstIn);
                    else drawRowsShader(shader, rectPtr, blendDstIn);
                }
                break;
            case GBlendMode::kSrcOut:
                {
                    if (shader == nullptr) drawRows(src, rectPtr, blendSrcOut);
                    else drawRowsShader(shader, rectPtr, blendSrcOut);
                }
                break;
            case GBlendMode::kDstOut:
                {
                    if (shader == nullptr) drawRows(src, rectPtr, blendDstOut);
                    else drawRowsShader(shader, rectPtr, blendDstOut);
                }
                break;
            case GBlendMode::kSrcATop:
                {
                    if (shader == nullptr) drawRows(src, rectPtr, blendSrcATop);
                    else drawRowsShader(shader, rectPtr, blendSrcATop);
                }
                break;
            case GBlendMode::kDstATop:
                {
                    if (shader == nullptr) drawRows(src, rectPtr, blendDstATop);
                    else drawRowsShader(shader, rectPtr, blendDstATop);
                }
                break;
            case GBlendMode::kXor:
                {
                    if (shader == nullptr) drawRows(src, rectPtr, blendXor);
                    else drawRowsShader(shader, rectPtr, blendXor);
                }
                break;
        }
    }

    /**
     *  Fill the entire canvas with the specified color, using the specified blendmode.
     */
    void drawPaint(const GPaint& paint) {
        // set up CTM
        if (mxStack.empty()) {
            GMatrix id = GMatrix();
            mxStack.push(id);
        }
        GMatrix CTM = mxStack.top();

        // if there is a shader, set context
        GShader* shaderPtr = paint.getShader();
        if (shaderPtr != nullptr) {
            bool sh = shaderPtr->setContext(CTM);
            if (!sh) return;
        }

        // establish pixel & blend mode to paint with
        GPixel newPixel = ColorToPixel(paint.getColor());
        GBlendMode mode = paint.getBlendMode();

        // optimize based on modes & opacity
        if (shaderPtr==nullptr && paint.getAlpha() == 0) {
            if (mode == GBlendMode::kSrcOver || mode == GBlendMode::kDstOver ||
                mode == GBlendMode::kDstOut || mode == GBlendMode::kSrcATop) return;
        }
        mode = optimizeMode(shaderPtr, mode, paint.getAlpha());

        // loop thru canvas based on which blend mode is being used
        blendAndDraw(mode, shaderPtr, newPixel, nullptr);

    }
    
    /**
     *  Fill the rectangle with the color, using the specified blendmode.
     *
     *  The affected pixels are those whose centers are "contained" inside the rectangle:
     *      e.g. contained == center > min_edge && center <= max_edge
     */
    void drawRect(const GRect& rect, const GPaint& paint) {
        // set up CTM
        if (mxStack.empty()) {
            GMatrix id = GMatrix();
            mxStack.push(id);
        }
        GMatrix CTM = mxStack.top();

        // if there is a shader, set context
        GShader* shaderPtr = paint.getShader();
        if (shaderPtr != nullptr) {
            bool sh = shaderPtr->setContext(CTM);
            if (!sh) return;
        }

        // map points based on CTM
        GPoint cornerPts[4] = {{rect.fLeft, rect.fTop}, {rect.fRight, rect.fTop},
                                {rect.fRight, rect.fBottom}, {rect.fLeft, rect.fBottom}, };
        GPoint newCornerPts[4];
        CTM.mapPoints(newCornerPts, cornerPts, 4);

        // if the rectangle has been rotated, treat it as a polygon instead
        bool isRotated = newCornerPts[0].fY != newCornerPts[1].fY;
        if (isRotated) {
            drawConvexPolygon(cornerPts, 4, paint);
            return;
        }

        // if it wasn't rotated, continue by making the transformed rect
        GRect newRect = {newCornerPts[0].fX, newCornerPts[0].fY,
                        newCornerPts[2].fX, newCornerPts[2].fY};

        // round rectangle into GIRect
        GIRect roundedRect = newRect.round();
        
        // establish pixel & blend mode & shader to paint with
        GPixel srcPixel = ColorToPixel(paint.getColor());
        GBlendMode mode = paint.getBlendMode();

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
        
        // optimize based on modes & opacity
        if (shaderPtr==nullptr && paint.getAlpha() == 0) {
            if (mode == GBlendMode::kSrcOver || mode == GBlendMode::kDstOver ||
                mode == GBlendMode::kDstOut || mode == GBlendMode::kSrcATop) return;
        }
        mode = optimizeMode(shaderPtr, mode, paint.getAlpha());

        blendAndDraw(mode, shaderPtr, srcPixel, &roundedRect);
    }

    /**
     *  Fill the convex polygon with the color and blendmode,
     *  following the same "containment" rule as rectangles.
     */
    void drawConvexPolygon(const GPoint points[], int count, const GPaint& paint) {

        if (count <= 2) return;
        
        // set up CTM
        if (mxStack.empty()) {
            GMatrix id = GMatrix();
            mxStack.push(id);
        }
        GMatrix CTM = mxStack.top();

        // if there is a shader, set context
        GShader* shaderPtr = paint.getShader();
        if (shaderPtr != nullptr) {
            bool sh = shaderPtr->setContext(CTM);
            if (!sh) return;
        }

        // map points based on CTM
        GPoint newPts[count];
        CTM.mapPoints(newPts, points, count);

        // get paint info
        GPixel srcPixel = ColorToPixel(paint.getColor());
        GBlendMode mode = paint.getBlendMode();
        
        // optimize based on modes & opacity
        if (shaderPtr==nullptr && paint.getAlpha() == 0) {
            if (mode == GBlendMode::kSrcOver || mode == GBlendMode::kDstOver ||
                mode == GBlendMode::kDstOut || mode == GBlendMode::kSrcATop) return;
        }
        mode = optimizeMode(shaderPtr, mode, paint.getAlpha());

        // contruct all edges
        Edge allEdges[count];
        for (int i = 0; i < count - 1; i++) {
            allEdges[i] = Edge(newPts[i], newPts[i+1]);
        }
        // add last edge
        allEdges[count - 1] = Edge(newPts[count - 1], newPts[0]);

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
                blendAndDraw(mode, shaderPtr, srcPixel, &rowRect);
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



class WaveShader : public GShader {
    const GMatrix fLocalMatrix;
    const float fScale;
    const float fWaveDepth;

    GMatrix fInverse;
    
public:
    WaveShader(float scale, float depth)
        : fLocalMatrix(GMatrix()), fScale(scale), fWaveDepth(depth)
    {}
    
    bool isOpaque() override {
        return true;
    }
    
    bool setContext(const GMatrix& ctm) override {
        return (ctm * fLocalMatrix).invert(&fInverse);
    }
    
    void shadeRow(int x, int y, int count, GPixel row[]) override {
        GColor color;
        for (int i = 0; i < count; ++i) {
            float newY = y + (float)sin(fScale * i) * fWaveDepth;
            float rad = fScale * newY / M_PI;
            color = {abs((float)sin(rad)), 1, 0, 1};
            row[i] = ColorToPixel(color);
        }
    }
};

/* my own drawing, using functions like this: canvas->func()
 * returning title of the artwork
 * dimensions are 256 x 256
 */
std::string GDrawSomething(GCanvas* canvas, GISize dim) {
    
    canvas->save();
    WaveShader waveSh = WaveShader(0.3, 7);
    GPaint bgPaint;
    bgPaint.setShader(&waveSh);
    GRect full = {0, 0, 256, 256};
    canvas->drawRect(full, bgPaint);

    canvas->restore();

    GBitmap bm;
    bm.readFromFile("mypngs/spongebob.png");
    float cx = bm.width() * 0.5f;
    float cy = bm.height() * 0.5f;
    GPoint pts[] = {
        { cx, 0 }, { 0, cy }, { cx, bm.height()*1.0f }, { bm.width()*1.0f, cy },
    };

    auto shader = GCreateBitmapShader(bm, GMatrix());
    GPaint paint(shader.get());

    GPoint midpts[] = {{0, 0}, {0, bm.height()}, {bm.width(), bm.height()}, {bm.width(), 0}};
    canvas->save();
    canvas->translate(90, 90);
    canvas->scale(0.1f, 0.1f);
    canvas->drawConvexPolygon(midpts, 4, paint);
    canvas->restore();

    canvas->save();
    int n = 17;
    canvas->scale(0.1f, 0.1f);
    for (int i = 0; i < n; ++i) {
        float radians = i * M_PI * 2 / n;
        canvas->save();
        canvas->translate(cx*3, cx*3);
        canvas->rotate(radians);
        canvas->translate(cx, -cy);
        canvas->drawConvexPolygon(pts, 4, paint);
        canvas->restore();
    }
    canvas->restore();

    canvas->save();
    n = 7;
    canvas->scale(0.1f, 0.1f);
    for (int i = 0; i < n; ++i) {
        float radians = i * M_PI * 2 / n;
        canvas->save();
        canvas->translate(cx*3, cx*3);
        canvas->rotate(radians);
        // canvas->translate(cx, -cy);
        canvas->drawConvexPolygon(pts, 4, paint);
        canvas->restore();
    }
    canvas->restore();

    return "me";
}

