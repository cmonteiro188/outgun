#ifndef ANTALIAS_H_INC
#define ANTALIAS_H_INC

#include <vector>
#include <list>

// // // // internal definitions

class PartialPixelSegment {
public:
	enum { scale = 23 };	// scale alphas by 1 << scale ((1 << scale) / 2 equals half transparent)

	PartialPixelSegment(int x0) : startx(x0) { }
	int x0() const { return startx; }
	int len() const { return pixels.size(); }
	void add(int index, int color, int alpha) { pixels[index].add(color, alpha); }//double alpha) { pixels[index].add(color, alpha); }
	void extend(int color, int alpha) { pixels.push_back(PartPix(color, alpha)); }//double alpha) { pixels.push_back(PartPix(color, alpha)); }
	void draw(BITMAP* buf, int y) const;

private:
/* original unscaled floating point version (slightly slower)
	class PartPix {
		float r, g, b, alphaTotal;

	public:
		PartPix(int color, double alpha) :
			r(getr(color) * alpha),
			g(getg(color) * alpha),
			b(getb(color) * alpha),
			alphaTotal(alpha) { }
		void add(int color, double alpha) {
			r += getr(color) * alpha;
			g += getg(color) * alpha;
			b += getb(color) * alpha;
			alphaTotal += alpha;
		}
		bool draw() const { return alphaTotal >= .001; }
		int color() const {
			// this ensures that only whole pixels are written; enable if that's true:	numAssert(alphaTotal >= .999, int(alphaTotal * 10000.));
			numAssert(alphaTotal <= 1.001, int(alphaTotal*10000));
			return makecol(static_cast<int>(r + .5), static_cast<int>(g + .5), static_cast<int>(b + .5));
		}
		int flexColor() const {	// allows more than 1 alphaTotal, with a cut on high intensities
			return makecol(min(255, static_cast<int>(r + .5)), min(255, static_cast<int>(g + .5)), min(255, static_cast<int>(b + .5)));
		}
	};
*/
	class PartPix {
		int r, g, b, alphaTotal;
		enum { scale = PartialPixelSegment::scale, scaleVal = 1 << PartialPixelSegment::scale };

	public:
		PartPix(int color, int alpha) :
			r(getr(color) * alpha),
			g(getg(color) * alpha),
			b(getb(color) * alpha),
			alphaTotal(alpha) { }
		void add(int color, int alpha) {
			r += getr(color) * alpha;
			g += getg(color) * alpha;
			b += getb(color) * alpha;
			alphaTotal += alpha;
		}
		bool draw() const { return alphaTotal >= scaleVal / 100; }
		int color() const {
			// this ensures that only whole pixels are written; enable if that's true:	numAssert(alphaTotal >= .999, int(alphaTotal * 10000.));
			numAssert(alphaTotal <= scaleVal * 1.001, alphaTotal);
			return makecol((r + scaleVal / 2) >> scale, (g + scaleVal / 2) >> scale, (b + scaleVal / 2) >> scale);
		}
		int flexColor() const {	// allows more than 1 alphaTotal, with a cut on high intensities
			return makecol(std::min(255, (r + scaleVal / 2) >> scale), std::min(255, (g + scaleVal / 2) >> scale), std::min(255, (b + scaleVal / 2) >> scale));
		}
	};

	int startx;
	std::vector<PartPix> pixels;
};

class RectWall;
class TriWall;
class CircWall;
class BorderFunctionBase;
class LineFunction;

struct WallBorderSegment {
	BorderFunctionBase* fn;
	double y0, y1;
	WallBorderSegment() { }
	WallBorderSegment(BorderFunctionBase* fn_, double y0_, double y1_) : fn(fn_), y0(y0_), y1(y1_) { }
};

struct ObjectSource {
	int texid;
	std::vector<WallBorderSegment> borders;
};

// // // // public interface

class PlainTexTexturizer {
public:
	PlainTexTexturizer(BITMAP* buffer, int x0, int y0, std::vector<BITMAP*>& textures) : buf(buffer), bx0(x0), by0(y0), texTab(textures), partials(buffer->h) { }
	void setTex(int texid) { numAssert2(texid >= 0 && texid < (int)texTab.size(), texid, texTab.size()); tex = texTab[texid]; }

	void setLine(int y);
	void nextLine();

	void putSpan(int x0, int x1, double alpha);	// fills the range [x0,x1[
	void startPixSpan(int x);
	void putPix(double alpha);	// draws at current x coord and increases it

	void finalize();	// draws all buffered pixels (use only when no longer drawing)

private:
	void doPutPix(int iAlpha);

	BITMAP* buf;
	int bx0, by0;	// buffer pixel offset
	int bx, by;	// active pixel in buf
	BITMAP* tex;
	int tx, ty;	// active pixel in tex
	std::vector<BITMAP*>& texTab;
	std::vector< std::list<PartialPixelSegment> > partials;
	PartialPixelSegment* partSpan;
	int spanIndex;	// index in partSpan
	int spanEnd;	// when we must move to the next segment to continue adding pixels
};

class PlainColorTexturizer {
public:
	PlainColorTexturizer(BITMAP* buffer, int x0, int y0, std::vector<int>& colors) : buf(buffer), bx0(x0), by0(y0), colTab(colors), partials(buffer->h) { }
	void setTex(int texid) { nAssert(texid >= 0 && texid < (int)colTab.size()); color = colTab[texid]; }

	void setLine(int y);
	void nextLine();

	void putSpan(int x0, int x1, double alpha);	// fills the range [x0,x1[
	void startPixSpan(int x);
	void putPix(double alpha);	// draws at current x coord and increases it

	void finalize();	// draws all buffered pixels (use only when everything has been drawn)

private:
	void doPutPix(int iAlpha);

	BITMAP* buf;
	int bx0, by0;	// buffer pixel offset
	int bx, by;	// active pixel in buf
	int color;
	std::vector<int>& colTab;
	std::vector< std::list<PartialPixelSegment> > partials;
	PartialPixelSegment* partSpan;
	int spanIndex;	// index in partSpan
	int spanEnd;	// when we must move to the next segment to continue adding pixels
};

class SceneAntialiaser {
public:
	SceneAntialiaser() { }
	~SceneAntialiaser();

	void setScaling(float x0_ = 0, float y0_ = 0, float scale_ = 1.);	// call before add*

	void addRectangle(float x1, float y1, float x2, float y2, int texture);
	void addRectWall(const RectWall& wall, int texture);
	void addTriWall (const  TriWall& wall, int texture);
	void addCircWall(const CircWall& wall, int texture);

	void setClipping(float x1, float y1, float x2, float y2);	// setClipping applies scaling to the coords
	void addRectWallClipped(const RectWall& wall, int texture);
	void addTriWallClipped (const  TriWall& wall, int texture);
	void addCircWallClipped(const CircWall& wall, int texture);
	void clipAll() { clip(0); }	// clips all added objects to the current clipping rectangle

	void render(PlainTexTexturizer& tex) const;
	void render(PlainColorTexturizer& tex) const;

private:
	// deny copying
	SceneAntialiaser(const SceneAntialiaser&) { nAssert(0); }
	SceneAntialiaser& operator=(const SceneAntialiaser&) { nAssert(0); return *this; }

	void createClipFns();
	void clip(int i0);
	template<class Texturizer> void renderTemplate(Texturizer& tex) const;

	std::vector<BorderFunctionBase*> bfns;
	std::vector<ObjectSource> objects;
	LineFunction* clipLeft, * clipRight;
	bool clipFunctionsValid;
	float x0, y0, scale;
	float clipx1, clipy1, clipx2, clipy2;
};

#endif	// ANTALIAS_H_INC
