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

// // // // public interface

/** Base class for data sources used as textures.
 *
 * Provides color and alpha values for individual pixels on a span basis keeping
 * track of the position itself.
 *
 * Positions given are target pixels.
 */
class PixelSource : private NoCopying {
public:
    virtual ~PixelSource() throw () { }
    virtual void setLine(int y) throw () = 0; ///< Set line where the next pixels will be drawn. Invalidates x-coordinate.
    virtual void nextLine() throw () = 0; ///< Increase target line by one. Invalidates x-coordinate.
    virtual void startPixSpan(int x) throw () = 0; ///< Set x-coordinate where the next span will start (next pixel drawn).
    virtual std::pair<int, int> getPixel() throw () = 0; ///< Get (color, alpha) to draw at the current position and increase x by one.
};

/// Source for a constant color and alpha value everywhere.
class SolidPixelSource : public PixelSource {
    int color, alpha;
    friend class SolidTexturizer;

public:
    SolidPixelSource(int color_,
                     int alpha_ = 256 ///< Translucency, range [0,256] where 256 is opaque and 0 invisible (useless).
                     ) throw () : color(color_), alpha(alpha_) { }
    void setLine(int) throw () { }
    void nextLine() throw () { }
    void startPixSpan(int) throw () { }
    std::pair<int, int> getPixel() throw ();
};

/// Source for pixels read from a bitmap tiled on screen.
class TexturePixelSource : public PixelSource {
    BITMAP* tex; // not const because fed to Allegro
    int alpha;
    int tx0, ty0;
    int tx, ty; ///< active pixel in tex
    friend class TextureTexturizer;

public:
    TexturePixelSource(BITMAP* texture, ///< Source bitmap. If alpha == 256 (much faster), the width and height must be (possibly different) powers of two. This is not checked!
                       int x0 = 0,
                       int y0 = 0, /**< Target buffer coordinates for the top left corner of a conceptual "master copy" of @a texture.
                                    *
                                    * Other copies are placed elsewhere starting immediately to the right and down
                                    * from the master copy, tiling the screen.
                                    *
                                    * A texture can not be used in areas where x < tex.x0 || y < tex.y0. This can
                                    * be circumvented by selecting x0 and y0 < 0.
                                    */
                       int alpha_ = 256 ///< Translucency, range [0,256] where 256 is opaque and 0 invisible (useless).
                       ) throw () : tex(texture), alpha(alpha_), tx0(x0), ty0(y0) { }

    void setLine(int y) throw ();
    void nextLine() throw ();
    void startPixSpan(int x) throw ();
    std::pair<int, int> getPixel() throw ();
};

/** Source for flag marker pixels.
 * Produces a variably translucent circular pattern stretched to the given radius.
 */
class FlagmarkerPixelSource : public PixelSource {
    int color;
    double markRadius;
    double intensityMul;
    double cx, cy;
    double dy, dy2;
    double dx;

public:
    FlagmarkerPixelSource(int color_, double cx_, double cy_, double r) throw ();

    void setLine(int y) throw ();
    void nextLine() throw ();
    void startPixSpan(int x) throw ();
    std::pair<int, int> getPixel() throw ();
};

/** Helper for creating a table of textures and freeing it.
 * Can contain both pointers with ownership and those without.
 */
class TextureTable {
    std::vector<PixelSource*> table, owned;

public:
    ~TextureTable() { for (int i = 0; i < (int)owned.size(); ++i) delete owned[i]; }

    int nextIndex() const { return table.size(); } ///< Get the texture index of the next texture to be added. First is 0, last added is nextIndex() - 1, etc.
    void addOwned(PixelSource* tex) { table.push_back(tex); owned.push_back(tex); } ///< Add a texture and transfer ownership. Calling addOwned(new ...(...)) is common.
    void addExternal(PixelSource* tex) { table.push_back(tex); } ///< Add a texture without transferring ownership.
    /** Store a texture without adding to table, but transfer ownership.
     * @return Index that can be passed to addStored to refer to this texture. Sequential calls to storeOwned (only) return sequential indices.
     */
    int storeOwned(PixelSource* tex) { owned.push_back(tex); return owned.size() - 1; }
    void addStored(int index) { table.push_back(owned[index]); } ///< Add a texture previously stored with storeOwned.

    const std::vector<PixelSource*>& read() const { return table; }
};

/** Full scene antialiasing.
 *
 * Collects drawn objects, scales and clips them to fit the viewport (or subset
 * thereof), and finally renders them all at once.
 *
 * Texture numbers given to add methods are indices and must correspond to
 * @a textures given to render.
 */
class SceneAntialiaser : private NoCopying {
public:
    SceneAntialiaser() throw () { }
    ~SceneAntialiaser() throw ();

    /** Set scaling applied to future adds and setClipping.
     *
     * All coordinates of added objects will be first multiplied by scale, then
     * translated so that (0,0) is drawn to (x0,y0).
     *
     * Must be called at least once before the first add or setClipping. The
     * same scaling applies to all adds and calls to setClipping until
     * setScaling is called again.
     */
    void setScaling(double x0_ = 0, double y0_ = 0, double scale_ = 1.) throw ();

    void addRectangle(double x1, double y1, double x2, double y2, int texture, bool overlay = false) throw (); ///< Add given rectangle. Overlay must be set iff the texture is not opaque (alpha!=256).
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

    void addWallClipped(const WallBase* wall, int texture) throw (); ///< Add and clip given wall. Texture must be opaque (alpha=256).

    /** Render all objects.
     *
     * Every fraction of a pixel must fit the buffer after the scaling and
     * clipping that has been applied. Whole pixels with no content are not
     * drawn, partial pixels are completed with black.
     *
     * @param buffer    Target bitmap large enough to contain all added objects.
     * @param textures  A pixel source for every texture id previously passed to add methods.
     *                  If not fully opaque, overlay must have been set when adding.
     */
    void render(BITMAP* buffer, const std::vector<PixelSource*>& textures) const throw ();

private:
    std::vector<WallBorderSegment>& addEmptyObject(int texture, bool overlay = false) throw ();
    void makeRectangleBorders(std::vector<WallBorderSegment>& borders, double x1, double y1, double x2, double y2) throw ();
    void makeBorders(std::vector<WallBorderSegment>& borders, const TriWall& wall) throw ();
    void makeBorders(std::vector<WallBorderSegment>& borders, const CircWall& wall) throw ();

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
