/*
 *  Copyright 2023 Georgie Stammer
 */

#include "include/GCanvas.h"
#include "include/GRect.h"
#include "include/GColor.h"
#include "include/GBitmap.h"


// Helper functions below!

// divides by 255 without using / for optimization
static int Div255(int p) {
    // (p + 128) * 257 >> 16
    int ret = p + 128;
    ret *= 257;
    ret = ret >> 16;
    return ret;
}

// Takes 2 GPixels & blends into 1 GPixel w/ srcover
static GPixel BlendSrcOver(GPixel srcPixel, GPixel destPixel) {
    
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
    // int newR = sR + (dCoef * dR + 127) / 255;
    // int newG = sG + (dCoef * dG + 127) / 255;
    // int newB = sB + (dCoef * dB + 127) / 255;
    // int newA = sA + (dCoef * dA + 127) / 255;
    int newR = sR + Div255(dCoef * dR);
    int newG = sG + Div255(dCoef * dG);
    int newB = sB + Div255(dCoef * dB);
    int newA = sA + Div255(dCoef * dA);

    GPixel outPixel = GPixel_PackARGB(newA, newR, newG, newB);
    return outPixel;
}

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

// Creates an array of 8 GRects to form a diamond shape
static void rectsForDiamond(GRect* rectList, float x, float y, float r) {
    float iterateX = x - r;
    float iterateY = y;
    float left;
    float top;
    float right;
    float bottom;
    for (int i = 0; i < 8; i++) {
        // left = -1 * sqrt(pow(r, 2) - pow(iterateY - y, 2)) + x;      // tried to make a circle shape unsuccessfully
        // top = -1 * sqrt(pow(r, 2) - pow(iterateX - x, 2)) + y;
        left = iterateX;
        top = iterateY;

        right = x + (x - left);
        bottom = y + (y - top);
        rectList[i] = {left, top, right, bottom};
        iterateX += 0.125 * r;
        iterateY -= 0.125 * r;
    }
}


class MyCanvas : public GCanvas {
public:
    MyCanvas(const GBitmap& device) : fDevice(device) {}

    void clear(const GColor& color) override {
        /**
        *  Fill the entire canvas with the specified color, using SRC porter-duff mode.
        */

        GPixel newPixel = ColorToPixel(color);

        int x = fDevice.width();
        int y = fDevice.height();
        GPixel* pixelArrayPointer = fDevice.pixels();
        int numPixels = x * y;
        int currPixel = 0;

        while (currPixel < numPixels) {
            *pixelArrayPointer = newPixel;
            pixelArrayPointer++;
            // note: memory address = 32 bits / 4 bytes
            currPixel++;
       }
    }
    
    void fillRect(const GRect& rect, const GColor& color) override {
        /**
        *  Fill the rectangle with the color, using SRC_OVER porter-duff mode.
        *
        *  The affected pixels are those whose centers are "contained" inside the rectangle:
        *      e.g. contained == center > min_edge && center <= max_edge
        *
        *  Any area in the rectangle that is outside of the bounds of the canvas is ignored.
        */

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
        if (color.a <= 0) return;

        // locate starting & ending pixel of rectangle (in memory)
        // GPixel* rectPointer = get_pixel_addr(fDevice, roundedRect.fLeft, roundedRect.fTop);
        // GPixel* rectEndPointer = get_pixel_addr(fDevice, roundedRect.fRight, roundedRect.fBottom);
        // GPixel* rectRowStart = get_pixel_addr(fDevice, roundedRect.fLeft, roundedRect.fTop);
        
        // loop thru pixels of rectangle & replace them
        // GPixel srcPixel = ColorToPixel(color);
        // while (rectPointer <= rectEndPointer - canvasX) {
        //     GPixel destPixel = *rectPointer;
        //     GPixel newPixel = BlendSrcOver(srcPixel, destPixel);
        //     *rectPointer = newPixel;
        //     // skip to next row if current row is done
        //     if (rectPointer == rectRowStart + roundedRect.width() - 1) {
        //         rectRowStart += canvasX;
        //         rectPointer = rectRowStart;
        //     }
        //     else {
        //         rectPointer++;
        //     }
        // }

        GPixel srcPixel = ColorToPixel(color);

        // loop thru pixels of rectangle & replace them W/O BLENDING
        if (color.a == 1) {
            for (int y = roundedRect.fTop; y < roundedRect.fBottom; y++) {
                for (int x = roundedRect.fLeft; x < roundedRect.fRight; x++) {
                    GPixel* destPixel = get_pixel_addr(fDevice, x, y);
                    *destPixel = srcPixel;
                }
            }
        }

        // loop thru pixels of rectangle & replace them W/ BLENDING
        else {
            for (int y = roundedRect.fTop; y < roundedRect.fBottom; y++) {
                for (int x = roundedRect.fLeft; x < roundedRect.fRight; x++) {
                    GPixel* destPixel = get_pixel_addr(fDevice, x, y);
                    GPixel newPixel = BlendSrcOver(srcPixel, *destPixel);
                    *destPixel = newPixel;
                }
            }
        }
        
    }

private:
    // Note: we store a copy of the bitmap
    const GBitmap fDevice;
};

std::unique_ptr<GCanvas> GCreateCanvas(const GBitmap& device) {
    return std::unique_ptr<GCanvas>(new MyCanvas(device));
}

std::string GDrawSomething(GCanvas* canvas, GISize dim) {
    // Implement this, drawing into the provided canvas, and returning the title of your artwork.
    // as fancy as you like
    // ...
    // canvas->clear(...);
    // canvas->fillRect(...);
    // dimensions 256 x 256

    canvas->clear({1, 1, 1, 1});
    // GRect rectList[8];
    
    // // medium red, small
    // GColor color = {1, 0.4, 0.4, 0.2};
    // float x = 0;
    // float y = 0;
    // float r = 32;
    // while (x <= 256) {
    //     while (y <= 256) {
    //         rectsForDiamond(rectList, x, y, r);
    //         for (int i = 0; i < 8; i++) {
    //             canvas->fillRect(rectList[i], color);
    //         }
    //         y += 64;
    //     }
    //     y = 0;
    //     x += 64;
    // }

    // // more medium red, small
    // x = 32;
    // y = 32;
    // while (x <= 256) {
    //     while (y <= 256) {
    //         rectsForDiamond(rectList, x, y, r);
    //         for (int i = 0; i < 8; i++) {
    //             canvas->fillRect(rectList[i], color);
    //         }
    //         y += 64;
    //     }
    //     y = 32;
    //     x += 64;
    // }

    // // pink, big
    // color = {1, 0.3, 1, 0.2};
    // r = 64;
    // x = 64;
    // y = 64;
    // while (x <= 256) {
    //     while (y <= 256) {
    //         rectsForDiamond(rectList, x, y, r);
    //         for (int i = 0; i < 8; i++) {
    //             canvas->fillRect(rectList[i], color);
    //         }
    //         y += 128;
    //     }
    //     y = 64;
    //     x += 128;
    // }

    // // yellow, big
    // color = {1, 1, 0, 0.1};
    // r = 64;
    // x = 0;
    // y = 0;
    // while (x <= 256) {
    //     while (y <= 256) {
    //         rectsForDiamond(rectList, x, y, r);
    //         for (int i = 0; i < 8; i++) {
    //             canvas->fillRect(rectList[i], color);
    //         }
    //         y += 128;
    //     }
    //     y = 0;
    //     x += 128;
    // }

    // // light purple, small
    // // color = {0.7, 0.5, 1, 0.2};
    // // x = 0;
    // // y = 32;
    // // r = 32;
    // // while (x <= 256) {
    // //     while (y <= 256) {
    // //         rectsForDiamond(rectList, x, y, r);
    // //         for (int i = 0; i < 8; i++) {
    // //             canvas->fillRect(rectList[i], color);
    // //         }
    // //         y += 64;
    // //     }
    // //     y = 32;
    // //     x += 64;
    // // }


    float distDelta = 0.2;
    float timesAround = 8;
    float xCenter = 255/2;
    float yCenter = 255/2;

    // red->yellow spiral
    float dist = 0;
    GColor color = {1, 0, 0, 0};
    for (int angle = 0; angle < 360 * timesAround; angle += 5) {
        float x = xCenter + cos(M_PI * angle/180) * dist;
        float y = yCenter + sin(M_PI * angle/180) * dist;
        GRect newRect = {x-8, y-8, x+8, y+8};
        canvas->fillRect(newRect, color);
        dist += distDelta;
        // increase opacity
        color.a = 2 * angle / (3600 * timesAround);
        // shift hue
        color.g = angle / (360 * timesAround);
    }

    // // purple->red spiral
    // dist = 0;
    // color = {0.5, 0, 1, 0};
    // for (int angle = 90; angle < 360 * timesAround; angle += 2) {
    //     float x = xCenter + cos(M_PI * angle/180) * dist;
    //     float y = yCenter + sin(M_PI * angle/180) * dist;
    //     GRect newRect = {x-8, y-8, x+8, y+8};
    //     canvas->fillRect(newRect, color);
    //     dist += distDelta;
    //     // increase opacity
    //     color.a = 2 * angle / (3600 * timesAround);
    //     // shift hue
    //     color.b = 1 - angle / (360 * timesAround);
    //     color.r = 0.5 + angle / (360 * timesAround * 2);
    // }

    // // blue->purple spiral
    // dist = 0;
    // color = {0, 0, 1, 0};
    // for (int angle = 180; angle < 360 * timesAround; angle += 2) {
    //     float x = xCenter + cos(M_PI * angle/180) * dist;
    //     float y = yCenter + sin(M_PI * angle/180) * dist;
    //     GRect newRect = {x-8, y-8, x+8, y+8};
    //     canvas->fillRect(newRect, color);
    //     dist += distDelta;
    //     // increase opacity
    //     color.a = 2 * angle / (3600 * timesAround);
    //     // shift hue
    //     color.r = angle / (360 * timesAround * 2);
    // }

    return "Diamonds in the Sky";
}

