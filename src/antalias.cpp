#include <vector>
#include <list>
#include <algorithm>
#include <cmath>
#include "nassert.h"
#include "world.h"

#include "antalias.h"

using std::vector;
using std::list;
using std::min;
using std::max;
using std::swap;

//#define DEBUG_SPLIT
//#define DEBUG_OVERLAP
//#define CERR_DEBUG
//#include <iostream>
//using std::cerr;

// INTERSECTION_TRESHOLD	is how much inside a segment an intersection is allowed (must allow for rounding errors)
// SPLIT_TRESHOLD			is how much y-borders are allowed to deviate (must exceed the precision of the given y coords)
// FINAL_EXTREMECUT		    is how much of each element is removed y-wise to protect from exceeding function range
// JOIN_TRESHOLD			is how much gap is allowed when joining two adjacent objects to one
// all tresholds' unit is a pixel
// these settings currently tune the module to work with values of y up to approximately +-1e8
// they have to be relaxed in order to gain more range
const double INTERSECTION_TRESHOLD = .00000001;	// roughly 2^-27
const double SPLIT_TRESHOLD        = .0000001;
const double FINAL_EXTREMECUT      = .000001;
const double JOIN_TRESHOLD         = .001;

struct ChangePoints {
	enum Side { S_Left, S_Right };	// tells on which side of the given line the function currently is

	Side startSide;
	double points[3];	// 3 is the logical maximum of points used; increase if more are needed by complex functions
						// convention: has one extra item at the end that is big; set at 1e99
	// the C array is an optimization: using a vector would significantly slow things down and wouldn't help at all
};

class BorderFunctionBase {
protected:
	BorderFunctionBase() { }

public:
	virtual ~BorderFunctionBase() { }
	virtual double operator()(double y) const = 0;
	virtual ChangePoints getChangePoints(double x) const = 0;
	virtual double spanLeftSideIntegral(double x0, double y0, double y1) const = 0;
	virtual bool centerExtremes() const = 0;	// must return true if the extreme X coordinate of some y-interval is not at either of the borders
	virtual double extremeY() const = 0;	// if centerExtremes() is true, must return the Y coordinate at which the extreme X is
	virtual double extremeX() const = 0;	// if centerExtremes() is true, must return the extreme X coordinate
	virtual bool operator==(const BorderFunctionBase& o) = 0;
	virtual void debug() const { }
};

class LineFunction;

class CurveFunction : public BorderFunctionBase {
	double cx, cy, r, r2;
	double sideMul;

	friend double getIntersection(LineFunction* f1, CurveFunction* f2, double miny);
	friend double getIntersection(CurveFunction* f1, CurveFunction* f2, double miny);

public:
	CurveFunction(double cx_, double cy_, double r_, bool rightSide) : cx(cx_), cy(cy_), r(r_), r2(r_*r_), sideMul(rightSide?+1:-1) { }
	double operator()(double y) const;
	ChangePoints getChangePoints(double x) const;
	double spanLeftSideIntegral(double x0, double y0, double y1) const;
	bool centerExtremes() const { return true; }
	double extremeY() const { return cy; }
	double extremeX() const { return cx + sideMul * r; }
	bool operator==(const BorderFunctionBase& o);
	void debug() const;
};

class LineFunction : public BorderFunctionBase {
	double px1, py1, px2, py2;
	double ratio;

	friend double getIntersection(LineFunction* f1, LineFunction* f2);
	friend double getIntersection(LineFunction* f1, CurveFunction* f2, double miny);

public:
	LineFunction(double x1, double y1, double x2, double y2) : px1(x1), py1(y1), px2(x2), py2(y2), ratio((x2 - x1)/(y2 - y1)) { }
	double operator()(double y) const;
	ChangePoints getChangePoints(double x) const;
	double spanLeftSideIntegral(double x0, double y0, double y1) const;
	bool centerExtremes() const { return false; }
	double extremeY() const { nAssert(0); return 0; }
	double extremeX() const { nAssert(0); return 0; }
	bool operator==(const BorderFunctionBase& o);
	void debug() const;
};

class DrawElement {
	BorderFunctionBase* fLeft, * fRight;	// these are not owned: the object pointed to must live as long as this DrawElement is used, and must be manually deleted
	double y0, y1;
	int texid;

public:
	DrawElement(BorderFunctionBase* flp, BorderFunctionBase* frp, double y0_, double y1_, int tex);
	void extendDown(double y1_) { nAssert(y1_ > y1); y1 = y1_; }
	const BorderFunctionBase& getLeft() const { return *fLeft; }
	const BorderFunctionBase& getRight() const { return *fRight; }
	double getY0() const { return y0; }
	double getY1() const { return y1; }
	int getTex() const { return texid; }
	bool isJoinable(const DrawElement& o) const;
};

class YSegment {
	class TexBorder {
		BorderFunctionBase* bfp;
		int texid;

	public:
		TexBorder(BorderFunctionBase* bf, int tex) : bfp(bf), texid(tex) { }
		BorderFunctionBase* getFn() const { return bfp; }
		int getTex() const { return texid; }
		void setTex(int tex) { texid = tex; }
	};

	typedef std::vector<BorderFunctionBase*> BorderListT;
	typedef std::list<TexBorder> TexBorderListT;

	double y0, y1;
	BorderListT build;
	TexBorderListT final;

	class BorderCompare {
		double my1, my2, my3;

	public:
		BorderCompare(double y0, double y1) : my1(y0*.8 + y1*.2), my2(y0*.5 + y1*.5), my3(y0*.2 + y1*.8) { nAssert(fabs(my3-my1)>SPLIT_TRESHOLD*.1); }
		bool operator()(const BorderFunctionBase* o1, const BorderFunctionBase* o2);
	};

public:
	YSegment(double y0_, double y1_) : y0(y0_), y1(y1_) { }
	double getY0() const { return y0; }
	double getY1() const { return y1; }
	double width() const { return y1 - y0; }
	void setY0(double y) { nAssert(y >= y0); y0 = y; }	// only allow shrinking the segment
	void setY1(double y) { nAssert(y <= y1); y1 = y; }
	void add(BorderFunctionBase* border) { build.push_back(border); }	// ownership of the pointed object is not transferred!
	bool getFirstIntersection(BorderFunctionBase* bfn, double* splity);
	YSegment split(double midy);	// the segment starting with midy is the returned one
	void sort();	// sorts the build list borders in increasing x-order
	void simplify();	// removes double borders from build list (assuming it's sorted)
	void moveElements(int texid);	// moves all borders from build list to final list (use only when the final list is empty)
	void moveElementsWithOverlap(int texid);	// moves all borders from build list to final list overlapping the old walls
	void extractDrawElements(vector<DrawElement>& dst) const;
	void debug(bool verbose = false) const;

	YSegment(const YSegment& o) { *this = o; }
	YSegment& operator=(const YSegment& o) { y0 = o.y0; y1 = o.y1; build = o.build; final = o.final; return *this; }
};

typedef std::list<YSegment> SegListT;

// // // //

double CurveFunction::operator()(double y) const {
	numAssert3(r2 - (y - cy)*(y - cy) >= 0, int(r2*1000.), int(y*1000.), int(cy*1000.));
	return cx + sqrt(r2 - (y - cy)*(y - cy)) * sideMul;
}

ChangePoints CurveFunction::getChangePoints(double x) const {
	ChangePoints ret;
	if ((x - cx) * sideMul < 0) {	// x is on the undefined side
		ret.startSide = sideMul>0 ? ChangePoints::S_Right : ChangePoints::S_Left;
		ret.points[0] = 1e99;
	}
	else {
		ret.startSide = sideMul>0 ? ChangePoints::S_Left : ChangePoints::S_Right;
		// change points are where x*x+y*y=r*r
		double rt = r2 - (x - cx)*(x - cx);
		if (rt > 0.) {
			rt = sqrt(rt);
			ret.points[0] = cy - rt;
			ret.points[1] = cy + rt;
			ret.points[2] = 1e99;
		}
		else
			ret.points[0] = 1e99;
	}
	return ret;
}

double CurveFunction::spanLeftSideIntegral(double x0, double y0, double y1) const {
	// this function computes the integral from y0 to y1 of (x(y) - x0)dy ; derive the expression yourself ;)
	y0 -= cy; y1 -= cy;
	numAssert3(fabs(y0) <= r, int(y0), int(r), int((y0-r)*1e20));
	numAssert3(fabs(y1) <= r, int(y1), int(r), int((y1-r)*1e20));
	double val = (y1 - y0)*(cx - x0) + .5*sideMul*( y1*sqrt(r2 - y1*y1) - y0*sqrt(r2 - y0*y0) + r2*(asin(y1/r) - asin(y0/r)) );
	numAssert(val >= -.00001 && val <= 1.00001, int(val*10000.));
	return val;
}

bool CurveFunction::operator==(const BorderFunctionBase& o) {
	const CurveFunction* cfp = dynamic_cast<const CurveFunction*>(&o);
	if (!cfp)
		return false;
	return cx == cfp->cx && cy == cfp->cy && r == cfp->r && sideMul == cfp->sideMul;
}

void CurveFunction::debug() const {
	#ifdef CERR_DEBUG
	cerr.precision(12);
	cerr << "Curve @" << cx << ',' << cy << " R=" << r << (sideMul>0 ? " right\n" : " left\n");
	#endif
}

double LineFunction::operator()(double y) const {
	if (py1 == py2) {	// in this case, ratio is invalid
		nAssert(y == py1);
		return px1;
	}
	return px1 + (y - py1) * ratio;
}

ChangePoints LineFunction::getChangePoints(double x) const {
	ChangePoints ret;
	// change point is where x = px1 + (px2 - px1)*(y - py1)/(py2 - py1) ; y = py1 + (py2 - py1)*(x - px1)/(px2 - px1)
	if (py1 == py2) {	// in this case, ratio is invalid
		ret.startSide = ChangePoints::S_Left;	// arbitrary
		ret.points[0] = 1e99;
	}
	else if (ratio == 0) {
		ret.startSide = px1 < x ? ChangePoints::S_Left : ChangePoints::S_Right;
		ret.points[0] = 1e99;
	}
	else {
		ret.startSide = ratio > 0 ? ChangePoints::S_Left : ChangePoints::S_Right;
		ret.points[0] = py1 + (x - px1) / ratio;
		ret.points[1] = 1e99;
	}
	return ret;
}

double LineFunction::spanLeftSideIntegral(double x0, double y0, double y1) const {
	// this function computes the integral from y0 to y1 of (x(y) - x0)dy ; derive the expression yourself ;)
	if (py1 == py2) {	// in this case, ratio is invalid
		nAssert(y0 == y1 && y0 == py1);
		return 0;
	}
	double val = (y1 - y0)*(px1 - x0 - py1 * ratio) + .5*(y1*y1 - y0*y0)*ratio;
	numAssert(val >= -.0001 && val <= 1.00001, int(val*1000000.));
	return val;
}

bool LineFunction::operator==(const BorderFunctionBase& o) {
	if (!dynamic_cast<const LineFunction*>(&o))
		return false;
	if (fabs(px1 - o(py1)) > .0001 || fabs(px2 - o(py2)) > .0001)
		return false;
	return true;
}

void LineFunction::debug() const {
	#ifdef CERR_DEBUG
	cerr.precision(12);
	cerr << "Line (" << px1 << ',' << py1 << ") - (" << px2 << ',' << py2 << ")\n";
	#endif
}

// swap used by pixelLeftSideIntegral (and elsewhere) ; ideally s = !s;
void swap(ChangePoints::Side& s) { s = (s == ChangePoints::S_Left) ? ChangePoints::S_Right : ChangePoints::S_Left; }

double pixelLeftSideIntegral(double x0, double y0, double y1, const BorderFunctionBase& fn) {
	const ChangePoints lc = fn.getChangePoints(x0), rc = fn.getChangePoints(x0+1.);
	ChangePoints::Side ls = lc.startSide, rs = rc.startSide;
	const double* lcpi = lc.points, * rcpi = rc.points;
	double totalPixel = 0;
	double y = y0;
	// update border markers to the status around the current y
	while (*lcpi <= y) { swap(ls); ++lcpi; }
	while (*rcpi <= y) { swap(rs); ++rcpi; }
	for (;;) {
		if (ls == ChangePoints::S_Left) {	// out on the left side (until *lcpi)
			if (*lcpi >= y1)
				return totalPixel;
			y = *lcpi++; swap(ls);
			while (*rcpi <= y) { swap(rs); ++rcpi; }
		}
		else if (rs == ChangePoints::S_Right) {	// out on the right side (until *rcpi)
			if (*rcpi >= y1)
				return totalPixel + 1. * (y1 - y);
			totalPixel += 1. * (*rcpi - y);
			y = *rcpi++; swap(rs);
			while (*lcpi <= y) { swap(ls); ++lcpi; }
		}
		else if (*lcpi < *rcpi) {	// within the clipping region until *lcpi
			if (*lcpi >= y1)
				return totalPixel + fn.spanLeftSideIntegral(x0, y, y1);
			totalPixel += fn.spanLeftSideIntegral(x0, y, *lcpi);
			y = *lcpi++; swap(ls);
		}
		else {	// within the clipping region until *rcpi
			if (*rcpi >= y1)
				return totalPixel + fn.spanLeftSideIntegral(x0, y, y1);
			totalPixel += fn.spanLeftSideIntegral(x0, y, *rcpi);
			y = *rcpi++; swap(rs);
		}
		numAssert(totalPixel >= -.0001 && totalPixel <= 1.0001, int(totalPixel*100000.));
	}
}

void PartialPixelSegment::draw(BITMAP* buf, int y) const {
	for (size_t i = 0; i < pixels.size(); ++i)
		if (pixels[i].draw())
			putpixel(buf, startx + i, y, pixels[i].color());
}

void PlainTexTexturizer::setLine(int y) {
	nAssert(y >= 0 && y < buf->h);
	by = by0 + y;
	ty = y % tex->h;
}

void PlainTexTexturizer::nextLine() {
	++by;
	nAssert(by < buf->h);
	if (++ty == tex->h)
		ty = 0;
}

void PlainTexTexturizer::putSpan(int x0, int x1, double alpha) {	// fills the range [x0,x1[
	nAssert(x0 < x1);	// empty spans aren't tolerated
	if (alpha >= .999) {
		drawing_mode(DRAW_MODE_COPY_PATTERN, tex, 0, 0);
		hline(buf, x0 + bx0, by, x1 + bx0, 0);
		solid_mode();
	}
	else {
		startPixSpan(x0);
		int iAlpha = static_cast<int>(ldexp(alpha, PartialPixelSegment::scale));
		for (int x = x0; x < x1; ++x)
			doPutPix(iAlpha);
	}
}

void PlainTexTexturizer::startPixSpan(int x) {
	bx = bx0 + x;
	tx = x % tex->w;
	list<PartialPixelSegment>& row = partials[by];
	for (list<PartialPixelSegment>::iterator si = row.begin();; ++si) {
		if (si == row.end()) {
			partSpan = &(*row.insert(row.end(), PartialPixelSegment(x)));
			spanEnd = INT_MAX;
			spanIndex = 0;
			break;
		}
		spanIndex = x - si->x0();
		if (spanIndex < 0) {
			int nextStart = si->x0();
			partSpan = &(*row.insert(si, PartialPixelSegment(x)));	// keep them sorted
			spanEnd = nextStart - partSpan->x0();
			spanIndex = 0;
			break;
		}
		if (spanIndex > si->len())
			continue;
		if (spanIndex == si->len()) {
			list<PartialPixelSegment>::iterator tsi = si;
			++tsi;
			if (tsi != row.end() && tsi->x0() == x)	// this means this pixel belongs to the next span; otherwise extend this one
				continue;
		}
		partSpan = &(*si);
		++si;
		if (si == row.end())
			spanEnd = INT_MAX;
		else
			spanEnd = si->x0() - partSpan->x0();
		// spanIndex already set
		break;
	}
}

void PlainTexTexturizer::putPix(double alpha) {
	doPutPix(static_cast<int>(ldexp(alpha, PartialPixelSegment::scale)));
}

void PlainTexTexturizer::doPutPix(int alpha) {
	int tpix = getpixel(tex, tx, ty);	//#opt: having the texture already in pixels might boost putPix about 25%
	if (spanIndex == partSpan->len()) {
		if (spanIndex < spanEnd)
			partSpan->extend(tpix, alpha);
		else {
			nAssert(spanIndex == spanEnd);
			startPixSpan(bx - bx0);
			nAssert(spanIndex == 0);
			nAssert(partSpan->len() > 0);
			partSpan->add(0, tpix, alpha);	// this opt. is the main reason empty spans aren't tolerated
		}
	}
	else {
		nAssert(spanIndex >= 0 && spanIndex < partSpan->len());
		partSpan->add(spanIndex, tpix, alpha);
	}
	++spanIndex;
	++bx;
	if (++tx == tex->w)
		tx = 0;
}

void PlainTexTexturizer::finalize() {
	for (int y = 0; y < buf->h; ++y) {
		list<PartialPixelSegment>& row = partials[y];
		for (list<PartialPixelSegment>::const_iterator si = row.begin(); si != row.end(); ++si)
			si->draw(buf, y);
		row.clear();
	}
}

void PlainColorTexturizer::setLine(int y) {
	nAssert(y >= 0 && y < buf->h);
	by = by0 + y;
}

void PlainColorTexturizer::nextLine() {
	++by;
	nAssert(by < buf->h);
}

void PlainColorTexturizer::putSpan(int x0, int x1, double alpha) {	// fills the range [x0,x1[
	nAssert(x0 < x1);	// empty spans aren't tolerated
	if (alpha >= .999)
		hline(buf, x0 + bx0, by, x1 + bx0, color);
	else {
		startPixSpan(x0);
		int iAlpha = static_cast<int>(ldexp(alpha, PartialPixelSegment::scale));
		for (int x = x0; x < x1; ++x)
			doPutPix(iAlpha);
	}
}

void PlainColorTexturizer::startPixSpan(int x) {
	bx = bx0 + x;
	list<PartialPixelSegment>& row = partials[by];
	for (list<PartialPixelSegment>::iterator si = row.begin();; ++si) {
		if (si == row.end()) {
			partSpan = &(*row.insert(row.end(), PartialPixelSegment(x)));
			spanEnd = INT_MAX;
			spanIndex = 0;
			break;
		}
		spanIndex = x - si->x0();
		if (spanIndex < 0) {
			int nextStart = si->x0();
			partSpan = &(*row.insert(si, PartialPixelSegment(x)));	// keep them sorted
			spanEnd = nextStart - partSpan->x0();
			spanIndex = 0;
			break;
		}
		if (spanIndex > si->len())
			continue;
		if (spanIndex == si->len()) {
			list<PartialPixelSegment>::iterator tsi = si;
			++tsi;
			if (tsi != row.end() && tsi->x0() == x)	// this means this pixel belongs to the next span; otherwise extend this one
				continue;
		}
		partSpan = &(*si);
		++si;
		if (si == row.end())
			spanEnd = INT_MAX;
		else
			spanEnd = si->x0() - partSpan->x0();
		// spanIndex already set
		break;
	}
}

void PlainColorTexturizer::putPix(double alpha) {
	doPutPix(static_cast<int>(ldexp(alpha, PartialPixelSegment::scale)));
}

void PlainColorTexturizer::doPutPix(int alpha) {
	if (spanIndex == partSpan->len()) {
		if (spanIndex < spanEnd)
			partSpan->extend(color, alpha);
		else {
			nAssert(spanIndex == spanEnd);
			startPixSpan(bx - bx0);
			nAssert(spanIndex == 0);
			nAssert(partSpan->len() > 0);
			partSpan->add(0, color, alpha);	// this opt. is the main reason empty spans aren't tolerated
		}
	}
	else {
		nAssert(spanIndex >= 0 && spanIndex < partSpan->len());
		partSpan->add(spanIndex, color, alpha);
	}
	++spanIndex;
	++bx;
}

void PlainColorTexturizer::finalize() {
	for (int y = 0; y < buf->h; ++y) {
		list<PartialPixelSegment>& row = partials[y];
		for (list<PartialPixelSegment>::const_iterator si = row.begin(); si != row.end(); ++si)
			si->draw(buf, y);
		row.clear();
	}
}

template<class Texturizer>
void renderLine(double y0, double y1, const BorderFunctionBase& fl, const BorderFunctionBase& fr, Texturizer& tex) {
	double minxl = min(fl(y0), fl(y1)), minxr = min(fr(y0), fr(y1));
	double maxxl = max(fl(y0), fl(y1)), maxxr = max(fr(y0), fr(y1));
	if (fl.centerExtremes() && fl.extremeY() > y0 && fl.extremeY() < y1) {
		minxl = min(minxl, fl.extremeX());
		maxxl = max(maxxl, fl.extremeX());
	}
	if (fr.centerExtremes() && fr.extremeY() > y0 && fr.extremeY() < y1) {
		minxr = min(minxr, fr.extremeX());
		maxxr = max(maxxr, fr.extremeX());
	}
	int l0 = static_cast<int>(floor(minxl)), l1 = static_cast<int>(ceil(maxxl));
	int r0 = static_cast<int>(floor(minxr)), r1 = static_cast<int>(ceil(maxxr));
	if (l1 < r0)
		tex.putSpan(l1, r0, 1. * (y1 - y0));
	else if (r0 < l1) {
		tex.startPixSpan(r0);
		for (int x = r0; x < l1; ++x)
			tex.putPix(pixelLeftSideIntegral(x, y0, y1, fr) - pixelLeftSideIntegral(x, y0, y1, fl));
		swap(r0, l1);
	}
	if (l0 < l1) {	// optimization: don't do the quite costly startPixSpan unnecessarily; also empty spans aren't tolerated
		tex.startPixSpan(l0);
		for (int lx = l0; lx < l1; ++lx)
			tex.putPix((y1 - y0) - pixelLeftSideIntegral(lx, y0, y1, fl));
	}
	if (r0 < r1) {	// optimization: see above
		tex.startPixSpan(r0);
		for (int rx = r0; rx < r1; ++rx)
			tex.putPix(pixelLeftSideIntegral(rx, y0, y1, fr));
	}
}

template<class Texturizer>
void renderBlock(double y0, double y1, const BorderFunctionBase& fl, const BorderFunctionBase& fr, Texturizer& tex) {
	tex.setLine(static_cast<int>(floor(y0)));
	if (ceil(y0) >= y1) {
		if (y1 > y0)
			renderLine(y0, y1, fl, fr, tex);
		return;
	}
	renderLine(y0, ceil(y0), fl, fr, tex);
	tex.nextLine();
	double y1f = floor(y1);
	for (y0 = ceil(y0); y0 < y1f; ++y0) {
		renderLine(y0, y0 + 1, fl, fr, tex);
		tex.nextLine();
	}
	renderLine(y1f, y1, fl, fr, tex);
}

DrawElement::DrawElement(BorderFunctionBase* flp, BorderFunctionBase* frp, double y0_, double y1_, int tex) :
	fLeft(flp),
	fRight(frp),
	y0(y0_),
	y1(y1_),
	texid(tex)
{
	nAssert(flp && frp);
}

bool DrawElement::isJoinable(const DrawElement& o) const {
	return o.y0 > y0 && fabs(y1 - o.y0) < JOIN_TRESHOLD && *fLeft == *o.fLeft && *fRight == *o.fRight && texid == o.texid;
}

bool YSegment::BorderCompare::operator()(const BorderFunctionBase* o1, const BorderFunctionBase* o2) {
	double c = (*o1)(my1) - (*o2)(my1);
	if (c != 0.)
		return c < 0;
	c = (*o2)(my2) - (*o2)(my2);
	if (c != 0.)
		return c < 0;
	return (*o1)(my3) < (*o2)(my3);
}

void YSegment::debug(bool verbose) const {
	#ifdef CERR_DEBUG
	cerr.precision(12);
	cerr << y0 << " -> " << y1 << ": " << build.size() << " (" << final.size() << ")\n";
	if (!verbose)
		return;
	for (BorderListT::const_iterator bi = build.begin(); bi != build.end(); ++bi) {
		cerr << "B   ";
		(*bi)->debug();
	}
	for (TexBorderListT::const_iterator fi = final.begin(); fi != final.end(); ++fi) {
		cerr << "T"; cerr.width(2); cerr << fi->getTex() << ' ';
		fi->getFn()->debug();
	}
	#else
	(void)verbose;
	#endif
}

double getIntersection(LineFunction* f1, LineFunction* f2) {
	// f1->px1 + (y - f1->py1) * f1->ratio = f2->px1 + (y - f2->py1) * f2->ratio
	if (f2->ratio == f1->ratio)
		return 1e99;	// no intersection
	return (f1->px1 - f2->px1 - f1->py1 * f1->ratio + f2->py1 * f2->ratio) / (f2->ratio - f1->ratio);
}

double getIntersection(LineFunction* f1, CurveFunction* f2, double miny) {
	// | f1->(px1,py1) + t * f1->(px2-px1,py2-py1) - f2->(cx,cy) |^2 = f2->r2
	// (x - f2->cx) * f2->sideMul > 0
	// t is calculated from the first equation just like in bounceFromPoint in world.cpp (code is copied from there)
	double dx = f2->cx  - f1->px1, dy = f2->cy  - f1->py1;
	double mx = f1->px2 - f1->px1, my = f1->py2 - f1->py1;
	double r2 = f2->r2;
	double m2 = mx*mx + my*my;
	double mdotd = mx*dx + my*dy;
	double d2 = dx*dx + dy*dy;
	double disc = mdotd*mdotd - m2*(d2-r2);
	if (disc <= 0)
		return 1e99;	// no intersection
	disc = sqrt(disc);
	double t = (mdotd - disc) / m2;	// smaller t
	double besty = 1e99;
	double y = f1->py1 + t * my;
	if (y >= miny) {
		double xside = t * mx - dx;
		if (xside * f2->sideMul > 0)	// same sign -> is on the 'active' side of the circle
			besty = y;
	}
	t = (mdotd + disc) / m2;	// larger t
	y = f1->py1 + t * my;
	if (y >= miny && y < besty) {
		double xside = t * mx - dx;
		if (xside * f2->sideMul > 0)	// same sign -> is on the 'active' side of the circle
			besty = y;
	}
	return besty;
}

double getIntersection(CurveFunction* f1, CurveFunction* f2, double miny) {
	// | (x,y) - f1->(cx,cy) |^2 = f1->r2
	// | (x,y) - f2->(cx,cy) |^2 = f2->r2
	// it's a tricky one; the solution is from http://mathforum.org/library/drmath/view/51836.html
	double dx = f2->cx - f1->cx;
	double dy = f2->cy - f1->cy;
	double sr2 = dx*dx + dy*dy;
	if (sr2 == 0)
		return 1e99;	// no intersection
	double t = .5 * (sr2 + f1->r2 - f2->r2);
	double xb = f1->cx + dx * t / sr2;
	double yb = f1->cy + dy * t / sr2;
	double srt = f1->r2 - t*t/sr2;
	if (srt <= 0)
		return 1e99;	// no intersection
	double sr = sqrt(sr2);
	double mul = sqrt(srt) / sr;
	// now the points are (xb +- dy*mul , yb -+ dx*mul)
	double y1 = yb - dx*mul;
	double besty = 1e99;
	if (y1 >= miny) {
		double x1 = xb + dy*mul;
		double dx1 = x1 - f1->cx, dx2 = x1 - f2->cx;
		#ifndef NDEBUG
		double dy1 = y1 - f1->cy, dy2 = y1 - f2->cy;
		nAssert(fabs(dx1*dx1 + dy1*dy1 - f1->r2) < .00001);
		nAssert(fabs(dx2*dx2 + dy2*dy2 - f2->r2) < .00001);
		#endif
		if (dx1 * f1->sideMul > 0 && dx2 * f2->sideMul > 0)
			besty = y1;
	}
	y1 = yb + dx*mul;
	if (y1 >= miny && y1 < besty) {
		double x1 = xb - dy*mul;
		double dx1 = x1 - f1->cx, dx2 = x1 - f2->cx;
		#ifndef NDEBUG
		double dy1 = y1 - f1->cy, dy2 = y1 - f2->cy;
		nAssert(fabs(dx1*dx1 + dy1*dy1 - f1->r2) < .00001);
		nAssert(fabs(dx2*dx2 + dy2*dy2 - f2->r2) < .00001);
		#endif
		if (dx1 * f1->sideMul > 0 && dx2 * f2->sideMul > 0)
			besty = y1;
	}
	return besty;
}

// getFirstIntersection gets the first y coordinate within the segment, with an intersection between bfn and a border in the final list
// extreme values of y (that might [would the math be exact] actually be at the extreme coordinate or even outside the segment) are ignored
bool YSegment::getFirstIntersection(BorderFunctionBase* bfn, double* splity) {
	*splity = 1e99;
	double miny = y0 + INTERSECTION_TRESHOLD;
	LineFunction* lfp1 = dynamic_cast<LineFunction*>(bfn);
	if (lfp1) {
		for (TexBorderListT::const_iterator bi = final.begin(); bi != final.end(); ++bi) {
			LineFunction* lfp2 = dynamic_cast<LineFunction*>(bi->getFn());
			if (lfp2) {
				double y = getIntersection(lfp1, lfp2);
				if (y > miny && y < *splity)
					*splity = y;
				continue;
			}
			CurveFunction* cfp2 = dynamic_cast<CurveFunction*>(bi->getFn());
			nAssert(cfp2);
			double y = getIntersection(lfp1, cfp2, miny);
			if (y < *splity)
				*splity = y;
			continue;
		}
	}
	else {
		CurveFunction* cfp1 = dynamic_cast<CurveFunction*>(bfn);
		nAssert(cfp1);
		for (TexBorderListT::const_iterator bi = final.begin(); bi != final.end(); ++bi) {
			LineFunction* lfp2 = dynamic_cast<LineFunction*>(bi->getFn());
			if (lfp2) {
				double y = getIntersection(lfp2, cfp1, miny);
				if (y > miny && y < *splity)
					*splity = y;
				continue;
			}
			CurveFunction* cfp2 = dynamic_cast<CurveFunction*>(bi->getFn());
			nAssert(cfp2);
			double y = getIntersection(cfp1, cfp2, miny);
			if (y < *splity)
				*splity = y;
			continue;
		}
	}
	return *splity < y1 - INTERSECTION_TRESHOLD;
}

YSegment YSegment::split(double midy) {
	nAssert(midy > y0 && midy < y1);
	YSegment ret(midy, y1);
	ret.build = build;
	ret.final = final;
	y1 = midy;
	return ret;
}

void YSegment::sort() {	// sorts the build list borders in increasing x-order
	nAssert((build.size() & 1) == 0);
	std::sort(build.begin(), build.end(), BorderCompare(y0, y1));
}

void YSegment::simplify() {	// removes double borders from build list (assuming it's sorted)
	nAssert((build.size() & 1) == 0);
	if (build.empty())
		return;
	BorderListT::iterator bi = build.begin();
	BorderListT::iterator bin = bi; ++bin;
	while (bin != build.end()) {
		if (*bi == *bin) {
			bi = build.erase(bi, ++bin);
			if (bi == build.end())
				break;
			bin = bi;
			++bin;
		}
		else {
			++bi;
			++bin;
		}
	}
}

void YSegment::moveElements(int texid) {	// moves all borders from build list to final list (use only when the final list is empty)
	nAssert(final.empty());
	for (BorderListT::iterator bi = build.begin(); bi != build.end(); ) {
		final.push_back(TexBorder(*bi, texid));
		bi = build.erase(bi);
		nAssert(bi != build.end());
		final.push_back(TexBorder(*bi, -1));
		bi = build.erase(bi);
	}
	build.clear();
}

/* how moveElementsWithOverlap works:
 *
 * the build list is assumed to be sorted, so that each pair of consequent borders forms a wall segment
 * (the final list already has tex information; nevertheless it must be sorted too; this function maintains the sort)
 * it is assumed that no walls intersect the walls to be added
 * 
 * taking one wall element (pair of borders) at a time from the build list, the final list is modified so the new wall overlaps old ones:
 * - search on while an old border is to the left left from the new left border
 * - add the new left border
 * - remove old borders while they are left from the new right border
 */
void YSegment::moveElementsWithOverlap(int texid) {	// moves all borders from build list to final list overlapping the old walls
	nAssert((build.size() & 1) == 0);
	BorderCompare bcmp(y0, y1);
	for (BorderListT::iterator sbi = build.begin(); sbi != build.end(); ) {
		#ifdef DEBUG_OVERLAP
		debug(true);
		cerr << '\n';
		#endif
		BorderListT::iterator endi = sbi;
		++endi;
		nAssert(endi != build.end());
		TexBorderListT::iterator dbi = final.begin();
		for (; dbi != final.end(); ++dbi)	//#opt: skipped entries are skipped on the next round too
			if (!bcmp(dbi->getFn(), *sbi))
				break;
		// dbi points to the first border that will be overwritten
		int prevTex;
		if (dbi == final.begin())
			prevTex = -1;
		else {
			TexBorderListT::iterator si = dbi;
			prevTex = (--si)->getTex();
		}
		if (texid != prevTex) {
			dbi = final.insert(dbi, TexBorder(*sbi, texid));
			++dbi;	// point dbi to the same item as before
		}
		while (dbi != final.end()) {
			if (bcmp(*endi, dbi->getFn()))
				break;
			prevTex = dbi->getTex();
			dbi = final.erase(dbi);
		}
		// dbi points to the first border that will not be overwritten
		if (texid != prevTex)	// note: new prevTex
			final.insert(dbi, TexBorder(*endi, prevTex));
		sbi = build.erase(sbi);
		sbi = build.erase(sbi);
	}
	#ifdef DEBUG_OVERLAP
	debug(true);
	cerr << "\n\n";
	#endif
	nAssert(final.empty() || final.back().getTex() == -1);
}

void YSegment::extractDrawElements(vector<DrawElement>& dst) const {
	nAssert(build.empty());
	if (final.empty())
		return;
	for (TexBorderListT::const_iterator bi = final.begin();; ++bi) {
		nAssert(bi != final.end());
		TexBorderListT::const_iterator bi1 = bi;
		++bi1;
		if (bi1 == final.end()) {
			nAssert(bi->getTex() == -1);
			break;
		}
		if (bi->getTex() != -1)
			dst.push_back(DrawElement(bi->getFn(), bi1->getFn(), y0 + FINAL_EXTREMECUT, y1 - FINAL_EXTREMECUT, bi->getTex()));
	}
}

// splitOnIntersect splits *si (which must be part of list "segs") to two parts if bfn intersects some of the borders in si's final array
// the split is done in the first intersection y-wise, and to handle multiple intersections, splitOnIntersect must also be called on the new segment
// returns true if si was split
bool splitOnIntersect(SegListT::iterator si, BorderFunctionBase* bfn, SegListT& segs) {
	double splity;
	if (!si->getFirstIntersection(bfn, &splity))	// relies on getFirstIntersection not to return an intersection at the segment's extremes
		return false;
	nAssert(splity > si->getY0() && splity < si->getY1());
	SegListT::iterator insPos = si;
	segs.insert(++insPos, si->split(splity));
	return true;
}

void assembleSegments(const vector<WallBorderSegment>& borders, SegListT& segDest) {
	nAssert(!segDest.empty());	// it must be pre-filled with (possibly empty) segments that cover all possible y's (one seg from -1e99 to 1e99 is good)
	nAssert(borders.size() >= 2);	// using clipping with objects outside the clip are will violate this; disable this line if that's possible

	#ifdef DEBUG_SPLIT
	cerr << '\n';
	#endif
	for (vector<WallBorderSegment>::const_iterator bi = borders.begin(); bi != borders.end(); ++bi) {
		#ifdef DEBUG_SPLIT
		cerr.precision(25);
		cerr << bi->y0 << " -> " << bi->y1 << '\n';
		#endif
		if (bi->y0 == bi->y1)
			continue;
		nAssert(bi->y0 < bi->y1);

		SegListT::iterator si;
		for (si = segDest.begin(); si->getY1() <= bi->y0; ++si)
			nAssert(si != segDest.end());
		// si points to first segment whose y1 > bi->y0
		if (si->getY1() < bi->y0 + SPLIT_TRESHOLD) {	// in this case, this segment is ignored (too little of bi is in this segment)
			si->setY1(bi->y0);
			if (si->width() < SPLIT_TRESHOLD)
				si = segDest.erase(si);
			else
				++si;
		}
		else if (si->getY0() < bi->y0 - SPLIT_TRESHOLD) {	// in this case, the segment must be split (too much of the segment is outside bi)
			SegListT::iterator insPos = si;
			++insPos;
			si = segDest.insert(insPos, si->split(bi->y0));
		}
		else if (si->getY0() < bi->y0) {	// in this case, the segment fits bi nicely and is only trimmed
			si->setY0(bi->y0);
			if (si->width() < SPLIT_TRESHOLD)
				si = segDest.erase(si);
		}
		nAssert(fabs(bi->y0 - si->getY0()) <= 5. * SPLIT_TRESHOLD);
		for (; si->getY1() <= bi->y1; ++si) {
			nAssert(si != segDest.end());
			splitOnIntersect(si, bi->fn, segDest);	// the next round will handle the newly created segment if any
			si->add(bi->fn);
		}
		// si points to first segment whose y1 > bi->y1
		for (;; ++si) {	// the loop is manually broken out of when the segment no longer gets split
			if (si->getY0() > bi->y1 - SPLIT_TRESHOLD) {	// in this case, the segment is ignored (too little of bi is in this segment)
				if (si->getY0() < bi->y1) {
					si->setY0(bi->y1);
					if (si->width() < SPLIT_TRESHOLD)
						segDest.erase(si);
				}
				break;
			}
			else if (si->getY1() > bi->y1 + SPLIT_TRESHOLD) {	// in this case, the segment must be split (too much of the segment is outside bi)
				SegListT::iterator insPos = si;
				++insPos;
				segDest.insert(insPos, si->split(bi->y1));
				bool split = splitOnIntersect(si, bi->fn, segDest);	// the next round will handle the newly created segment if any
				si->add(bi->fn);
				if (!split)
					break;
				if (si->width() < SPLIT_TRESHOLD)
					si = segDest.erase(si);
			}
			else {	// in this case, the segment fits bi nicely and is only trimmed
				si->setY1(bi->y1);
				if (si->width() < SPLIT_TRESHOLD)
					segDest.erase(si);
				else {
					bool split = splitOnIntersect(si, bi->fn, segDest);	// the next round will handle the newly created segment if any
					si->add(bi->fn);
					if (!split)
						break;
					if (si->width() < SPLIT_TRESHOLD)
						si = segDest.erase(si);
				}
			}
		}
	}
	#ifdef DEBUG_SPLIT
	cerr << "- - - split into: - - -\n";
	for (SegListT::iterator dsi = segDest.begin(); dsi != segDest.end(); ++dsi)
		dsi->debug();
	#endif
}

void joinElements(vector<DrawElement>& els) {
	for (vector<DrawElement>::iterator i1 = els.begin(); i1 != els.end(); ++i1)
		for (vector<DrawElement>::iterator i2 = i1 + 1; i2 != els.end(); ) {	// i2.y0 >= i1.y0 because of the ordering
			if (i2->getY0() > i1->getY1() + JOIN_TRESHOLD)	// all elements from i2 on have a greater y0, so no point in continuing
				break;
			if (i1->isJoinable(*i2)) {
				i1->extendDown(i2->getY1());
				i2 = els.erase(i2);
			}
			else
				++i2;
		}
}

vector<DrawElement> assembleWall(const vector<WallBorderSegment>& borders, int texid) {
	SegListT segs;
	segs.push_back(YSegment(-1e99, 1e99));	// this makes the splitting routine simpler, since the new borders will always be within an existing segment

	// split borders into segs
	assembleSegments(borders, segs);

	// finalize segments and extract a DrawElement list
	vector<DrawElement> ret;
	for (SegListT::iterator si = segs.begin(); si != segs.end(); ++si) {
		si->sort();
		si->simplify();
		si->moveElements(texid);
		si->extractDrawElements(ret);
	}

	// finalize the DrawElement list by joining y-neighboring elements with same border functions
	joinElements(ret);
	return ret;
}

vector<DrawElement> assembleScene(const vector<ObjectSource>& objects) {
	SegListT segs;
	segs.push_back(YSegment(-1e99, 1e99));	// this makes the splitting routine simpler, since the new borders will always be within an existing segment

	// finalize segments
	for (vector<ObjectSource>::const_iterator oi = objects.begin(); oi != objects.end(); ++oi) {
		// split borders into segs
		assembleSegments(oi->borders, segs);
		// overlap the old segments
		for (SegListT::iterator si = segs.begin(); si != segs.end(); ++si) {
			si->sort();
			si->simplify();
			SegListT::iterator nexti = si;
			++nexti;
			si->moveElementsWithOverlap(oi->texid);
		}
	}
	// extract a DrawElement list
	vector<DrawElement> ret;
	for (SegListT::iterator si = segs.begin(); si != segs.end(); ++si)
		si->extractDrawElements(ret);

	// finalize the DrawElement list by joining y-neighboring elements with same border functions
	joinElements(ret);
	return ret;
}

SceneAntialiaser::~SceneAntialiaser() {
	for (vector<BorderFunctionBase*>::iterator bi = bfns.begin(); bi != bfns.end(); ++bi)
		delete *bi;
}

void SceneAntialiaser::setScaling(float x0_, float y0_, float scale_) {	// call before add*
	x0 = x0_;
	y0 = y0_;
	scale = scale_;
}

void SceneAntialiaser::addRectangle(float x1, float y1, float x2, float y2, int texture) {
	numAssert2(y1 <= y2, int(y1*10.), int(y2*10.));

	objects.push_back(ObjectSource());
	objects.back().texid = texture;
	vector<WallBorderSegment>& borders = objects.back().borders;

	x1 = x0 + x1 * scale;
	y1 = y0 + y1 * scale;
	x2 = x0 + x2 * scale;
	y2 = y0 + y2 * scale;

	bfns.push_back(new LineFunction(x1, y1, x1, y2));
	borders.push_back(WallBorderSegment(bfns.back(), y1, y2));

	bfns.push_back(new LineFunction(x2, y1, x2, y2));
	borders.push_back(WallBorderSegment(bfns.back(), y1, y2));
}

void SceneAntialiaser::addRectWall(const RectWall& wall, int texture) {
	numAssert2(wall.y1() <= wall.y2(), int(wall.y1()*10.), int(wall.y2()*10.));
	addRectangle(wall.x1(), wall.y1(), wall.x2(), wall.y2(), texture);
}

void SceneAntialiaser::addTriWall (const  TriWall& wall, int texture) {
	objects.push_back(ObjectSource());
	objects.back().texid = texture;
	vector<WallBorderSegment>& borders = objects.back().borders;

	float x1 = x0 + wall.x1() * scale;
	float y1 = y0 + wall.y1() * scale;
	float x2 = x0 + wall.x2() * scale;
	float y2 = y0 + wall.y2() * scale;
	float x3 = x0 + wall.x3() * scale;
	float y3 = y0 + wall.y3() * scale;

	bfns.push_back(new LineFunction(x1, y1, x2, y2));
	borders.push_back(WallBorderSegment(bfns.back(), y1, y2));

	bfns.push_back(new LineFunction(x1, y1, x3, y3));
	borders.push_back(WallBorderSegment(bfns.back(), y1, y3));

	bfns.push_back(new LineFunction(x2, y2, x3, y3));
	borders.push_back(WallBorderSegment(bfns.back(), y2, y3));
}

void SceneAntialiaser::addCircWall(const CircWall& wall, int texture) {
	objects.push_back(ObjectSource());
	objects.back().texid = texture;
	vector<WallBorderSegment>& borders = objects.back().borders;

	double cx = x0 + wall.X() * scale, cy = y0 + wall.Y() * scale;
	double ro = wall.radius() * scale;
	double ri = wall.radius_in() * scale;

	if (wall.angles()[0] == wall.angles()[1]) {	// a whole circle/ring
		bfns.push_back(new CurveFunction(cx, cy, ro, false));
		borders.push_back(WallBorderSegment(bfns.back(), cy - ro, cy + ro));
		bfns.push_back(new CurveFunction(cx, cy, ro, true));
		borders.push_back(WallBorderSegment(bfns.back(), cy - ro, cy + ro));
		if (ri > 0) {	// a ring
			bfns.push_back(new CurveFunction(cx, cy, ri, false));
			borders.push_back(WallBorderSegment(bfns.back(), cy - ri, cy + ri));
			bfns.push_back(new CurveFunction(cx, cy, ri, true));
			borders.push_back(WallBorderSegment(bfns.back(), cy - ri, cy + ri));
		}
		return;
	}

	const Coords& va1 = wall.angle_vector_1();
	const Coords& va2 = wall.angle_vector_2();

	float ar[2];
	for (int i = 0; i < 2; ++i)
		ar[i] = wall.angles()[i] * M_PI / 180.;
	if (ar[1] < ar[0])
		ar[1] += 2. * M_PI;
	nAssert(ar[1] >= ar[0]);
	nAssert(ar[0] >= 0.);

	double yeo = cy - ro * va2.second;	// - belongs to va2.second
	double yei = cy - ri * va2.second;	// - belongs to va2.second
	double ang = ar[0];
	int pi_i = static_cast<int>(ang / M_PI) + 1;
	bool rightSide = (pi_i & 1) != 0;
	double npi = M_PI * pi_i;

	for (;;) {
		double yao = cy - ro * cos(ang);	// - belongs to cos
		double yai = cy - ri * cos(ang);	// - belongs to cos
		if (npi < ar[1]) {	// draw from ang to npi
			bfns.push_back(new CurveFunction(cx, cy, ro, rightSide));
			if (rightSide)
				borders.push_back(WallBorderSegment(bfns.back(), yao, cy + ro));
			else
				borders.push_back(WallBorderSegment(bfns.back(), cy - ro, yao));
			if (ri > 0) {
				bfns.push_back(new CurveFunction(cx, cy, ri, rightSide));
				if (rightSide)
					borders.push_back(WallBorderSegment(bfns.back(), yai, cy + ri));
				else
					borders.push_back(WallBorderSegment(bfns.back(), cy - ri, yai));
			}
		}
		else {
			bfns.push_back(new CurveFunction(cx, cy, ro, rightSide));
			if (rightSide)
				borders.push_back(WallBorderSegment(bfns.back(), yao, yeo));
			else
				borders.push_back(WallBorderSegment(bfns.back(), yeo, yao));
			if (ri > 0) {
				bfns.push_back(new CurveFunction(cx, cy, ri, rightSide));
				if (rightSide)
					borders.push_back(WallBorderSegment(bfns.back(), yai, yei));
				else
					borders.push_back(WallBorderSegment(bfns.back(), yei, yai));
			}
			break;
		}
		ang = npi;
		npi += M_PI;
		rightSide = !rightSide;
	}

	double x1 = cx + va1.first * ri, y1 = cy - va1.second * ri;	// - belongs to va1.second
	double x2 = cx + va1.first * ro, y2 = cy - va1.second * ro;	// - belongs to va1.second
	if (va1.second > 0) {	// this is reversed, too
		swap(x1, x2);
		swap(y1, y2);
	}
	bfns.push_back(new LineFunction(x1, y1, x2, y2));
	borders.push_back(WallBorderSegment(bfns.back(), y1, y2));

	x1 = cx + va2.first * ri; y1 = cy - va2.second * ri;	// - belongs to va2.second
	x2 = cx + va2.first * ro; y2 = cy - va2.second * ro;	// - belongs to va2.second
	if (va2.second > 0) {	// this is reversed, too
		swap(x1, x2);
		swap(y1, y2);
	}
	bfns.push_back(new LineFunction(x1, y1, x2, y2));
	borders.push_back(WallBorderSegment(bfns.back(), y1, y2));
}

void SceneAntialiaser::setClipping(float x1, float y1, float x2, float y2) {
	nAssert(x1 < x2 && y1 < y2);
	clipx1 = x0 + x1 * scale;
	clipy1 = y0 + y1 * scale;
	clipx2 = x0 + x2 * scale;
	clipy2 = y0 + y2 * scale;
	clipFunctionsValid = false;
}

void SceneAntialiaser::createClipFns() {
	if (!clipFunctionsValid) {
		clipLeft = new LineFunction(clipx1, clipy1, clipx1, clipy2);
		clipRight = new LineFunction(clipx2, clipy1, clipx2, clipy2);
		bfns.push_back(clipLeft);
		bfns.push_back(clipRight);
		clipFunctionsValid = true;
	}
}

void SceneAntialiaser::clip(int i0) {
	for (vector<ObjectSource>::iterator object = objects.begin() + i0; object != objects.end(); ++object) {
		int handleBorders = object->borders.size();	// must save this since new borders may be added that don't need clipping
		for (int bi = 0; bi < handleBorders; ++bi) {
			WallBorderSegment* border = &object->borders[bi];

			// clip y direction
			if (border->y0 < clipy1)
				border->y0 = clipy1;
			if (border->y1 > clipy2)
				border->y1 = clipy2;
			if (border->y1 <= border->y0) {	// nothing is visible after clipping
				object->borders.erase(object->borders.begin() + bi);
				--handleBorders;
				--bi;
				continue;
			}

			// clip x direction
			// note: the code from here on is highly of the same structure as pixelLeftSideIntegral
			const ChangePoints lc = border->fn->getChangePoints(clipx1), rc = border->fn->getChangePoints(clipx2);
			ChangePoints::Side ls = lc.startSide, rs = rc.startSide;
			const double* lcpi = lc.points, * rcpi = rc.points;
			double y = border->y0;
			// update border markers to the status around the current y
			while (*lcpi <= y) { swap(ls); ++lcpi; }
			while (*rcpi <= y) { swap(rs); ++rcpi; }
			for (;;) {
				if (ls == ChangePoints::S_Left) {	// out on the left side (until *lcpi)
					createClipFns();
					if (*lcpi >= border->y1) {
						border->fn = clipLeft;
						break;
					}
					WallBorderSegment newSeg(border->fn, *lcpi, border->y1);
					border->fn = clipLeft;
					border->y1 = *lcpi;
					object->borders.push_back(newSeg);
					border = &object->borders.back();	// continue splitting with this new segment
					y = *lcpi++; swap(ls);
					while (*rcpi <= y) { swap(rs); ++rcpi; }
				}
				else if (rs == ChangePoints::S_Right) {	// out on the right side (until *rcpi)
					createClipFns();
					if (*rcpi >= border->y1) {
						border->fn = clipRight;
						break;
					}
					WallBorderSegment newSeg(border->fn, *rcpi, border->y1);
					border->fn = clipRight;
					border->y1 = *rcpi;
					object->borders.push_back(newSeg);
					border = &object->borders.back();	// continue splitting with this new segment
					y = *rcpi++; swap(rs);
					while (*lcpi <= y) { swap(ls); ++lcpi; }
				}
				else if (*lcpi < *rcpi) {	// within the clipping region until *lcpi
					if (*lcpi >= border->y1)
						break;
					WallBorderSegment newSeg(border->fn, *lcpi, border->y1);
					border->y1 = *lcpi;
					object->borders.push_back(newSeg);
					border = &object->borders.back();	// continue splitting with this new segment
					y = *lcpi++; swap(ls);
				}
				else {	// within the clipping region until *rcpi
					if (*rcpi >= border->y1)
						break;
					WallBorderSegment newSeg(border->fn, *rcpi, border->y1);
					border->y1 = *rcpi;
					object->borders.push_back(newSeg);
					border = &object->borders.back();	// continue splitting with this new segment
					y = *rcpi++; swap(rs);
				}
			}
		}
	}
}

void SceneAntialiaser::addRectWallClipped(const RectWall& wall, int texture) {
	int startNew = objects.size();
	addRectWall(wall, texture);
	clip(startNew);
}

void SceneAntialiaser::addTriWallClipped (const  TriWall& wall, int texture) {
	int startNew = objects.size();
	addTriWall (wall, texture);
	clip(startNew);
}

void SceneAntialiaser::addCircWallClipped(const CircWall& wall, int texture) {
	int startNew = objects.size();
	addCircWall(wall, texture);
	clip(startNew);
}

template<class Texturizer>
void SceneAntialiaser::renderTemplate(Texturizer& tex) const {
	vector<DrawElement> drawEls = assembleScene(objects);
	for (vector<DrawElement>::const_iterator ei = drawEls.begin(); ei != drawEls.end(); ++ei) {
		tex.setTex(ei->getTex());
		renderBlock(ei->getY0(), ei->getY1(), ei->getLeft(), ei->getRight(), tex);
	}
}

void SceneAntialiaser::render(  PlainTexTexturizer& tex) const { renderTemplate(tex); }
void SceneAntialiaser::render(PlainColorTexturizer& tex) const { renderTemplate(tex); }

