/*
 *  antialias_internal.h
 *
 *  Copyright (C) 2004, 2014 - Niko Ritari
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
 * Internal definitions of the antialiasing system.
 */

#ifndef ANTIALIAS_INTERNAL_H_INC
#define ANTIALIAS_INTERNAL_H_INC

#include <vector>
#include <list>
#include "antialias.h"

//#define DEBUG_SPLIT
//#define DEBUG_OVERLAP
//#define DEBUG_RENDER
//#define CERR_DEBUG

#ifdef CERR_DEBUG
#include <iostream>
using std::cerr;
#endif

// INTERSECTION_TRESHOLD    is how much inside a segment an intersection is allowed (must allow for rounding errors)
// SPLIT_TRESHOLD           is how much y-borders are allowed to deviate (must exceed the precision of the given y coords)
// FINAL_EXTREMECUT         is how much of each element is removed y-wise to protect from exceeding function range
// X_EXTREMECUT             is how much of each pixel edge is removed x-wise to counter imprecision (a too little cut is not fatal but does trigger an assertion)
// JOIN_TRESHOLD            is how much gap is allowed when joining two adjacent objects to one
// all tresholds' unit is a pixel
// these settings currently tune the module to work with values of y up to approximately +-1e7
// they have to be relaxed in order to gain more range
const double INTERSECTION_TRESHOLD = .0000001;  // roughly 2^-23
const double SPLIT_TRESHOLD        = .0000002;
const double FINAL_EXTREMECUT      = .000001;
const double X_EXTREMECUT          = .000001;
const double JOIN_TRESHOLD         = .001;

/** The relative position of a function to a line for every value.
 *
 * Contains the starting side compared to the (externally defined) line and an
 * ordered list of parameter values where that side changes, i.e. where the
 * function crosses the line.
 */
struct ChangePoints {
    /// On which side of the given line the function is at a given value of y.
    enum Side { S_Left, S_Right };

    Side startSide; ///< Side when y < points[0]
    double points[3]; ///< List of y-values where the side changes, terminated with a huge value (set at 1e99).
    // 3 is the logical maximum of points used; increase if more are needed by complex functions
    // the C array is an optimization: using a vector would significantly slow things down and wouldn't help at all
};

/** A function defining a continuous curve, used as a border of drawable elements.
 * - The curve is specified as a function of y, and so can have at most one point
 *   at each y-coordinate.
 * - The shape is restricted: there can be at most one y-coordinate where the
 *   function changes from increasing to decreasing or vice versa. (can be relaxed)
 * - The range of valid or in-use values must be tracked by the user.
 * - Intersections between any pairs of functions are calculated separately, using
 *   dynamic_cast and friend functions.
 */
class BorderFunctionBase {
protected:
    BorderFunctionBase() throw () { }

public:
    virtual ~BorderFunctionBase() throw () { }
    virtual double operator()(double y) const throw () = 0; ///< Get the x-coordinate at y (which must be in valid range)
    virtual ChangePoints getChangePoints(double x) const throw () = 0; ///< Get the value's side from the given x-coordinate at each y
    virtual double spanLeftSideIntegral(double x0, double y0, double y1) const throw () = 0; ///< Calculate the integral with y from y0 to y1 of f(y) - x0
    virtual bool centerExtremes() const throw () = 0; ///< Return true if the function is not monotonic. I.e. if an extreme x coordinate of any y-interval might not be at either of the borders.
    virtual double extremeY() const throw () = 0; ///< If centerExtremes() is true, return the y coordinate at the point of non-monotonicity
    virtual double extremeX() const throw () = 0; ///< If centerExtremes() is true, return the x coordinate at the point of non-monotonicity (f.extremeX() == f(f.extremeY()) except for FP inaccuracies)
    virtual bool operator==(const BorderFunctionBase& o) throw () = 0;
    virtual void debug() const throw () { }
};

class LineFunction;

/// A half-circle defined by center, radius and left/right side.
class CurveFunction : public BorderFunctionBase {
    double cx, cy, r, r2;
    double sideMul;

    friend double getIntersection(LineFunction* f1, CurveFunction* f2, double miny) throw ();
    friend double getIntersection(CurveFunction* f1, CurveFunction* f2, double miny) throw ();

public:
    CurveFunction(double cx_, double cy_, double r_, bool rightSide) throw () : cx(cx_), cy(cy_), r(r_), r2(r_*r_), sideMul(rightSide?+1:-1) { }
    double operator()(double y) const throw ();
    ChangePoints getChangePoints(double x) const throw ();
    double spanLeftSideIntegral(double x0, double y0, double y1) const throw ();
    bool centerExtremes() const throw () { return true; }
    double extremeY() const throw () { return cy; }
    double extremeX() const throw () { return cx + sideMul * r; }
    bool operator==(const BorderFunctionBase& o) throw ();
    void debug() const throw ();
};

/// A line defined by two points on it.
class LineFunction : public BorderFunctionBase {
    double px1, py1, px2, py2;
    double ratio;

    friend double getIntersection(LineFunction* f1, LineFunction* f2) throw ();
    friend double getIntersection(LineFunction* f1, CurveFunction* f2, double miny) throw ();

public:
    LineFunction(double x1, double y1, double x2, double y2) throw () : px1(x1), py1(y1), px2(x2), py2(y2), ratio((x2 - x1)/(y2 - y1)) { }
    double operator()(double y) const throw ();
    ChangePoints getChangePoints(double x) const throw ();
    double spanLeftSideIntegral(double x0, double y0, double y1) const throw ();
    bool centerExtremes() const throw () { return false; }
    double extremeY() const throw () { nAssert(0); return 0; }
    double extremeX() const throw () { nAssert(0); return 0; }
    bool operator==(const BorderFunctionBase& o) throw ();
    void debug() const throw ();
};

/** A simple drawable element.
 *
 * Defined by its borders and the textures to be applied to the area within.
 * The left and right border are given by border functions. For top and bottom,
 * y-coordinates are given that either are parts of the borders as straight
 * lines, or the left and right borders meet there.
 *
 * Must hold: fLeft(y) <= fRight(y) for all y0 <= y <= y1
 */
class DrawElement {
    BorderFunctionBase* fLeft, * fRight;    // these are not owned: the object pointed to must live as long as this DrawElement is used, and must be manually deleted
    double y0, y1;
    std::vector<int> texid;

public:
    DrawElement(BorderFunctionBase* flp, BorderFunctionBase* frp, double y0_, double y1_, std::vector<int> tex) throw ();
    void extendDown(double y1_) throw () { nAssert(y1_ > y1); y1 = y1_; }
    const BorderFunctionBase& getLeft() const throw () { return *fLeft; }
    const BorderFunctionBase& getRight() const throw () { return *fRight; }
    double getY0() const throw () { return y0; }
    double getY1() const throw () { return y1; }
    int getBaseTex() const throw () { return texid.front(); }
    const std::vector<int>& getAllTextures() const throw () { return texid; }
    bool isJoinable(const DrawElement& o) const throw (); ///< Return true if the elements share their border functions and textures, and @a o immediately follows *this y-wise.
};

/** A simple segment of y-axis used to build draw elements.
 *
 * Represents a segment of y-axis where all draw elements span the whole height
 * and their border functions don't intersect.
 *
 * Contains both the completed elements, and a growing list of borders that
 * represents an object of a single texture being built. When finished, this
 * object is added "on top" of the existing elements with a moveElements
 * function.
 *
 * Finally, a list of draw elements can be extracted.
 */
class YSegment {
    /** A left border of a draw element.
     * The right border is the next TexBorder in the list.
     */
    class TexBorder {
        BorderFunctionBase* bfp;
        std::vector<int> texid;

    public:
        TexBorder(BorderFunctionBase* bf, int tex) throw () : bfp(bf), texid(1, tex) { }
        TexBorder(BorderFunctionBase* bf, std::vector<int> tex) throw () : bfp(bf), texid(tex) { }
        BorderFunctionBase* getFn() const throw () { return bfp; }
        int getBaseTex() const throw () { return texid.front(); }
        const std::vector<int>& getAllTextures() const throw () { return texid; }
        void setTex(int tex) throw () { texid.clear(); texid.push_back(tex); }
        void addTex(int tex) throw () { texid.push_back(tex); }
    };

    typedef std::vector<BorderFunctionBase*> BorderListT;
    typedef std::list<TexBorder> TexBorderListT;

    double y0, y1;
    BorderListT build;
    TexBorderListT final;

    /** Positional comparison for border functions within the segment.
     *
     * The functions can't fully intersect within the segment but they can get
     * very close. Floating point inaccuracies can make them equal, so 3
     * different y are used if necessary. Extreme values of y are avoided
     * because the functions can meet at edges of the segment.
     */
    class BorderCompare {
        double my1, my2, my3;

    public:
        BorderCompare(double y0, double y1) throw () : my1(y0*.8 + y1*.2), my2(y0*.5 + y1*.5), my3(y0*.2 + y1*.8) { nAssert(fabs(my3-my1)>SPLIT_TRESHOLD*.1); }
        bool operator()(const BorderFunctionBase* o1, const BorderFunctionBase* o2) throw ();
    };

public:
    YSegment(double y0_, double y1_) throw () : y0(y0_), y1(y1_) { nAssert(y1 >= y0); }
    double getY0() const throw () { return y0; }
    double getY1() const throw () { return y1; }
    double height() const throw () { nAssert(y1 >= y0); return y1 - y0; }
    void setY0(double y) throw () { nAssert(y >= y0); nAssert(y1 >= y); y0 = y; } ///< Shrink (only) the segment from the top.
    void setY1(double y) throw () { nAssert(y <= y1); nAssert(y >= y0); y1 = y; } ///< Shrink (only) the segment from the bottom.
    void add(BorderFunctionBase* border) throw () { build.push_back(border); } ///< Add a border to the object being built. Must not intersect other borders, final or being built. Ownership not transferred.
    /** Find intersection between @a bfn and an existing border.
     *
     * Find the first (lowest) y-coordinate within the segment, where @a bfn
     * intersects an existing border in the FINAL list.
     * Extreme values of y (that might, would the math be exact, actually be at
     * the extreme coordinate or even outside the segment) are ignored.
     * Returns false if no such intersections exist, else sets @a *splity.
     */
    bool getFirstIntersection(BorderFunctionBase* bfn, double* splity) throw ();
    YSegment split(double midy) throw (); ///< Split the segment somewhere in the middle. *this is shrunk and a new YSegment representing the bottom side is returned.
    void sort() throw (); ///< Sort the build list borders in increasing x-order.
    void simplify() throw (); ///< Remove duplicate borders from build list (assumed sorted).
    void moveElements(int texid) throw (); ///< Move the borders of the built object to the final list (must be empty).
    /** Move the borders of the built object to the final list, over the old elements.
     * If @a overlay is set, @a texid is added to the textures already in the area,
     * else old (parts of) elements falling below the new object are hidden (removed).
     */
    void moveElementsWithOverlap(int texid, bool overlay) throw ();
    void extractDrawElements(std::list<DrawElement>& dst) const throw (); ///< Turn the final element border list into actual DrawElements.
    void debug(bool verbose = false) const throw ();
};

typedef std::list<YSegment> SegListT;

/** Span texturizer locked to a single texture (solid).
 *
 * Provides the span texturizer interface (for template use) for drawing a
 * single solid color texture. "Inlines" the operations of SolidPixelSource
 * without concerns for alpha. (lesser alpha only happens with multiple textures)
 */
class SolidTexturizer { // includes inlined the same operations as SolidPixelSource
    Texturizer& host;
    int color;

    void putPixI(int alpha) throw () { host.putPix(color, alpha); }

public:
    SolidTexturizer(Texturizer& host_, const SolidTexdata& td) throw () : host(host_), color(td.color) { nAssert(td.alpha == 256); }

    void setLine(int y) throw () { host.setLine(y); } ///< Set line where the next pixels will be drawn. Invalidates x-coordinate.
    void nextLine() throw () { host.nextLine(); } ///< Increase target line by one. Invalidates x-coordinate.
    void putSpan(int x0, int x1, double alpha) throw (); ///< Draw the range [x0,x1[ with given fractional alpha (in [0,1]).
    void startPixSpan(int x) throw () { host.startPixSpan(x); } ///< Set x-coordinate where the next individual pixels will be drawn. Semi-expensive.
    void putPix(double alpha) throw (); ///< Draw at the current position with given fractional alpha, and increase x by one.
};

/// Span texturizer locked to a single texture (bitmap). See SolidTexturizer for method documentation.
class TextureTexturizer { // includes inlined the same operations as TexturePixelSource
    Texturizer& host;
    BITMAP* tex;
    int tx0, ty0;
    int tx, ty; // active pixel in tex

    void putPixI(int alpha) throw ();

public:
    TextureTexturizer(Texturizer& host_, const TextureTexdata& td) throw () : host(host_), tex(td.image), tx0(td.x0), ty0(td.y0) { nAssert(td.alpha == 256); }

    void setLine(int y) throw ();
    void nextLine() throw ();
    void putSpan(int x0, int x1, double alpha) throw ();
    void startPixSpan(int x) throw ();
    void putPix(double alpha) throw ();
};

/** Generic span texturizer for any number of textures.
 *
 * Provides the span texturizer interface (for template use) for drawing a
 * stack of textures provided by PixelSources.
 *
 * See SolidTexturizer for method documentation.
 */
class MultiLayerTexturizer {
    Texturizer& host;
    std::vector<PixelSource*> layers;

public:
    MultiLayerTexturizer(Texturizer& host_, int layersReserve = 2) throw () : host(host_) { layers.reserve(layersReserve); }
    /** Initialization: add a texture layer to the stack.
     * Must be called at least once before trying to draw. Ownership not transferred.
     */
    void addLayer(PixelSource* layerSource) throw () { layers.push_back(layerSource); }

    void setLine(int y) throw ();
    void nextLine() throw ();
    void putSpan(int x0, int x1, double alpha) throw ();
    void startPixSpan(int x) throw ();
    void putPix(double alpha) throw ();
};

#endif
