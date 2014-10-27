/*
 *  antialias.h
 *
 *  Copyright (C) 2004, 2008, 2014 - Niko Ritari
 *
 *  This file is part of Outgun.
 *
 *  Outgun is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Outgun is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Outgun; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/** @file
 * The full-scene antialiasing system (public interface to it).
 */

#ifndef ANTIALIAS_H_INC
#define ANTIALIAS_H_INC

#include <vector>
#include <list>
#include "incalleg.h"
#include "nassert.h"
#include "utility.h"

// // // // internal definitions

/** A continuous span of partially rendered pixels.
 *
 * "Alpha" values are actually fractions of a full pixel, not a measure of
 * translucency as such, and must be scaled so that a full pixel is
 * 1 << scale (constant).
 *
 * A less than fully filled pixel is completed with black when drawn.
 */
class PartialPixelSegment {
public:
    enum { scale = 20 /**< Alphas are scaled by 1 << scale: (1 << scale) / 2 equals half transparent.*/ };

    PartialPixelSegment(int x0) throw () : startx(x0) { }
    int x0() const throw () { return startx; }
    int len() const throw () { return pixels.size(); }
    void add(int index, int color, int alpha) throw () { pixels[index].add(color, alpha); }
    void extend(int color, int alpha) throw () { pixels.push_back(PartPix(color, alpha)); }
    void draw(BITMAP* buf, int y) const throw ();

private:
    /// A single partially rendered pixel.
    class PartPix {
        int r, g, b, alphaTotal; ///< all scaled
        enum { scale = PartialPixelSegment::scale, scaleVal = 1 << PartialPixelSegment::scale };

    public:
        PartPix(int color, int alpha) throw () :
            r(getr(color) * alpha),
            g(getg(color) * alpha),
            b(getb(color) * alpha),
            alphaTotal(alpha)
        { }
        void add(int color, int alpha) throw () {
            r += getr(color) * alpha;
            g += getg(color) * alpha;
            b += getb(color) * alpha;
            alphaTotal += alpha;
        }
        bool draw() const throw () { return alphaTotal >= scaleVal / 100; }
        int color() const throw () {
            // this ensures that only whole pixels are written; enable if that's true:  numAssert(alphaTotal >= .999, alphaTotal * 10000.);
            numAssert(alphaTotal <= scaleVal * 1.001, alphaTotal);
            return makecol((r + scaleVal / 2) >> scale, (g + scaleVal / 2) >> scale, (b + scaleVal / 2) >> scale);
        }
        /** Get color value where more than a full pixel may have been added.
         * If the pixel is more than full, the color is the average color over
         * that larger area and the extra area is ignored.
         */
        int flexColor() const throw () {
            #if 1
            int rc, gc, bc;
            if (alphaTotal > scaleVal * 1.001) {
                rc = (r + scaleVal / 2) / alphaTotal;
                gc = (g + scaleVal / 2) / alphaTotal;
                bc = (b + scaleVal / 2) / alphaTotal;
            }
            else {
                rc = (r + scaleVal / 2) >> scale;
                gc = (g + scaleVal / 2) >> scale;
                bc = (b + scaleVal / 2) >> scale;
            }
            #elif 1
            // alternative cutting method: use the extra intensity as much as possible, possibly distorting the color
            int rc = std::min(255, (r + scaleVal / 2) >> scale);
            int gc = std::min(255, (g + scaleVal / 2) >> scale);
            int bc = std::min(255, (b + scaleVal / 2) >> scale);
            #elif 1
            // alternative cutting method: use the extra intensity but dim all components (subtract constant) if nonrepresentable
            int rc = (r + scaleVal / 2) >> scale;
            int gc = (g + scaleVal / 2) >> scale;
            int bc = (b + scaleVal / 2) >> scale;
            if (alphaTotal >= scaleVal && (rc > 255 || gc > 255 || bc > 255)) { // the alphaTotal check is an optimization
                int cut = max(max(rc, gc), bc) - 255;
                rc = std::max(0, rc - cut);
                gc = std::max(0, gc - cut);
                bc = std::max(0, bc - cut);
            }
            #endif
            return makecol(rc, gc, bc);
        }
    };

    int startx;
    std::vector<PartPix> pixels;
};

class RectWall;
class TriWall;
class CircWall;
class WallBase;
class BorderFunctionBase;
class LineFunction;
class DrawElement;

/// A segment of the border of an object defined by an f(y) and the range of y.
struct WallBorderSegment {
    BorderFunctionBase* fn;
    double y0, y1;
    WallBorderSegment() throw () { }
    WallBorderSegment(BorderFunctionBase* fn_, double y0_, double y1_) throw () : fn(fn_), y0(y0_), y1(y1_) { }
};

/** The definition of a drawable object.
 * Contains a set of non-intersecting borders that must form some number of
 * closed loops (when completed with implicit horizontal segments as needed), and
 * texturing information for the inside.
 */
struct ObjectSource {
    int texid;
    bool overlay; ///< Set if the texture is not fully opaque.
    std::vector<WallBorderSegment> borders;
};

/// Data describing a constant color & alpha.
struct SolidTexdata {
    int color;
    int alpha; ///< Translucency, range [0,256] where 256 is opaque and 0 invisible (useless).

    void set(int color_, int alpha_ = 256) throw () { color = color_; alpha = alpha_; }
};

/** Data describing texturization by tiling with a bitmap.
 *
 * Because of Allegro's limitations, if alpha == 256 (much faster), the width
 * and height of the image must be (possibly different) powers of two. This is
 * not checked!
 *
 * A conceptual "master copy" of the image is anchored by its top left corner to
 * (x0,y0) on screen. Other copies are placed elsewhere starting immediately to
 * the right and down from the master copy, tiling the screen.
 *
 * A texture can not be used in areas where x < tex.x0 || y < tex.y0. This can
 * be circumvented by selecting x0 and y0 < 0.
 */
struct TextureTexdata {
    BITMAP* image; // not const because fed to Allegro
    int x0, y0; ///< Screen position corresponding to the possible copy of the top-left corner of the image. Can be negative.
    int alpha; ///< Translucency, range [0,256] where 256 is opaque and 0 invisible (useless).

    void set(BITMAP* texture, int x0_ = 0, int y0_ = 0, int alpha_ = 256) throw () { image = texture; alpha = alpha_; x0 = x0_; y0 = y0_; }
};

/** Data describing texturization with a flag marker.
 *
 * Describes a specific variably translucent circular pattern stretched to the given radius.
 */
struct FlagmarkerTexdata {
    int color;
    double cx, cy;
    double radius;

    void set(int color_, double cx_, double cy_, double r) throw () { color = color_; cx = cx_; cy = cy_; radius = r; }
};

// // // // public interface

/** Description of a single texture used to fill elements on screen.
 *
 * A texture can be more generally any source that produces color/alpha values
 * for pixels on screen.
 *
 * Screen is not literal but the target bitmap whatever it is.
 */
class TextureData {
public:
    void setSolid(int color, int alpha = 256) throw () { t = T_solid; d.s.set(color, alpha); } ///< See SolidTexdata
    void setTexture(BITMAP* texture, int x0 = 0, int y0 = 0, int alpha = 256) throw () { t = T_texture; d.t.set(texture, x0, y0, alpha); } ///< See TextureTexdata
    void setFlagmarker(int color, double cx, double cy, double r) throw () { t = T_flagmarker; d.f.set(color, cx, cy, r); } ///< See FlagmarkerTexdata

    enum TexType { T_solid, T_texture, T_flagmarker };
    typedef union {
        SolidTexdata s;
        TextureTexdata t;
        FlagmarkerTexdata f;
    } TexdataUnion;

    TexType type() const throw () { return t; }
    const TexdataUnion& data() const throw () { return d; }

private:
    TexType t;
    TexdataUnion d;
};

/** Handler of the target buffer and source texture data.
 *
 * Used by SceneAntialiaser to render one draw element at a time. Half-written
 * pixels are stored separately and only drawn in the buffer at once in the end.
 */
class Texturizer {
public:
    Texturizer(BITMAP* buffer, int x0, int y0, const std::vector<TextureData>& textures) throw ()
                    : buf(buffer), bx0(x0), by0(y0), texTab(textures), partials(buffer->h) { }

    /** Render a single draw element.
     * @param textures Used textures as indices to the texture table, in layer order.
     * @param elp      Element to draw. Its textures are overridden by @a textures.
     */
    void render(const std::vector<int>& textures, const DrawElement* elp) throw ();
    void finalize() throw (); ///< Draw all buffered pixels. Use exactly once at the end, before destroying.

// semi-private: for use by rendering functions called by render() only
    void setLine(int y) throw (); ///< Set line where the next pixels will be drawn. Invalidates x-coordinate.
    void nextLine() throw (); ///< Increase target line by one. Invalidates x-coordinate.
    inline void startPixSpan(int x) throw (); ///< Set x-coordinate where the next span will start (next pixel drawn). Semi-expensive.
    inline void putPix(int color, int alpha) throw (); ///< Draw pixel at the current position and increase x by one.

    inline BITMAP* getBuf() throw () { return buf; }
    inline int getbx() const throw () { return bx; }
    inline int getby() const throw () { return by; }
    inline int getbx0() const throw () { return bx0; }
    inline int getby0() const throw () { return by0; }

private:
    BITMAP* buf;
    int bx, by; ///< Active pixel in buf.
    int bx0, by0; ///< Buffer pixel offset. Pixel (0,0) is drawn at (bx0,by0) in buf.

    const std::vector<TextureData>& texTab;

    std::vector< std::list<PartialPixelSegment> > partials;
    PartialPixelSegment* partSpan; ///< Active span in partials.
    int spanIndex; ///< Active index in *partSpan. partSpan->pixels[spanIndex] corresponds to (bx,by) in buf.
    int spanEnd; ///< Maximal size of *partSpan, when we must move to the next segment to continue adding pixels.
};

/** Full scene antialiasing, together with Texturizer.
 *
 * Collects drawn objects, scales and clips them to fit the viewport (or subset
 * thereof), and renders them using a Texturizer.
 *
 * When rendering, every fraction of a pixel must fit the buffer given to
 * Texturizer after scaling, clipping, and offsetting by the origin given to
 * Texturizer. Whole pixels with no content are not drawn, partial pixels are
 * completed with black.
 *
 * Texture numbers given to the add methods are incides and must correspond to
 * entries in the TextureData vector given to Texturizer.
 */
class SceneAntialiaser : private NoCopying {
public:
    SceneAntialiaser() throw () { }
    ~SceneAntialiaser() throw ();

    /** Set scaling applied to future adds and setClipping.
     *
     * All coordinates of added objects will be first multiplied by scale, then
     * translated so that (0,0) is drawn to (x0,y0). (further translated when
     * rendered by the origin coords given to Texturizer)
     *
     * Must be called at least once before the first add or setClipping. The
     * same scaling applies to all adds and calls to setClipping until
     * setScaling is called again.
     */
    void setScaling(double x0_ = 0, double y0_ = 0, double scale_ = 1.) throw ();

    void addRectangle(double x1, double y1, double x2, double y2, int texture, bool overlay = false) throw (); ///< Add given rectangle. Overlay must be set iff the texture is not opaque (alpha!=256).
    void addRectWall(const RectWall& wall, int texture) throw (); ///< Add given wall. Texture must be opaque (alpha=256).
    void addTriWall (const  TriWall& wall, int texture) throw (); ///< Add given wall. Texture must be opaque (alpha=256).
    void addCircWall(const CircWall& wall, int texture) throw (); ///< Add given wall. Texture must be opaque (alpha=256).
    void addWall    (const WallBase* wall, int texture) throw (); ///< Add given wall. Texture must be opaque (alpha=256).

    /** Set clipping rectangle applied to future addClippeds and clips.
     *
     * The coordinates are scaled just like object coordinates but only once
     * when the clipping is set. If you wish to clip to coordinates in the
     * target bitmap, call setScaling() before setClipping, and
     * setScaling(x,y,s) with appropriate parameters after.
     */
    void setClipping(double x1, double y1, double x2, double y2) throw ();
    void clipAll() throw () { clipFrom(0); } ///< Clip all already added objects to the current clipping rectangle.

    int getClipPos() const throw () { return objects.size(); } ///< Get position for calling clipFrom later.
    void clipFrom(int base) throw (); ///< Clip all objects added after getClipPos() returned @a base, and before this call.

    void addRectWallClipped(const RectWall& wall, int texture) throw (); ///< Add and clip given wall. Texture must be opaque (alpha=256).
    void addTriWallClipped (const  TriWall& wall, int texture) throw (); ///< Add and clip given wall. Texture must be opaque (alpha=256).
    void addCircWallClipped(const CircWall& wall, int texture) throw (); ///< Add and clip given wall. Texture must be opaque (alpha=256).
    void addWallClipped    (const WallBase* wall, int texture) throw (); ///< Add and clip given wall. Texture must be opaque (alpha=256).

    void render(Texturizer& tex) const throw (); ///< Render all objects using @a tex.

private:
    void createClipFns() throw ();
    void clip(ObjectSource& object) throw ();

    std::vector<BorderFunctionBase*> bfns;
    std::vector<ObjectSource> objects;
    LineFunction* clipLeft, * clipRight;
    bool clipFunctionsValid;
    double x0, y0, scale;
    double clipx1, clipy1, clipx2, clipy2;
};

#endif  // ANTIALIAS_H_INC
