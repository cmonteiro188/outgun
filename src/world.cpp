#include "commont.h"
#include "world.h"
#include "nassert.h"

#define PHYS_NEW
//#define PHYS_VECTOR_ACC

//same as PLAYER RADIUS (15) + ROCKET RADIUS (3)
#define SHOT_DELTAX 17  // V0.4.8 : A HAIR LESS!

//minimum time between flag steal at base and capture, to consider a map to be valid for scoring
#define MINIMUM_GRAB_TO_CAPTURE_TIME 6.0

//#define ALWAYS_FRICTION

#define PI M_PI //3.1416
#define PIOIT M_PI_4 //0.7854 //DOIS PI SOBRE 8 = PI SOBRE 4 = 0.7854

/* subIntersection:
 * returns true if the area between lines (lx1,ly1)-(lx2,ly2) and (rx1,ry1)-(rx2,ry2) intersects the rectangle (rectx1,recty1)-(rectx2,recty2)
 * every line must be y-ordered : ly1<=ly2, ry1<=ry2, recty1<=recty2 ; additionally rectx1<=rectx2 and lx(y)<=rx(y) for all applicable y
 *
 * how it works:
 * for the appropriate range of y, if there exists an y so that lx(y)<=rectx2 AND rx(y)>=rectx1 , there is an intersection in that range
 * those ranges are solved with simple linear equasions since lx and rx are linear
 */
bool subIntersection(double lx1, double ly1,  double lx2, double ly2,  double rx1, double ry1,  double rx2, double ry2,
				double rectx1, double recty1, double rectx2, double recty2) {
	nAssert(ly1<=ly2 && ry1<=ry2);
	double miny = max(max(ly1, ry1), recty1), maxy=min(min(ly2, ry2), recty2);
	if (maxy < miny)
		return false;
	// first narrow the range by lx(y) <= rectx2
	if (lx1 == lx2) {	// can't formulate a value for intersection-y
		if (lx1 > rectx2)	// lx(y) <= rectx2 identically false => no solutions
			return false;
		// lx(y) <= rectx2 identically true => no narrowing from lx
	}
	else {
		// solve lx(y) == rectx2 , where lx(y) = lx1 + (y-ly1)*(lx2-lx1)/(ly2-ly1)
		double intersect_y = (rectx2 - lx1) * (ly2 - ly1) / (lx2 - lx1) + ly1;
		if (lx2 > lx1) {	// the intersection is at y <= intersect_y
			if (maxy > intersect_y)
				maxy = intersect_y;
		}
		else {	// the intersection is at y >= intersect_y
			if (miny < intersect_y)
				miny = intersect_y;
		}
	}
	if (maxy < miny)
		return false;
	// now narrow the range further by rx(y) >= rectx1, similarly
	if (rx1 == rx2)
		return (rx1 >= rectx1);
	else {
		double intersect_y = (rectx1 - rx1) * (ry2 - ry1) / (rx2 - rx1) + ry1;
		if (rx2 > rx1) {	// the intersection is at y >= intersect_y
			if (miny < intersect_y)
				miny = intersect_y;
		}
		else {	// the intersection is at y <= intersect_y
			if (maxy > intersect_y)
				maxy = intersect_y;
		}
	}
	return (maxy >= miny);
}

bool TriWall::intersects_rect(double rx1, double ry1, double rx2, double ry2) const {
	nAssert(ry1<=ry2 && rx1<=rx2);
	nAssert(p1y<=p2y && p2y<=p3y);
	if (rx1>boundx2 || rx2<boundx1 || ry1>boundy2 || ry2<boundy1)
		return false;
	/* idea: triangle is split in two triangles: y<=y2 and y>=y2
	 * for both parts, the right and left edge are checked separately
	 * for each edge there may be a region [yi0, yi1] where (for a right side edge) xr(y)>=x1
	 * if those regions overlap with each other and [ry1, ry2], there exists an intersection
	 */
	if (p2x < p1x + (p2y-p1y) * (p3x-p1x) / (p3y-p1y)) {	// p2 is left to the p1-p3 line
		if (subIntersection(p1x,p1y, p2x,p2y, p1x,p1y, p3x,p3y, rx1, ry1, rx2, ry2))	// part y<=p2y : L,R sides p1-p2, p1-p3
			return true;
		if (subIntersection(p2x,p2y, p3x,p3y, p1x,p1y, p3x,p3y, rx1, ry1, rx2, ry2))	// part y>=p2y : L,R sides p2-p3, p1-p3
			return true;
	}
	else {
		if (subIntersection(p1x,p1y, p3x,p3y, p1x,p1y, p2x,p2y, rx1, ry1, rx2, ry2))	// part y<=p2y : L,R sides p1-p3, p1-p2
			return true;
		if (subIntersection(p1x,p1y, p3x,p3y, p2x,p2y, p3x,p3y, rx1, ry1, rx2, ry2))	// part y>=p2y : L,R sides p1-p3, p2-p3
			return true;
	}
	return false;
}

CircWall::CircWall(int x_, int y_, int ro_, int ri_, float ang1, float ang2, int tex_, int alpha_):
	x(x_),
	y(y_),
	ro(ro_),
	ri(ri_),
	tex(tex_),
	alpha(alpha_)
{
	angle[0] = ang1;
	angle[1] = ang2;
	float a1r = angle[0] * M_PI / 180;
	float a2r = angle[1] * M_PI / 180;
	va1 = Coords(sin(a1r), cos(a1r));
	va2 = Coords(sin(a2r), cos(a2r));
	float a2r_greater = (a2r >= a1r) ? a2r : (a2r + 2.*M_PI);
	if (a1r == a2r)
		anglecos = -1.;	// full circle => max width
	else
		anglecos = cos((a2r_greater - a1r)/2.);
	float midangle = (a2r_greater + a1r) / 2;
	midvec = Coords(sin(midangle), cos(midangle));
}

/* CircWall::intersects_rect idea:
 *
 * this function cheats a bit: it often returns true even if they don't really intersect, but if it returns false, it's certain they don't intersect
 *
 * the rectangle is extended: instead of it, the intersection is tested against it's bounding circle
 * the wall is extended: it's limiting angles are discarded, and so it's treated as a solid ring
 */
bool CircWall::intersects_rect(double x1, double y1, double x2, double y2) const {
	// more crude check against the wall's bounding rectangle would be: return x1<=x+ro && x2>=x-ro && y1<=y+ro && y2>=y-ro;
	double rwr = (x2 - x1) / 2., rhr = (y2 - y1) / 2.;
	double rr = sqrt( rwr*rwr + rhr*rhr );
	double rcx = x1 + rwr, rcy = y1 + rhr;
	double dx = rcx - x, dy = rcy - y;
	double dcr = sqrt( dx*dx + dy*dy );	// this is the radius of the rect bound-circle center in relation to the wall center
	// if the extended rectangle is wholly within the wall inner bounding circle (r=ri), there's no intersection
	if (dcr + rr < ri)
		return false;
	// now if the extended rectangle and the wall bounding circle (r=ro) overlap, there is an intersection, otherwise not
	if (dcr - rr < ro)
		return true;
	return false;
}

bool Map::load(const char *mapdir, const string& mapname) {
	char lebuffer[1024];
	char dest[WHERE_PATH_SIZE];

	// MAPDIR + / + MAPNAME + .TXT
	strcpy(lebuffer, mapdir);
	put_backslash(lebuffer);
	strcat(lebuffer, mapname.c_str());
	strcat(lebuffer, ".txt");

	//append all that to the root dir of the game
	append_filename(dest, wheregamedir, lebuffer, WHERE_PATH_SIZE);
	FILE *fmap = fopen(dest, "r");	// FIXME: r or rb ??
	if (fmap) {
		*this = Map();
		NLubyte lebigbuf[65536];
		int numread = fread((void*)lebigbuf, 1, 65536, fmap);
		crc = nlGetCRC16((NLubyte*)lebigbuf, numread);
		rewind(fmap);
		bool ok = parse_label(fmap, 0);
		fclose(fmap);
		if (!ok) {
			LOG1("Error loading map '%s'\n", mapname.c_str());
			return false;
		}
		fclose(fmap);
		return true;
	}
	else {
		LOG1("can't load mapfile from '%s'!\n", dest);
		return false;
	}
}

bool Map::parse_label(FILE* f, const char* scan_label, int crx, int cry, float scalex, float scaley) {	// crx,cry = "current room pointer"
	rewind(f);
	for (;;) {
		char s[1024];
		if (!fgets(s, 1024, f)) {	// end-of-file or error
			if (scan_label) {
				LOG1("Map label %s not found\n", scan_label);
				return false;
			}
			else
				return true;
		}
		for (int si=strlen(s)-1; si>=0; --si) {
			if (s[si]=='\n' || s[si]=='\r')
				s[si] = '\0';
			else
				break;
		}
		if (s[0] == '\0' || s[0]==';')
			continue;
		if (s[0]==':') {	// a label is found
			if (!scan_label)
				return true;
			if (!strcmp(s+1, scan_label))
				scan_label = 0;
			continue;
		}
		if (scan_label)
			continue;
		char nullc;	// to be used at ends of sscanf to make sure there is nothing extra on the line
		if (s[0]=='W' || s[0]=='G') {	// W x1 y1 x2 y2 [tex alpha] : rectangular wall (x1,y1)-(x2,y2) using given texture and alpha ; G : ground texture
			float x1, y1, x2, y2;
			int texid, alpha;
			int n = sscanf(s+1, " %f %f %f %f %i %i %c", &x1, &y1, &x2, &y2, &texid, &alpha, &nullc);
			if (n == 4)
				texid = 0;
			if (n == 4 || n == 5)
				alpha = 255;
			if (n < 4 || n > 6 || crx < 0 || cry < 0 || crx >= w || cry >= h) {
				LOG1("Invalid map line: %s\n", s);
				return false;
			}
			x1 *= plw / scalex; x2 *= plw / scalex;
			y1 *= plh / scaley; y2 *= plh / scaley;
			Room& rm = room[crx][cry];
			vector<RectWall>& wvec = (s[0] == 'W') ? rm.rwalls : rm.rground;
			wvec.push_back(RectWall(int(x1), int(y1), int(x2), int(y2), texid, alpha));
			continue;
		}
		if (s[0]=='T') {	// T (W|G) x1 y1 x2 y2 x3 y3 [tex alpha] : triangular wall (W) or ground tex (G) (x1,y1)-(x2,y2)-(x3,y3) using given texture and alpha
			char type;
			float x1, y1, x2, y2, x3, y3;
			int texid, alpha;
			int n = sscanf(s+1, " %c %f %f %f %f %f %f %i %i %c", &type, &x1, &y1, &x2, &y2, &x3, &y3, &texid, &alpha, &nullc);
			if (n == 7)
				texid = 0;
			if (n == 7 || n == 8)
				alpha = 255;
			if (n < 7 || n > 9 || (type != 'W' && type != 'G') || crx < 0 || cry < 0 || crx >= w || cry >= h) {
				LOG1("Invalid map line: %s\n", s);
				return false;
			}
			x1 *= plw / scalex; x2 *= plw / scalex; x3 *= plw / scalex;
			y1 *= plh / scaley; y2 *= plh / scaley; y3 *= plh / scaley;
			Room& rm = room[crx][cry];
			vector<TriWall>& wvec = (type == 'W') ? rm.twalls : rm.tground;
			wvec.push_back(TriWall(int(x1), int(y1), int(x2), int(y2), int(x3), int(y3), texid, alpha));
			continue;
		}
		if (s[0]=='C') {	// C (W|G) x y or [ir [a1 a2 [tex alpha]]] : circular wall (W) or ground tex (G)
			char type;
			float x, y, ro, ri, a1, a2;
			int texid, alpha;
			int n = sscanf(s + 1, " %c %f %f %f %f %f %f %i %i %c", &type, &x, &y, &ro, &ri, &a1, &a2, &texid, &alpha, &nullc);
			switch (n) {
				case 4:
					ri = 0;			// flow
				case 5:
					a1 = a2 = 0;	// flow
				case 7:
					texid = 0;
				case 8:
					alpha = 255;
					break;
				default: break;
			}
			if (a1 < 0)
				a1 += 360;
			if (a2 < 0)
				a2 += 360;
			if ((n != 9 && n != 8 && n != 7 && n != 5 && n != 4) || ro <= 0 || ri < 0 || ri >= ro || (a1 != 0 && a1 == a2) || a1 < 0 || a2 < 0 || a1 >= 360 || a2 >= 360) {
				LOG1("Invalid map line: %s\n", s);
				return false;
			}
			x *= plw / scalex;
			y *= plh / scaley;
			ro *= plh / scaley;
			ri *= plh / scaley;
			Room& rm = room[crx][cry];
			vector<CircWall>& wvec = (type == 'W') ? rm.cwalls : rm.cground;
			wvec.push_back(CircWall(int(x), int(y), int(ro), int(ri), a1, a2, texid, alpha));
			continue;
		}
		if (s[0]=='R') {	// R x y : set room pointer to (x,y)
			int n = sscanf(s+1, " %i %i %c", &crx, &cry, &nullc);
			if (n!=2 || crx<0 || crx>=w || cry<0 || cry>=h) {
				LOG1("Invalid map line: %s\n", s);
				return false;
			}
			continue;
		}
		if (s[0]=='X') {	// X label x1 y1 [x2 y2] : add walls from label to the rectangle (x1,y1)-(x2,y2)
			char nextlabel[64];
			int rx1, ry1, rx2, ry2;
			int n = sscanf(s+1, " %64s %i %i %i %i %c", nextlabel, &rx1, &ry1, &rx2, &ry2, &nullc);
			if (n == 3) {	// one room only
				rx2 = rx1;
				ry2 = ry1;
			}
			if ((n!=3 && n!=5) || rx1<0 || rx2>=w || rx2<rx1 || ry1<0 || ry2>=h || ry2<ry1) {
				LOG1("Invalid map line: %s\n", s);
				return false;
			}
			long filepos = ftell(f);
			for (int ry=ry1; ry<=ry2; ry++)
				for (int rx=rx1; rx<=rx2; rx++)
					if (!parse_label(f, nextlabel, rx, ry, scalex, scaley))
						return false;
			fseek(f, filepos, SEEK_SET);
			crx=rx2; cry=ry2;	// compatibility with original sloppy specs (needed?)
			continue;
		}
		if (!strncmp(s, "P width ", 8)) {	// P width w : set map width to w rooms #FIXME: Allow "P   width"
			if (w != 0) {
				LOG("Redefined map width\n");
				return false;
			}
			int n = sscanf(s+1, " width %i %c", &w, &nullc);
			if (w<1 || n!=1) {
				LOG1("Invalid map line: %s\n", s);
				return false;
			}
			room.resize(w);
			for (vector< vector<Room> >::iterator ri=room.begin(); ri!=room.end(); ++ri)
				ri->resize(h);
			continue;
		}
		if (!strncmp(s, "P height ", 9)) {	// P height h : set map height to h rooms
			if (h != 0) {
				LOG("Redefined map height\n");
				return false;
			}
			int n = sscanf(s+1, " height %i %c", &h, &nullc);
			if (h<1 || n!=1) {
				LOG1("Invalid map line: %s\n", s);
				return false;
			}
			for (vector< vector<Room> >::iterator ri=room.begin(); ri!=room.end(); ++ri)
				ri->resize(h);
			continue;
		}
		if (!strncmp(s, "P title ", 8)) {	// P title text : set map title to text
			if (!title.empty()) {
				LOG("Redefined map title\n");
				return false;
			}
			title=string(s+8);
			continue;
		}
		if (!strncmp(s, "P author ", 9)) {	// P author text : set map author to text
			if (!author.empty()) {
				LOG("Redefined map author\n");
				return false;
			}
			author=string(s+9);
			continue;
		}
		if (!strncmp(s, "spawn ", 6)) {	// spawn t rx ry x y : make a spawn spot for team t at room (rx,ry) at (x,y)
			int team, rx, ry;
			float x, y;
			int n = sscanf(s, "spawn %i %i %i %f %f %c", &team, &rx, &ry, &x, &y, &nullc);
			if (n != 5) {
				LOG1("Invalid map line: %s\n", s);
				return false;
			}
			spoint_t spot;
			spot.px = bound(rx, 0, w-1);
			spot.py = bound(ry, 0, h-1);
			spot.x = (int)(x * (double)plw / scalex);
			spot.y = (int)(y * (double)plh / scaley);
			tinfo[team].spawn.push_back(spot);
			continue;
		}
		if (!strncmp(s, "flag ", 5)) {	// flag t rx ry x y : set team t's flag position to room (rx,ry) at (x,y)
			int team, rx, ry;
			float x, y;
			int n = sscanf(s, "flag %i %i %i %f %f %c", &team, &rx, &ry, &x, &y, &nullc);
			if (n != 5) {
				LOG1("Invalid map line: %s\n", s);
				return false;
			}
			spoint_t flag(bound(rx, 0, w-1), bound(ry, 0, h-1),
				static_cast<int>(x * (double)plw / scalex), static_cast<int>(y * (double)plh / scaley));
			tinfo[team].flags.push_back(flag);
			continue;
		}
		if (s[0]=='V') {	// V ver : set file format version
			int n = sscanf(s+1, " %i %c", &ver, &nullc);
			if (n != 1) {
				LOG1("Invalid map line: %s\n", s);
				return false;
			}
			continue;
		}
		if (s[0]=='S') {	// S x y : set map scale
			float sx, sy;
			int n = sscanf(s+1, " %f %f %c", &sx, &sy, &nullc);
			if (n != 2 || sx <= 0 || sy <= 0) {
				LOG1("Invalid map line: %s\n", s);
				return false;
			}
			scalex = sx;
			scaley = sy;
			continue;
		}
		LOG1("Unrecognized map line: %s\n", s);
	}
}

/* bounceFromPoint():
 *
 * calculates how many times the vector (mx,my) can be traveled until point (dx,dy) is hit by a circle of radius r
 *
 *
 *        (mx,my)      __--
 *          \    __--^^
 *         __+-^^  + -(dx,dy)
 *     +-^^
 *   (0,0)
 *  
 * | t(mx,my)-(dx,dy) | = r
 * take the smaller solution of t (if any real solution exists)
 *
 * d? = distance vector of the point, m? = movement vector of the circle, r = radius of the circle
 * returns: pair( t, pair(collisionn-centern, collisionp-centerp) ) or pair(1e99, ...) for no collision
 */
BounceData bounceFromPoint(double dx, double dy, double mx, double my, double r) {
	double m2 = mx*mx + my*my, r2 = r*r;
	double mdotd = mx*dx + my*dy;
	double d2 = dx*dx + dy*dy;
	double disc = mdotd*mdotd - m2*(d2-r2);
	if (disc >= 0) {	// there are real solutions
		double t = (mdotd-sqrt(disc))/m2;	// the collision with smaller t (the larger t is when going away from the point)
		if (t >= 0)
			return BounceData(t, Coords(dx-t*mx, dy-t*my));
	}
	return BounceData(1e99, Coords());	// no collision
}

/* bounceFromLine():
 *
 * calculates how many times the vector (mx,my) can be traveled until wall (dx1,dy1)-(dx2,dy2) is hit by a circle of radius r
 *
 *
 *        (mx,my)      __--
 *          \    __--^^
 *         __+-^^  + -(dx1,dy1)
 *     +-^^      .>|
 *   (0,0)      /  + -(dx2,dy2)
 *             /
 *           wall
 *  
 * the circle hits the wall proper with it's center projection on the line
 * | ( t(mx,my)-(dx1,dy1) ) x ( (dx2,dy2)-(dx1,dy1) ) | / | (dx2,dy2)-(dx1,dy1) | = r
 * take the smaller solution of t and make sure the point is on the line
 *
 * d?? = distance vectors of the line's end-points, m? = movement vector of the circle, r = radius of the circle
 * returns: pair( t, pair(collisionn-centern, collisionp-centerp) ) or pair(1e99, ...) for no collision
 */
BounceData bounceFromLine(double dx1, double dy1, double dx2, double dy2, double mx, double my, double r) {
	// t * ( mx(dy2-dy1) - my(dx2-dx1) ) = dx1(dy2-dy1) - dy1(dx2-dx1) +-R*|(dx2,dy2)-(dx1,dy1)|
	double diffx = dx2-dx1, diffy = dy2-dy1;
	double div = mx*diffy - my*diffx;
	if (div != 0) {	// div == 0 <=> movement is parallel to the line => no collisions possible
		double rBase = ( dx1*diffy - dy1*diffx ) / div;
		double rAdd = r * sqrt(diffx*diffx+diffy*diffy) / div;
		double t = rBase - fabs(rAdd);	// the collision with smaller t (the larger t is when going away from the line)
		if (t >= 0) {
			// make sure we are not off an end of the line
			// this can surely be calculated in a simpler way, but this first came to mind
			// collp = p1 + k(p2-p1)	0<=k<=1 if on the line
			// | t*m - collp |  minimum (=r)
			// | t*m - p1 - k(p2-p1) |  minimum (=r)
			// ( t*mx - dx1 - k(dx2-dx1) )^2 + ( t*my - dy1 - k(dy2-dy1) )^2  minimum (=r)
			// (dx2-dx1)*( t*mx - dx1 - k(dx2-dx1) ) + (dy2-dy1)*( t*my - dy1 - k(dy2-dy1) ) = 0  (derivative of the expression above *(-.5))
			// (dx2-dx1)*(t*mx-dx1) + (dy2-dy1)*(t*my-dy1) = k[ (dx2-dx1)^2 + (dy2-dy1)^2 ]
			double k = ( diffx*(t*mx-dx1) + diffy*(t*my-dy1) ) / (diffx*diffx + diffy*diffy);
			if (k>=0. && k<=1.)
				return BounceData(t, Coords(dx1+k*diffx-t*mx, dy1+k*diffy-t*my));
		}
	}
	return BounceData(1e99, Coords());	// no collision
}

/* bounceFromArc():
 *
 * calculates how many times the vector (mx,my) can be traveled until the arc is hit by a circle of radius cr
 *
 *
 *        (mx,my)      __--
 *          \    __--^^
 *         __+-^^   /^^^^
 *     +-^^        /   
 *   (0,0) av,____|____.<-- (dx,dy)
 *           `    |     }
 *                 \    } ar
 *                  \___}
 *
 * the circle hits the arc proper with it's center ar+cr (if outside) or ar-cr (if inside) and within the given angle from arc center vector
 *
 * | t(mx,my)-(dx,dy) | = ar+-cr , taking the smaller solution of t and making sure the position is within the given angle from av
 *
 * d? = distance vector of the arc's radial center, m? = movement vector of the circle, ar = radius of the arc, cr = radius of the moving circle
 * av = arc center unit vector, ahwcos = cosine of half arc width
 * returns: pair( t, pair(collisionn-centern, collisionp-centerp) ) or pair(1e99, ...) for no collision
 */
BounceData bounceFromArc(double dx, double dy, double mx, double my, const Coords& av, double ahwcos, double ar, double cr, bool outside) {
	// check for solution A (if there is one, there is no need to check B)
	double bounceRad = ar + (outside?cr:-cr);
	double m2 = mx*mx + my*my, r2 = bounceRad*bounceRad;
	double mdotd = mx*dx + my*dy;
	double d2 = dx*dx + dy*dy;
	double disc = mdotd*mdotd - m2*(d2-r2);
	if (disc >= 0) {	// there are real solutions
		double t;
		if (outside)
			t = (mdotd-sqrt(disc))/m2;	// the collision with smaller t (the larger t is when going away from the center)
		else
			t = (mdotd+sqrt(disc))/m2;	// the collision with larger t (the smaller t is when going towards the center)
		if (t >= 0) {
			// make sure the point is within the given angle from av
			// [ (t(mx,my) - (dx,dy)) dot av ] / [ |t(mx,my) - (dx,dy)| * |av| ] >= ahwcos
			double xd = t*mx - dx, yd = t*my - dy;
			double dot = xd*av.first - yd*av.second;	//NOTE: - because av.second is in reversed coordinates
			if (dot >= ahwcos * bounceRad) {	// |(dx,dy) - t(mx,my)| = bounceRad, |av| = 1
				// calc the vector from t(mx,my) to collision point:
				// length = cr
				// (xd,yd) = along that vector, direction from radial center to center of circle, length bounceRad
				double mul = (ar-bounceRad)/bounceRad;
				return BounceData(t, Coords(xd*mul, yd*mul));
			}
		}
	}
	return BounceData(1e99, Coords());
}

void tryBounce(BounceData* bd, const RectWall& w, double stx, double sty, double mx, double my, double plyRadius) {
	#define add_rv() if (rv.first < bd->first) *bd = rv;

	BounceData rv;
	rv.first = 1e99;
	bool onLine = false;
	if (mx>0 && w.a>stx)	// check vertical wall a
		rv = bounceFromLine(w.a - stx, w.b - sty, w.a - stx, w.d - sty, mx, my, plyRadius);
	else if (mx<0 && w.c<stx)	// check vertical wall c
		rv = bounceFromLine(w.c - stx, w.b - sty, w.c - stx, w.d - sty, mx, my, plyRadius);
	if (rv.first < 1e10) {
		onLine = true;
		add_rv();
	}
	if (my>0 && w.b>sty)	// check horizontal wall b
		rv = bounceFromLine(w.a - stx, w.b - sty, w.c - stx, w.b - sty, mx, my, plyRadius);
	else if (my<0 && w.d<sty)	// check horizontal wall d
		rv = bounceFromLine(w.a - stx, w.d - sty, w.c - stx, w.d - sty, mx, my, plyRadius);
	if (rv.first < 1e10) {
		onLine = true;
		add_rv();
	}
	if (!onLine) {	// check corners
		rv = bounceFromPoint(w.a - stx, w.b - sty, mx, my, plyRadius);
		add_rv();
		rv = bounceFromPoint(w.c - stx, w.b - sty, mx, my, plyRadius);
		add_rv();
		rv = bounceFromPoint(w.a - stx, w.d - sty, mx, my, plyRadius);
		add_rv();
		rv = bounceFromPoint(w.c - stx, w.d - sty, mx, my, plyRadius);
		add_rv();
	}

	#undef add_rv
}

void tryBounce(BounceData* bd, const TriWall& w, double stx, double sty, double mx, double my, double plyRadius) {
	#define add_rv() if (rv.first < bd->first) *bd = rv;

	BounceData rv;
	rv = bounceFromLine(w.p1x - stx, w.p1y - sty, w.p2x - stx, w.p2y - sty, mx, my, plyRadius);	// wall p1-p2
	add_rv();
	rv = bounceFromLine(w.p1x - stx, w.p1y - sty, w.p3x - stx, w.p3y - sty, mx, my, plyRadius);	// wall p1-p3
	add_rv();
	rv = bounceFromLine(w.p2x - stx, w.p2y - sty, w.p3x - stx, w.p3y - sty, mx, my, plyRadius);	// wall p2-p3
	add_rv();
	rv = bounceFromPoint(w.p1x - stx, w.p1y - sty, mx, my, plyRadius);
	add_rv();
	rv = bounceFromPoint(w.p2x - stx, w.p2y - sty, mx, my, plyRadius);
	add_rv();
	rv = bounceFromPoint(w.p3x - stx, w.p3y - sty, mx, my, plyRadius);
	add_rv();

	#undef add_rv
}

void tryBounce(BounceData* bd, const CircWall& w, double stx, double sty, double mx, double my, double plyRadius) {
	#define add_rv() if (rv.first < bd->first) *bd = rv;

	BounceData rv;
	// outside
	rv = bounceFromArc(w.x - stx, w.y - sty, mx, my, w.midvec, w.anglecos, w.ro, plyRadius, true);
	add_rv();
	// inside
	if (w.ri > plyRadius) {
		rv = bounceFromArc(w.x - stx, w.y - sty, mx, my, w.midvec, w.anglecos, w.ri, plyRadius, false);
		add_rv();
	}
	if (w.angle[0] == w.angle[1])	// no sectoring
		return;
	double p1x = w.x + w.ro*w.va1.first - stx, p1y = w.y - w.ro*w.va1.second - sty;	//NOTE: - ...*w.va1.second because va1.second is in reversed coordinates
	double p2x = w.x + w.ri*w.va1.first - stx, p2y = w.y - w.ri*w.va1.second - sty;	//NOTE: - ...*w.va1.second because va1.second is in reversed coordinates
	// side wall at angle va1
	rv = bounceFromLine(p1x, p1y, p2x, p2y, mx, my, plyRadius);
	add_rv();
	// corners at angle va1
	rv = bounceFromPoint(p1x, p1y, mx, my, plyRadius);
	add_rv();
	rv = bounceFromPoint(p2x, p2y, mx, my, plyRadius);
	add_rv();
	// side wall at angle va2
	p1x = w.x + w.ro*w.va2.first - stx, p1y = w.y - w.ro*w.va2.second - sty;	//NOTE: - ...*w.va2.second because va2.second is in reversed coordinates
	p2x = w.x + w.ri*w.va2.first - stx, p2y = w.y - w.ri*w.va2.second - sty;	//NOTE: - ...*w.va2.second because va2.second is in reversed coordinates
	rv = bounceFromLine(p1x, p1y, p2x, p2y, mx, my, plyRadius);
	add_rv();
	// corners at angle va2
	rv = bounceFromPoint(p1x, p1y, mx, my, plyRadius);
	add_rv();
	rv = bounceFromPoint(p2x, p2y, mx, my, plyRadius);
	add_rv();

	#undef add_rv
}

BounceData WorldBase::genGetTimeTillWall(const Room& room, double x, double y, double mx, double my, double radius, float maxFraction) {
	BounceData bd;
	bd.first = 1e99;

	if (mx == 0 && my == 0)
		return bd;

	Coords bbox0(min(x-radius, x+mx*maxFraction-radius), min(y-radius, y+my*maxFraction-radius)),
	       bbox1(max(x+radius, x+mx*maxFraction+radius), max(y+radius, y+my*maxFraction+radius));

	for (vector<RectWall>::const_iterator wi=room.rwalls.begin(); wi!=room.rwalls.end(); ++wi) {	// go through rectangular walls
		// fast and crude bounding-box style check first
		if (!wi->intersects_rect(bbox0.first, bbox0.second, bbox1.first, bbox1.second))
			continue;
		// check more carefully
		tryBounce(&bd, *wi, x, y, mx, my, radius);
		#ifndef NDEBUG
		if (bd.first < 1e10) {
			double dx = bd.second.first, dy = bd.second.second, r = radius;
			nAssert(fabs(dx*dx+dy*dy-r*r)<.0001);
		}
		#endif
	}
	for (vector<TriWall>::const_iterator wi=room.twalls.begin(); wi!=room.twalls.end(); ++wi) {	// go through triangular walls
		// fast and crude bounding-box style check first
		if (!wi->intersects_rect(bbox0.first, bbox0.second, bbox1.first, bbox1.second))
			continue;
		// check more carefully
		tryBounce(&bd, *wi, x, y, mx, my, radius);
		#ifndef NDEBUG
		if (bd.first < 1e10) {
			double dx = bd.second.first, dy = bd.second.second, r = radius;
			nAssert(fabs(dx*dx+dy*dy-r*r)<.0001);
		}
		#endif
	}
	for (vector<CircWall>::const_iterator wi=room.cwalls.begin(); wi!=room.cwalls.end(); ++wi) {	// go through circular walls
		// fast and crude bounding-box style check first
		if (!wi->intersects_rect(bbox0.first, bbox0.second, bbox1.first, bbox1.second))
			continue;
		// check more carefully
		tryBounce(&bd, *wi, x, y, mx, my, radius);
		#ifndef NDEBUG
		if (bd.first < 1e10) {
			double dx = bd.second.first, dy = bd.second.second, r = radius;
			nAssert(fabs(dx*dx+dy*dy-r*r)<.0001);
		}
		#endif
	}

	nAssert(bd.first>=0.);
	return bd;
}

BounceData WorldBase::getTimeTillBounce(const Room& room, const PlayerBase& pl, double plyRadius, float maxFraction) {
	return genGetTimeTillWall(room, pl.lx, pl.ly, pl.sx, pl.sy, plyRadius, maxFraction);
}

double WorldBase::getTimeTillWall(const Room& room, const rocket_c& rock, float maxFraction) {
	return genGetTimeTillWall(room, rock.x, rock.y, rock.sx, rock.sy, ROCKET_RADIUS, maxFraction).first;
}

double WorldBase::getTimeTillCollision(const PlayerBase& pl, const rocket_c& rock, double collRadius) {
	double dx = rock.x-pl.lx, dy = rock.y-pl.ly, r2 = collRadius*collRadius;
	if (dx*dx + dy*dy < r2)
		return 0;
	double mx = pl.sx-rock.sx, my = pl.sy-rock.sy;

	double m2 = mx*mx + my*my;
	double mdotd = mx*dx + my*dy;
	double d2 = dx*dx + dy*dy;
	double disc = mdotd*mdotd - m2*(d2-r2);
	if (disc >= 0) {	// there are real solutions
		double t = (mdotd-sqrt(disc))/m2;	// the collision with smaller t (the larger t is when going away)
		if (t >= 0)
			return t;
	}
	return 1e99;
}

void WorldBase::applyPlayerAcceleration(int pid) {
	PlayerBase* h = player[pid].getPtr();
	bool turbo = h->item_speed;
	bool deathbringer_affected = h->under_deathbringer_effect(get_time());
	const Team& enemy = teams[1 - pid / TSIZE];
	bool carryFlag = false;
	for (vector<Flag>::const_iterator fi = enemy.flags().begin(); fi != enemy.flags().end(); ++fi)
		if (fi->carrier() == pid) {
			carryFlag = true;
			break;
		}

	//select effective physics vars for the player
	float player_accel, player_friction, player_maxspeed;
	if (h->controls.isRun()) {
		if (turbo) {
			player_accel    = svp_accel_turborun;
			player_friction = svp_fric_turborun;
			player_maxspeed = svp_maxspeed_turborun;
		}
		else {
			player_accel    = svp_accel_run;
			player_friction = svp_fric_run;
			player_maxspeed = svp_maxspeed_run;
		}
	}
	else {
		if (turbo) {
			player_accel    = svp_accel_turbo;
			player_friction = svp_fric_turbo;
			player_maxspeed = svp_maxspeed_turbo;
		}
		else {
			player_accel    = svp_accel;
			player_friction = svp_fric;
			player_maxspeed = svp_maxspeed;
		}
	}
	//flag carrier disadvantage when running
	if (h->controls.isRun() && carryFlag)
		player_maxspeed -= svp_flag_penalty;

	int xAcc = (h->controls.isRight()?1:0) - (h->controls.isLeft()?1:0), yAcc = (h->controls.isDown()?1:0) - (h->controls.isUp()?1:0);

	#ifdef PHYS_VECTOR_ACC

	// this is a more physically correct model by Nix

	// scale these up by 1.2 so the acceleration is near the average of original (either 1 or sqrt(2) times)
	player_maxspeed *= 1.2;
	player_friction *= 1.2;
	player_accel    *= 1.2;
	player_accel    += player_friction;	// to balance forward acceleration with the original model; backward acceleration is too big however

	// friction
	float spd = sqrt( h->sx*h->sx + h->sy*h->sy );
	if (spd > 0) {
		float mul;
		if (spd <= player_friction)
			mul = 0.;
		else
			mul = 1. - player_friction/spd;
		h->sx *= mul;
		h->sy *= mul;
	}

	// acceleration
	if (!deathbringer_affected && spd<player_maxspeed) {
		// spd<player_maxspeed is a hack: the player is frozen for a while when maxspeed decreases
		// to do this in a nicer way, player_maxspeed would have to be replaced with a speed-proportional term to friction
		float mul = player_accel;
		if (xAcc!=0 && yAcc!=0)	// normalize the total acceleration vector
			mul /= sqrt(2.);

		h->sx += float(xAcc)*mul;
		h->sy += float(yAcc)*mul;
		spd = sqrt( h->sx*h->sx + h->sy*h->sy );

		if (spd > player_maxspeed) {
			float mul = player_maxspeed/spd;
			h->sx *= mul;
			h->sy *= mul;
		}
	}

	#else	// PHYS_VECTOR_ACC

	// this is the original weird physics model only re-written

	// friction
	float absx=fabs(h->sx), absy=fabs(h->sy);
	if (!xAcc || absx>=player_maxspeed) {
		if (absx > player_friction)
			h->sx *= 1. - player_friction/absx;
		else
			h->sx = 0.;
	}
	if (!yAcc || absy>=player_maxspeed) {
		if (absy > player_friction)
			h->sy *= 1. - player_friction/absy;
		else
			h->sy = 0.;
	}

	// acceleration
	if (!deathbringer_affected) {
		if (fabs(h->sx) < player_maxspeed)
			h->sx += float(xAcc)*player_accel;
		if (fabs(h->sy) < player_maxspeed)
			h->sy += float(yAcc)*player_accel;
	}

	#endif	// PHYS_VECTOR_ACC else
}

void WorldBase::returnAllFlags() {
	for (int t = 0; t < 2; t++)
		teams[t].return_all_flags();
}

void WorldBase::returnFlag(int team, int flag) {
	teams[team].return_flag(flag);
}

void WorldBase::dropFlag(int team, int flag, int px, int py, int x, int y) {
	const spoint_t pos(px, py, x, y + 15);
	teams[team].drop_flag(flag, pos);
}

void WorldBase::stealFlag(int team, int flag, int carrier) {
	teams[team].steal_flag(flag, carrier, get_time());
}

void WorldBase::addRocket(int i, int playernum, int team, int px, int py, int x, int y,
													bool power, int dir, int xdelta, int frameAdvance, PhysicsCallbacksBase& cb) {
	rocket_c* r = &rock[i];
	r->owner = playernum;
	r->team = team;
	r->power = power;
	r->px = px;
	r->py = py;
	r->x = x;
	r->y = y;

	float deg = dir * PIOIT;

	if (xdelta) {
		r->sx = xdelta * SHOT_DELTAX * cos(deg + PI/2);
		r->sy = xdelta * SHOT_DELTAX * sin(deg + PI/2);
		double wallTime = getTimeTillWall(map.room[px][py], *r, 1.);
		r->move(1);
		if (wallTime < 1.) {
			cb.rocketHitWall(i, r->power, r->x, r->y, r->px, r->py);
			return;
		}
	}

	r->sx = cos(deg) * ROCKET_SPEED;
	r->sy = sin(deg) * ROCKET_SPEED;
	double advance = .5 + double(frameAdvance);
	double wallTime = getTimeTillWall(map.room[px][py], *r, 1.);
	if (wallTime <= advance) {
		r->move(wallTime);
		cb.rocketHitWall(i, r->power, r->x, r->y, r->px, r->py);
		return;
	}
	r->move(advance);
}

void WorldBase::shootRockets(PhysicsCallbacksBase& cb, int playernum, int pow, int dir, NLubyte* rids, int frameAdvance,
																	int team, bool power, int px, int py, int x, int y) {
	struct RocketFormation {
		int nForward;
		int directions[6];
	};
	static const RocketFormation formations[9] = {
		{ 1, { } },
		{ 2, { } },
		{ 3, { } },
		{ 2, { -1, +1 } },
		{ 3, { -2, +2 } },
		{ 2, { -1, +1, -2, +2 } },
		{ 3, { -2, +2, -3, +3 } },
		{ 2, { -1, +1, -2, +2, -3, +3 } },
		{ 3, { -1, +1, -2, +2, -3, +3 } }
	};
	const RocketFormation& form = formations[pow-1];

	if (form.nForward == 1)
		addRocket(rids[0], playernum, team, px, py, x, y, power, dir,  0, frameAdvance, cb);
	else if (form.nForward == 2) {
		addRocket(rids[0], playernum, team, px, py, x, y, power, dir, -1, frameAdvance, cb);
		addRocket(rids[1], playernum, team, px, py, x, y, power, dir, +1, frameAdvance, cb);
	}
	else {
		addRocket(rids[0], playernum, team, px, py, x, y, power, dir,  0, frameAdvance, cb);
		addRocket(rids[1], playernum, team, px, py, x, y, power, dir, -2, frameAdvance, cb);
		addRocket(rids[2], playernum, team, px, py, x, y, power, dir, +2, frameAdvance, cb);
	}
	const int* dirp = &form.directions[0];
	for (int ri = form.nForward; ri < pow; ++ri, ++dirp)
		addRocket(rids[ri], playernum, team, px, py, x, y, power, dir + *dirp, 0, frameAdvance, cb);
}

void PowerupSettings::reset() {
	pup_add_time = 60;
	pup_max_time = 180;

	pups_min = 6;
	pups_min_percentage = false;
	pups_max = MAX_PICKUPS;
	pups_max_percentage = false;
	pups_respawn_time = 25;

	pup_chance_shield = 16;
	pup_chance_turbo = 14;
	pup_chance_shadow = 14;
	pup_chance_power = 14;
	pup_chance_weapon = 18;
	pup_chance_megahealth = 13;
	pup_chance_deathbringer = 11;

	pup_deathbringer_switch = true;
}

void PowerupSettings::print(LineReceiver& printer) const {
	ostringstream line;
	if (pup_max_time > pup_add_time)
		line << "- Power-ups add " << pup_add_time << " seconds to what's left, with a maximum of " << pup_max_time << " seconds";
	else
		line << "- Power-up time is " << pup_max_time << " seconds";
	printer(line.str());
	if (pup_deathbringer_switch) {
		line.str("");
		line << "- Picking up a second deathbringer power-up cancels the effect";
		printer(line.str());
	}
	line.str("");
	line << "- Base number of power-ups is " << pups_min; if (pups_min_percentage) line << '%';
	line << " and upper limit " << pups_max; if (pups_max_percentage) line << '%';
	if (pups_min_percentage || pups_max_percentage)
		line << " (% of map size)";
	printer(line.str());
}

int PowerupSettings::choose_powerup_kind() const {
	int max = pup_chance_shield + pup_chance_turbo + pup_chance_shadow + pup_chance_power
						+ pup_chance_weapon + pup_chance_megahealth + pup_chance_deathbringer;

	int chance = 1 + rand() % max;		//1..100 por exemplo se max = 100

	chance -= pup_chance_shield;
	if (chance <= 0) return 1;
	chance -= pup_chance_turbo;
	if (chance <= 0) return 2;
	chance -= pup_chance_shadow;
	if (chance <= 0) return 3;
	chance -= pup_chance_power;
	if (chance <= 0) return 4;
	chance -= pup_chance_weapon;
	if (chance <= 0) return 5;
	chance -= pup_chance_megahealth;
	if (chance <= 0) return 6;
	//chance -= pup_chance_deathbringer;
	return 7;
}

int PowerupSettings::pups_by_percent(int percentage, const Map& map) const {
	int result = (map.w*map.h*percentage+50) / 100;	// +50 to round properly
	if (result==0 && percentage>0)
		return 1;
	if (result>MAX_PICKUPS)
		return MAX_PICKUPS;
	return result;
}

#include "server.h"
//#fix: include needed for funny callback activities and SV_SHADOW_MINIMUM_NORMAL - get rid!

void WorldSettings::reset() {
	respawn_time = 2.0;
	waiting_time_deathbringer = 4.0;
	shadow_minimum = SV_SHADOW_MINIMUM_NORMAL;
	time_limit = 0;	// no time limit
	capture_limit = 8;
}

void WorldSettings::print(LineReceiver& printer) const {
	ostringstream line;
	line << "- Flag capture limit: " << capture_limit;
	printer(line.str());
	line.str("");
	if (time_limit == 0)
		line << "- No map time limit.";
	else
		line << "- Map time limit: " << time_limit / 10 / 60 << " min";
	printer(line.str());
	if (shadow_minimum == 1)
		printer("- A player using the shadow power-up gets totally invisible");
}

class ServerPhysicsCallbacks : public PhysicsCallbacksBase {
	ServerWorld& w;

public:
	ServerPhysicsCallbacks(ServerWorld& w_) : w(w_) { }

	bool collideToRockets() const { return true; }
	bool gatherMovementDistance() const { return true; }
	bool allowRoomChange() const { return true; }
	void addMovementDistance(int pid, float dist) { w.addMovementDistanceCallback(pid, dist); }
	void playerScreenChange(int pid) { w.playerScreenChangeCallback(pid); }
	void rocketHitWall(int rid, bool, float, float, int, int) { w.rocketHitWallCallback(rid); }
	bool rocketHitPlayer(int rid, int pid) { return w.rocketHitPlayerCallback(rid, pid); }
	void playerHitWall(int) { }
	void rocketOutOfBounds(int rid) { w.rocketOutOfBoundsCallback(rid); }
	bool shouldApplyPhysicsToPlayer(int pid) { return w.shouldApplyPhysicsToPlayerCallback(pid); }
};

void ServerWorld::reset() {
	// zero teamscores
	returnAllFlags();
	teams[0].clear();
	teams[1].clear();

	for (int i=0;i<maxplayers;i++)
		if (player[i].used) {
			//kill - to respawn
			player[i].respawn_to_base = true;
			resetPlayer(i);
			//zero score
			player[i].frags = 0;
		}

	//zero all rockets
	for (int i=0;i<MAX_ROCKETS;i++)
		rock[i].owner = -1;

	// remove and regenerate powerups
	for (int i=0;i<MAX_PICKUPS;i++)
		item[i].kind = 0;
	check_pickup_creation(true);

	map_start_time = frame;
}

void ServerWorld::printTimeStatus(LineReceiver& printer) {
	// server uptime
	unsigned long uptime = frame/10/60;	// minutes
	int days = uptime / 60 / 24;
	ostringstream server_time;
	server_time << "@IThe server has been up for ";
	if (days > 0)
		server_time << ' ' << days << " day" << (days > 1 ? "s " : " ");
	server_time << uptime / 60 % 24 << ':' << setfill('0') << setw(2) << uptime % 60;
	if (days == 0)
		server_time << " hours";
	server_time << '.';
	printer(server_time.str());
	// map time
	int seconds = getMapTime() / 10;
	ostringstream map_time;
	map_time << "@IMap time: " << seconds / 60 << ':' << setfill('0') << setw(2) << seconds % 60 << '.';
	if (config.getTimeLimit() == 0)
		map_time << " There is no time limit.";
	else {
		int remaining_seconds = getTimeLeft() / 10;
		// time limit not very useful when only one player
		int players = 0;
		for (int i = 0; i < maxplayers; i++)
			if (player[i].used)
				players++;
		if (players == 1)
			map_time << " No time limit at the moment as you are the only player.";
		else if (remaining_seconds < 0) // if time is out and game continues, it must be sudden death
			map_time << " Sudden death.";
		else {
			map_time << " Time left: " << remaining_seconds / 60 << ':';
			map_time << setfill('0') << setw(2) << remaining_seconds % 60 << '.';
		}
	}
	printer(map_time.str());
}

bool ServerWorld::load_map(const char *mapdir, const string& mapname) {
	map_start_time = frame;
	bool success = WorldBase::load_map(mapdir, mapname);
	for (int t = 0; t < 2; t++) {
		teams[t].remove_flags();
		for (vector<spoint_t>::const_iterator pi = map.tinfo[t].flags.begin(); pi != map.tinfo[t].flags.end(); ++pi)
			teams[t].add_flag(*pi);
	}
	return success;
}

void ServerWorld::returnAllFlags() {
	WorldBase::returnAllFlags();
	for (int i = 0; i < MAX_PLAYERS; i++)
		player[i].drop_flag();
	net->ctf_net_flag_status(-1, 0);
	net->ctf_net_flag_status(-1, 1);
}

void ServerWorld::returnFlag(int team, int flag) {
	WorldBase::returnFlag(team, flag);
	net->ctf_net_flag_status(-1, team);
}

void ServerWorld::dropFlag(int team, int flag, int roomx, int roomy, int lx, int ly) {
	WorldBase::dropFlag(team, flag, roomx, roomy, lx, ly);
	net->ctf_net_flag_status(-1, team);
}

void ServerWorld::stealFlag(int team, int flag, int carrier) {
	WorldBase::stealFlag(team, flag, carrier);
	net->ctf_net_flag_status(-1, team);
}

bool ServerWorld::dropFlagIfAny(int pid, bool purpose) {
	const int enemy = 1 - pid / TSIZE;
	if (!player[pid].flag())
		return false;
	int flag = 0;
	int i = 0;
	for (vector<Flag>::const_iterator fi = teams[enemy].flags().begin(); fi != teams[enemy].flags().end(); ++fi, ++i)
		if (fi->carrier() == pid) {
			flag = i;
			break;
		}
	net->bprintf("@I%s LOST THE %s FLAG!", player[pid].name.c_str(), teamname[enemy]);
	net->broadcast_sample(SAMPLE_CTF_LOST);
	net->broadcast_flag_drop(player[pid]);
	dropFlag(enemy, flag, player[pid].roomx, player[pid].roomy, (int)player[pid].lx, (int)player[pid].ly);
	player[pid].drop_flag();
	player[pid].stats().add_flag_drop();
	teams[pid / TSIZE].add_flag_drop();
	player[pid].total_flags_dropped++;
	if (purpose)
		player[pid].frags--;
	return true;
}

void ServerWorld::respawnPlayer(int pid) {
	player[pid].respawn_time = -1;
	int t = pid/TSIZE;	// team

	spoint_t pos;
	if (map.tinfo[t].spawn.empty())
		player[pid].respawn_to_base = false;
	else if (player[pid].respawn_to_base) {
		//choose a team spawn point
		if (++map.tinfo[t].lastspawn >= map.tinfo[t].spawn.size())
			map.tinfo[t].lastspawn = 0;
		pos = map.tinfo[t].spawn[ map.tinfo[t].lastspawn ];	// the point
	}

	//if was killed or map spawn point places player over a wall
	if (!player[pid].respawn_to_base || map.fall_on_wall(pos.px, pos.py, pos.x-20, pos.y-20, pos.x+20, pos.y+20)) {
		// generate a random spot for respawn:
		// - unnocupied screen
		// - away from walls

		//calculate room touch matrix
		vector<bool> roompop;
		roompop.resize(map.w*map.h, false);
		for (int i=0; i<maxplayers; i++)
			if (player[i].used && player[i].roomx >= 0 && player[i].roomy >= 0 && player[i].roomx < map.w && player[i].roomy < map.h)
				roompop[player[i].roomy * map.w + player[i].roomx] = true;

		int runaway = 400;
		do {
			//find screen
			int ridx;
			do {
				ridx = rand() % (map.w*map.h);
			} while ((runaway-- > 200) && (roompop[ridx] == true));	//keep trying until unnocupied (==false)
			pos.px = ridx%map.w;
			pos.py = ridx/map.w;

			//find a suitable coordinate -- middle square
			pos.x = plw / 8 + rand() % (3 * plw / 4);
			pos.y = plh / 8 + rand() % (3 * plh / 4);

			//do a check for walls, maybe retrying another screen if hits a wall
			if (!map.fall_on_wall(pos.px, pos.py, pos.x-20, pos.y-20, pos.x+20, pos.y+20))
				break;	//success!

			//fall on wall true, keep trying...

		} while (runaway-- > 0);

		if (runaway <= 0)
			net->broadcast_message("PLAYER SPAWN RUNAWAY");
	}

	//put player there
	//LOG("SPAWN %i %i  %i %i\n", pos.px, pos.py, pos.x, pos.y);
	player[pid].roomx = pos.px;	//screen
	player[pid].roomy = pos.py;
	player[pid].lx = pos.x;	//screen position
	player[pid].ly = pos.y;

	//reset speeds / z
	player[pid].sx = 0;
	player[pid].sy = 0;

	//reset player attributes
	player[pid].health = 100;
	player[pid].energy = 100;
	player[pid].megabonus = 0;  //balaca megahealth

	player[pid].weapon = 0;		//default weapon

	net->sendWeaponPower(pid);

	player[pid].item_shield = false;
	player[pid].item_quad = false;
	player[pid].item_speed = false;
	player[pid].visibility = 255;
	player[pid].item_deathbringer = false;
	player[pid].deathbringer_end = 0;

	player[pid].respawn_to_base = false;

	player[pid].last_spawn_time = (int)get_time();
	player[pid].dead = false;
	
	net->broadcast_spawn(player[pid]);

	//for all effects, player screen changed
	game_player_screen_change(pid);
}

//flag touched by player?
bool ServerWorld::check_flag_touch(const Flag& flag, int px, int py, int x, int y) {
	//carried and in different screen can't be touched
	if (flag.carried() || flag.position().px != px || flag.position().py != py)
		return false;

	const int fx = flag.position().x;
	const int fy = flag.position().y - 15;

	if (fx > x - (PLAYER_RADIUS + 15) &&
		fx < x + (PLAYER_RADIUS + 15) &&
		fy > y - (PLAYER_RADIUS + 15) &&
		fy < y + (PLAYER_RADIUS + 15))
			return true;	//touch
	return false;
}

void ServerWorld::respawn_pickup(int p) {
	item[p].kind = 0;

	//find a screen with no players and no other powerups
	int px, py, itemx, itemy, i;
	for (int runaway=300;; --runaway) {
		bool hit = false;
		px = rand() % map.w;
		py = rand() % map.h;

		//check for players if not tried a 100 times yet

		//check players
		if (runaway>200)
			for (i=0; i<maxplayers; i++)
				if (player[i].used && player[i].roomx==px && player[i].roomy==py) {
					hit = true;
					break;
				}
		if (hit)
			continue;

		//check for items if not tried 200 times yet

		//check items if no players found
		if (runaway>100)
			for (i=0;i<MAX_PICKUPS;i++)
				if (item[i].kind!=0 && item[i].px==px && item[i].py==py) {
					hit = true;
					break;
				}
		if (hit)
			continue;

		//find a suitable coordinate -- middle square
		itemx = plw / 8 + rand() % (3 * plw / 4);
		itemy = plh / 8 + rand() % (3 * plh / 4);

		//do a check for walls, maybe retrying another screen if hits a wall
		hit = map.fall_on_wall(px, py, itemx - 20, itemy - 20, itemx + 20, itemy + 20);
		if (!hit)
			break;
		if (--runaway < 0) {
			net->broadcast_message("ITEM SPAWN RUNAWAY");
			return;
		}
	}
	int kind = pupConfig.choose_powerup_kind();
	item[p].kind = (NLubyte)kind;
	item[p].px = px;
	item[p].py = py;
	item[p].x = itemx;	//copy from randomized position
	item[p].y = itemy;
	//screen-change message of players in the screen the powerup arrived
	//fixes "invisible powerup" problem, I hope
	for (i=0;i<maxplayers;i++)
		if (player[i].used && player[i].roomx==px && player[i].roomy==py)
			net->sendPickupVisible(i, p, item[p]);
}

// verifica powerups unused por jogadores presentes
void ServerWorld::check_pickup_creation(bool instant) {
	int i, pc, ic;

	//count number of players
	pc = 0;
	for (i=0;i<maxplayers;i++)
		if (player[i].used)
			pc++;

	//count number of items
	// TEST SERVER FUK : change "if" to :    if (player[i].used)
	ic = 0;
	for (i=0;i<MAX_PICKUPS;i++)
		if (item[i].kind != 0)	//0=unused 255=respawning 1..6(?)=spawned/kind
			ic++;

	int real_min = pupConfig.getMinPups(map);
	int real_max = pupConfig.getMaxPups(map);
	if (pc > real_min)
		real_min = pc;
	if (real_min > real_max)
		real_min = real_max;
	if (ic >= real_min)
		return;
	//while number of players > number of pickups: create a pickup and ic++
	for (i=0; i<MAX_PICKUPS; i++)
		if (item[i].kind == 0) {
			item[i].kind = 255;
			if (instant)
				respawn_pickup(i);
			else
				item[i].respawn_time = get_time() + pupConfig.getRespawnTime();
			if (++ic>=real_min)
				break;
		}
}

// player i touches a pickup p!
void ServerWorld::game_touch_pickup(int p, int pk) {
	pickup_c *it = &item[pk];

	//send "item removed" message to all players on the current screen
	//
	char lebuf[256]; int count = 0;
	writeByte(lebuf, count, data_pup_picked);
	writeByte(lebuf, count, (NLubyte)pk);	//what item id
	net->broadcast_screen_message(it->px, it->py, lebuf, count);

	switch (it->kind) {
		case 1: {	// shield
			player[p].item_shield = true;

			//increase health to minimum of 100
			if (player[p].health < 100)
				player[p].health = 100;		//full health

			//increase energy +100
			if (player[p].energy < 200) {
				player[p].energy += 100;
				if (player[p].energy > 200)
					player[p].energy = 200;
			}

			net->broadcast_screen_sample(p, SAMPLE_SHIELD_PICKUP);
			break;
		}
		case 2: {	// turbo
			double itemTime = player[p].item_speed_time-get_time();
			if (!player[p].item_speed || itemTime<0)
				itemTime = 0;
			itemTime = pupConfig.addTime(itemTime);

			player[p].item_speed = true;
			player[p].item_speed_time = get_time() + itemTime;

			net->sendPupTime(p, it->kind, itemTime);
			net->broadcast_screen_sample(p, SAMPLE_BOOTS_ON);
			break;
		}
		case 3:	{	// shadow
			double itemTime = player[p].item_helm_time - get_time();
			if (!player[p].item_helm() || itemTime < 0)
				itemTime = 0;
			itemTime = pupConfig.addTime(itemTime);

			player[p].visibility = 0;		// invisible
			player[p].item_helm_time = get_time() + itemTime;

			net->sendPupTime(p, it->kind, itemTime);
			net->broadcast_screen_sample(p, SAMPLE_HELM_ON);
			break;
		}
		case 4:	{	// power
			double itemTime = player[p].item_quad_time-get_time();
			if (!player[p].item_quad || itemTime<0)
				itemTime = 0;
			itemTime = pupConfig.addTime(itemTime);

			player[p].item_quad = true;
			player[p].item_quad_time = get_time() + itemTime;

			net->sendPupTime(p, it->kind, itemTime);
			net->broadcast_screen_sample(p, SAMPLE_QUAD_ON);
			break;
		}
		case 5:	{	// weapon
			if (player[p].weapon < 8)	// test for max (shots=weapon+1, entao p/ shots max 9, weapon max = 8)
				player[p].weapon++;	//increase weapon power

			if (player[p].energy < 200) {
				player[p].energy += 100;
				if (player[p].energy > 200)
					player[p].energy = 200;
			}

			net->sendWeaponPower(p);
			net->broadcast_screen_sample(p, SAMPLE_WEAPON_UP);
			break;
		}
		case 6:	{	// megahealth
			player[p].megabonus += 160;
			net->broadcast_screen_sample(p, SAMPLE_MEGAHEALTH);
			break;
		}
		case 7: {	// deathbringer
			if (pupConfig.getDeathbringerSwitch())
				player[p].item_deathbringer = !player[p].item_deathbringer;
			else
				player[p].item_deathbringer = true;

			net->broadcast_screen_sample(p, SAMPLE_GETDEATHBRINGER);
			break;
		}
	}

	// unused item
	it->kind = 0;

	// check pickup creation
	check_pickup_creation(false);
}

//game player screen changed
// --> send any pickups on screen
void ServerWorld::game_player_screen_change(int p) {
	//check for new pickups visible
	for (int i=0; i<MAX_PICKUPS; i++) {
		pickup_c *it = &item[i];
		if (it->kind && it->kind!=255 && it->px==player[p].roomx && it->py==player[p].roomy)
			net->sendPickupVisible(p, i, item[i]);
	}
}

void ServerWorld::resetPlayer(int target, float time_penalty) {	// take the player out of the game
	player[target].health = 0;

	player[target].visibility = 255;
	player[target].item_quad = false;
	player[target].item_speed = false;
	// deathbringer is not removed until respawn because the flag is needed

	//stop all speed
	player[target].sx = 0;
	player[target].sy = 0;

	dropFlagIfAny(target);
	player[target].respawn_time = get_time() + config.getRespawnTime() + time_penalty;
	if (!player[target].dead) {
		player[target].lifetime += (int)get_time() - player[target].last_spawn_time;
		player[target].dead = true;
	}
}

void ServerWorld::killPlayer(int target, bool time_penalty) {	// kill the player in the usual way with score penalties and deathbringer effect
	host->score_neg(target, 1);	// score neg points because of death
	if (dropFlagIfAny(target))
		host->score_neg(target, 1);	// score neg points because of losing the flag
	player[target].total_deaths++;
	if (++player[target].current_consecutive_deaths > player[target].most_consecutive_deaths)
		player[target].most_consecutive_deaths = player[target].current_consecutive_deaths;
	player[target].current_consecutive_kills = 0;

	if (player[target].item_deathbringer) {
		//record time to simulate the deathbringer explosion
		player[target].item_deathbringer_time = frame;
		net->sendDeathbringer(target, player[target]);
	}

	resetPlayer(target, (player[target].item_deathbringer || time_penalty)?config.getDeathbringerWaitingTime():0);
}

void ServerWorld::damagePlayer(int target, int attacker, int damage, bool deathbringer) {	// inflict normal or deathbringer damage on target
	//HELM powerup: show player
	if (player[target].item_helm())
		player[target].visibility = 254;

	if (player[target].item_shield) {
		player[target].energy -= damage;
		if (player[target].energy <= 0) {
			player[target].energy = 0;
			player[target].item_shield = false;
			if (!deathbringer)
				net->broadcast_screen_sample(target, SAMPLE_SHIELD_LOST);
		}
		else if (!deathbringer)
			net->broadcast_screen_sample(target, SAMPLE_SHIELD_DAMAGE);
	}
	//else do the regular body damage
	else {
		player[target].health -= damage;
		//freeze target's gun
		player[target].next_shoot_time = get_time() + 1.0;
	}
	if (player[target].health > 0)
		return;

	host->score_frag(attacker, 1);	//frag to attacker for the kill
	player[attacker].total_kills++;
	if (++player[attacker].current_consecutive_kills > player[attacker].most_consecutive_kills)
		player[attacker].most_consecutive_kills = player[attacker].current_consecutive_kills;
	player[attacker].current_consecutive_deaths = 0;

	const int tateam = target / TSIZE;
	const int atteam = attacker / TSIZE;

	//check if the enemy flag is carried in this screen(target's) by somebody that is not me
	for (vector<Flag>::const_iterator fi = teams[tateam].flags().begin(); fi != teams[tateam].flags().end(); ++fi)
		if (fi->carried()) {
			const int p = fi->carrier();
			if (player[p].used && p != attacker && player[p].roomx == player[target].roomx && player[p].roomy == player[target].roomy) {
				net->bprintf("@I%s DEFENDS THE %s CARRIER", player[attacker].name.c_str(), teamname[atteam]);
				host->score_frag(attacker, 1);
				break;	// only one message
			}
		}
	for (vector<Flag>::const_iterator fi = teams[atteam].flags().begin(); fi != teams[atteam].flags().end(); ++fi)
		if (!fi->carried() && fi->position().px == player[target].roomx && fi->position().py == player[target].roomy) {
			net->bprintf("@I%s DEFENDS THE %s FLAG", player[attacker].name.c_str(), teamname[atteam]);
			host->score_frag(attacker, 1);
			break;		// only one message
		}
	const bool flag = player[target].flag();
	/* ### REMOVE ###
	for (vector<Flag>::const_iterator fi = teams[atteam].flags().begin(); fi != teams[atteam].flags().end(); ++fi)
		if (fi->carrier() == target) {
			host->score_frag(attacker, 1);	// extra frag for fragging a carrier
			player[attacker].total_flag_carriers_killed++;
			flag = true;
		}*/
	if (flag) {
		host->score_frag(attacker, 1);	// extra frag for fragging a carrier
		player[attacker].total_flag_carriers_killed++;
	}

	if (deathbringer) {
		if (player[attacker].used)
			net->bprintf("@I%s was choked by %s", player[target].name.c_str(), player[attacker].name.c_str());
		else
			net->bprintf("@I%s was choked", player[target].name.c_str());
		net->broadcast_screen_sample(target, SAMPLE_DIEDEATHBRINGER);
	}
	else
		net->bprintf("@I%s was nailed by %s", player[target].name.c_str(), player[attacker].name.c_str());

	net->broadcast_kill(player[attacker], player[target], deathbringer, flag);

	player[attacker].stats().add_kill(deathbringer);
	teams[atteam].add_kill();
	player[target].stats().add_death(deathbringer, static_cast<int>(get_time()));
	teams[tateam].add_death();

	killPlayer(target, false);
}

//remove player from the game
void ServerWorld::removePlayer(int pid) {
	//remove all shots from this player
	for (int r=0; r<MAX_ROCKETS; r++)
		if (rock[r].owner == pid)
			deleteRocket(r, 0, 0, 255);

	dropFlagIfAny(pid);

	//erase player
	player[pid].delayedMessages.clear();
	player[pid].used = false;
}

void ServerWorld::suicide(int pid) {
	if (player[pid].health > 0) {
		bool flag = player[pid].flag();
		killPlayer(pid, true);
		player[pid].frags--;
		player[pid].total_suicides++;
		player[pid].stats().add_suicide(static_cast<int>(get_time()));
		teams[pid / TSIZE].add_suicide();
		net->broadcast_suicide(player[pid], flag);
	}
}

NLubyte ServerWorld::getFreeRocket() {
	for (int i=0; i<MAX_ROCKETS; i++)
		if (rock[i].owner == -1) {
			rock[i].owner = 0;
			return i;
		}
	LOG("Rocket overwrite!\n");
	int i = rand() % MAX_ROCKETS;
	rock[i].owner = 0;
	return i;
}

void ServerWorld::shootRockets(int pid, int shots) {
	int px = player[pid].roomx, py = player[pid].roomy, x = int(player[pid].lx), y = int(player[pid].ly);

	player[pid].total_shots++;
	player[pid].stats().add_shot();
	teams[pid / TSIZE].add_shot();

	NLubyte sid[16];
	for (int i = 0; i < shots; ++i)
		sid[i] = getFreeRocket();

	ServerPhysicsCallbacks cb(*this);
	WorldBase::shootRockets(cb, pid, shots, player[pid].gundir, sid, 0, pid/TSIZE, player[pid].item_quad, px, py, x, y);

	//build people-that-know DOUBLE WORD (32bits == 32players max)
	//send message to players on the same screen
	NLulong  vislist = 0;
	for (int p=0; p<maxplayers; p++)
		if (player[p].used && player[p].roomx==px && player[p].roomy==py)
			vislist |= (1 << p);

	//mark all created rockets with the vislist
	for (int k=0;k<shots;k++)
		rock[ sid[k] ].vislist = vislist;

	net->sendRocketMessage(shots, player[pid].gundir, sid, pid/TSIZE, player[pid].item_quad, px, py, x, y);
}

void ServerWorld::deleteRocket(int rid, NLshort hitx, NLshort hity, int targ) {
	rocket_c* r = &rock[rid];
	net->sendRocketDeletion(r->vislist, rid, hitx, hity, targ);
	r->owner = -1;
}

void ServerWorld::changeRocketsOwner(int source, int target) {
	for (int i = 0; i < MAX_ROCKETS; i++)
		if (rock[i].owner == source)
			rock[i].owner = target;
}

void ServerWorld::swapRocketOwners(int a, int b) {
	for (int i = 0; i < MAX_ROCKETS; i++) {
		if (rock[i].owner == a)
			rock[i].owner = b;
		else if (rock[i].owner == b)
			rock[i].owner = a;
	}
}

void ServerWorld::addMovementDistanceCallback(int pid, float dist) {
	player[pid].total_movement += dist;
	player[pid].stats().add_movement(dist);
}

void ServerWorld::playerScreenChangeCallback(int pid) {
	game_player_screen_change(pid);
}

void ServerWorld::rocketHitWallCallback(int rid) {
	rock[rid].owner = -1;
}

bool ServerWorld::rocketHitPlayerCallback(int rid, int pid) {
	//record wether the player had shield, if yes, will not blink him
	bool had_shield = player[pid].item_shield;

	//default damage to the target: 70
	int damage = 70;
//	if (player[rock[rid].owner].item_deathbringer)
//		damage = 50;
	if (rock[rid].power)
		damage *= 2;

	damagePlayer(pid, rock[rid].owner, damage, false);
	player[rock[rid].owner].total_hits++;
	player[pid].total_shots_taken++;

	player[rock[rid].owner].stats().add_hit();
	teams[rock[rid].team].add_hit();
	player[pid].stats().add_shot_take();

	//if player not dead, push him
	if (player[pid].health > 0) {
		player[pid].sx += rock[rid].sx * .33;
		player[pid].sy += rock[rid].sy * .33;
	}

	if (had_shield)
		deleteRocket(rid, (NLshort)rock[rid].x, (NLshort)rock[rid].y, 252);		//do not blink
	else
		deleteRocket(rid, (NLshort)rock[rid].x, (NLshort)rock[rid].y, pid);		//blink
	return player[pid].health<=0;
}

void ServerWorld::rocketOutOfBoundsCallback(int rid) {
	rock[rid].owner = -1;
}

bool ServerWorld::shouldApplyPhysicsToPlayerCallback(int pid) {
	return player[pid].health > 0;
}

void WorldBase::executeBounce(PlayerBase& ply, const BounceData& b, double plyRadius) {	// needs plyRadius as a shortcut to b.second's length
	// bounce: speed component parallel with bounceVec ( (S dot b / |b|) * b / |b| ) is reversed, while perpendicular component is kept
	// : S -= 2* ( (S dot b) * b / |b|^2 )	; |b| is always plyRadius
	const Coords& bounceVec = b.second;
	double mul = 2. * (ply.sx*bounceVec.first + ply.sy*bounceVec.second) / (plyRadius*plyRadius);
	ply.sx -= mul*bounceVec.first;
	ply.sy -= mul*bounceVec.second;
	// lose some speed too
	ply.sx *= .95;
	ply.sy *= .95;
}

void WorldBase::applyPhysics(PhysicsCallbacksBase& callback, double plyRadius, float fraction) {
	if (fraction < .001)
		return;

	// note: design decision: vector is used extensively instead of list, to provide access by index
	//       it shouldn't harm since the vectors are short and anything is rarely erased
	vector< vector< vector<int> > > roomPly, roomRock;	// player id's for players in room, rocket id's for rockets in room

	roomPly.resize(map.w);
	roomRock.resize(map.w);
	for (int rx=0; rx<map.w; ++rx) {
		roomPly[rx].resize(map.h);
		roomRock[rx].resize(map.h);
	}

	// add players and rockets to room structs for physics run
	for (int i=0; i<maxplayers; i++) {
		PlayerBase& pl = player[i];
		if (!pl.used)
			continue;
		if (callback.shouldApplyPhysicsToPlayer(i)) {
			if (pl.roomx<0 || pl.roomy<0 || pl.roomx>=map.w || pl.roomy>=map.h)
				continue;	//#fix: remove this and track why these are given sometimes
			applyPlayerAcceleration(i);
			roomPly[pl.roomx][pl.roomy].push_back(i);
		}
	}
	for (int i=0; i<MAX_ROCKETS; i++) {
		if (rock[i].owner == -1)
			continue;
		nAssert(rock[i].px>=0 && rock[i].py>=0 && rock[i].px<map.w && rock[i].py<map.h);
		roomRock[rock[i].px][rock[i].py].push_back(i);
	}

	// apply physics to each room separately
	for (int rx=0; rx<map.w; ++rx)
		for (int ry=0; ry<map.h; ++ry)
			applyPhysicsToRoom(map.room[rx][ry], roomPly[rx][ry], roomRock[rx][ry], callback, plyRadius, fraction);
}

void WorldBase::applyPhysicsToRoom(const Room& room, vector<int>& rply, vector<int>& rrock, PhysicsCallbacksBase& callback, double plyRadius, float fraction) {
	vector<BounceData> plyMoveMax;	// plyMoveMax changes when player bounces
	vector<double> rockMoveMax;	// rockMoveMax is fixed

	typedef unsigned int uint;	// for loop counters, to disable the brainless 'signed vs unsigned comparison' warning by G++

	for (vector<int>::const_iterator pi=rply.begin(); pi!=rply.end(); ++pi)
{nAssert(*pi>=0 && *pi<maxplayers);//#t
		plyMoveMax.push_back(getTimeTillBounce(room, player[*pi], plyRadius, fraction));
}
	for (vector<int>::const_iterator ri=rrock.begin(); ri!=rrock.end(); ++ri)
		rockMoveMax.push_back(getTimeTillWall(room, rock[*ri], fraction));

	double subFrame = 0.;	// signifies current time within frame, goes from 0 to fraction (0 <= fraction <= 1)
	#ifndef NDEBUG
	int round = 0;
	#endif
	for (;;) {	//#fix: optimize this loop, esp. for client
		nAssert(++round < 200);
		// find out next player-wall collision
		double minBounce = fraction + 1;	// at what time the first player bounces (absolute frame time: 1 is end of frame)
		int bPly=0, bPlyI=-1;	// which player it is, pid and room-table-index
		for (uint pi=0; pi<rply.size(); ++pi) {
			double bt = plyMoveMax[pi].first;
			if (bt < minBounce) {
				minBounce = bt;
				bPlyI = pi;
				bPly = rply[pi];
			}
		}
		nAssert(minBounce > subFrame - .001);

		// find out next player-rocket collision
		double minCollision = fraction + 1;	// at what time the first player-rocket collision occurs (forward time: 1-subFrame is end of frame)
		int cPly=0, cPlyI=-1, cRock=0, cRockI=-1;	// which player and rocket they are, pid/rid and room-table-indices
		if (callback.collideToRockets()) {
			for (uint pi=0; pi<rply.size(); ++pi) {
				int pid = rply[pi];
				for (uint ri=0; ri<rrock.size(); ++ri) {
					int rid = rrock[ri];
					if (rock[rid].team == pid/TSIZE)	// friendly rocket
						continue;
					double time = getTimeTillCollision(player[pid], rock[rid], ROCKET_RADIUS + static_cast<PlayerBase&>(player[pid]).item_shield?SHIELD_RADIUS:plyRadius);
					if (time < minCollision && time < rockMoveMax[ri]) {
						minCollision = time;
						cPlyI = pi;
						cPly = rply[pi];
						cRockI = ri;
						cRock = rrock[ri];
					}
				}
			}
			nAssert(minCollision >= 0.);
			minCollision += subFrame;	// it was calculated in forward time, now it's in absolute frame time as are player movements
		}

		// execute movement
		double mt = min<double>(fraction, min<double>(minCollision, minBounce + .01));	// time of the next event (add .01 to minBounce to not bounce infinitely)
		nAssert(mt >= subFrame-.0001);
		for (int pi = 0; pi < static_cast<int>(rply.size()); ) {
			// don't move more than mt or more than plyMoveMax-.001 but don't move backwards (-.001 to stay out of walls)
			double amount = bound<double>(plyMoveMax[pi].first - .001, subFrame, mt);
			PlayerBase& pl = player[rply[pi]];
			pl.move(amount - subFrame);
			if (callback.gatherMovementDistance())
				callback.addMovementDistance(rply[pi], (amount - subFrame) * sqrt( pl.sx*pl.sx + pl.sy*pl.sy ));
			bool rch = false;
			if (callback.allowRoomChange()) {
				if (pl.lx < 0)   { nAssert(pl.sx != 0); pl.ly -=  pl.lx     *pl.sy/pl.sx; pl.lx = plw; rch = true; if (--pl.roomx <      0) pl.roomx = map.w - 1; }
				if (pl.lx > plw) { nAssert(pl.sx != 0); pl.ly -= (pl.lx-plw)*pl.sy/pl.sx; pl.lx =   0; rch = true; if (++pl.roomx >= map.w) pl.roomx =         0; }
				if (pl.ly < 0)   { nAssert(pl.sy != 0); pl.lx -=  pl.ly     *pl.sx/pl.sy; pl.ly = plh; rch = true; if (--pl.roomy <      0) pl.roomy = map.h - 1; }
				if (pl.ly > plh) { nAssert(pl.sy != 0); pl.lx -= (pl.ly-plh)*pl.sx/pl.sy; pl.ly =   0; rch = true; if (++pl.roomy >= map.h) pl.roomy =         0; }
			}
			if (rch) {
				callback.playerScreenChange(rply[pi]);
				rply.erase(rply.begin() + pi);
				plyMoveMax.erase(plyMoveMax.begin() + pi);
				if (bPlyI == pi)
					bPlyI = -1;
				else if (bPlyI > pi)
					--bPlyI;
				if (cPlyI == pi)
					cPlyI = -1;
				else if (cPlyI > pi)
					--cPlyI;
				// continue with the same index (which points to the next player now)
			}
			else
				++pi;
		}
		for (int ri = 0; ri < static_cast<int>(rrock.size()); ) {
			rocket_c& r = rock[rrock[ri]];
			if (mt > rockMoveMax[ri]) {
				r.move(rockMoveMax[ri] - subFrame);
				callback.rocketHitWall(rrock[ri], r.power, r.x, r.y, r.px, r.py);
				rrock.erase(rrock.begin() + ri);
				rockMoveMax.erase(rockMoveMax.begin() + ri);
				if (cRockI == ri)
					cRockI = -1;
				else if (cRockI > ri)
					--cRockI;
				// continue with the same index (which points to the next rocket now)
			}
			else {
				r.move(mt - subFrame);
				++ri;
			}
		}
		subFrame = mt;
		if (subFrame > fraction-.001)
			break;

		// execute collision or bounce
		int plyChanged;
		if (minCollision < minBounce) {	// the event is a collision
			if (cRockI != -1 && cPlyI != -1) {
				nAssert(cRockI < static_cast<int>(rrock.size()));
				nAssert(cPlyI < static_cast<int>(rply.size()));
				if (callback.rocketHitPlayer(cRock, cPly)) {	// true if player is dead
					rply.erase(rply.begin() + cPlyI);
					plyMoveMax.erase(plyMoveMax.begin() + cPlyI);
					plyChanged = -1;	// no player needs recalculation since the changed player was removed
				}
				else
					plyChanged = cPlyI;
				rrock.erase(rrock.begin() + cRockI);
				rockMoveMax.erase(rockMoveMax.begin() + cRockI);
			}
			else
				plyChanged = -1;
		}
		else {	// the event is a bounce
			if (bPlyI != -1) {
				nAssert(bPlyI < static_cast<int>(rply.size()));
				executeBounce(player[bPly], plyMoveMax[bPlyI], plyRadius);
				callback.playerHitWall(bPly);
				plyChanged = bPlyI;
			}
			else
				plyChanged = -1;
		}
		if (plyChanged != -1) {
			plyMoveMax[plyChanged] = getTimeTillBounce(room, player[rply[plyChanged]], plyRadius, fraction - subFrame);
			plyMoveMax[plyChanged].first += subFrame;	// keep the table in absolute frame time
		}
	}
	for (vector<int>::const_iterator ri=rrock.begin(); ri!=rrock.end(); ++ri) {
		const rocket_c& r = rock[*ri];
		if (r.x<0 || r.x>plw || r.y<0 || r.y>plh)
			callback.rocketOutOfBounds(*ri);	// don't bother with removing it from rrock since the simulation is over
	}
}

void WorldBase::rocketFrameAdvance(int frames, PhysicsCallbacksBase& callback) {
	for (int i = 0; i < MAX_ROCKETS; ++i)
		if (rock[i].owner != -1) {
			double wallTime = getTimeTillWall(map.room[rock[i].px][rock[i].py], rock[i], frames);
			if (wallTime < frames) {
				rock[i].move(wallTime);
				callback.rocketHitWall(i, rock[i].power, rock[i].x, rock[i].y, rock[i].px, rock[i].py);
			}
			else
				rock[i].move(frames);
		}
}

void ServerWorld::simulateFrame() {
	// (-1) check powerup respawn
	double thetime = get_time();
	for (int i=0;i<MAX_PICKUPS;i++)
		if (item[i].kind == 255 && thetime > item[i].respawn_time)
			respawn_pickup(i);

	// (0) do stuff for every player
	for (int i=0;i<maxplayers;i++) {
		if (!player[i].used)
			continue;

		//dec talk flood protect counter
		player[i].talk_temp -= 0.1;
		if (player[i].talk_temp < 0.0)
			player[i].talk_temp = 0.0;
		player[i].talk_hotness -= 0.1;
		if (player[i].talk_hotness < 1.0)
			player[i].talk_hotness = 1.0;

		//check frags changed
		if (player[i].oldfrags != player[i].frags) {
			//updated
			player[i].oldfrags = player[i].frags;
			net->sendFragUpdate(i, player[i].frags);
		}

		// check powerups expired
		if (player[i].item_speed)
			if (get_time() > player[i].item_speed_time) {
				player[i].item_speed = false;
				net->broadcast_screen_sample(i, SAMPLE_BOOTS_OFF);
			}
		if (player[i].item_quad)
			if (get_time() > player[i].item_quad_time) {
				player[i].item_quad = false;
				net->broadcast_screen_sample(i, SAMPLE_QUAD_OFF);
			}
		if (player[i].item_helm())
			if (get_time() > player[i].item_helm_time) {
				player[i].visibility = 255;
				net->broadcast_screen_sample(i, SAMPLE_HELM_OFF);
			}

		// helm alpha down
		if (player[i].item_helm()) {
			player[i].visibility -= 10;		//slowly fades....
			if (player[i].visibility < config.getShadowMinimum())	// minimum
				player[i].visibility = config.getShadowMinimum();
		}

		// check deathbringer effect
		//
		if (player[i].deathbringer_end > get_time()) {
			//check if still alive
			if (player[i].health > 0) {
				//has shield: do big damage to it, in order to remove the shield
				if (player[i].item_shield)
					damagePlayer(i, player[i].deathbringer_attacker, 12, true);
				else
					damagePlayer(i, player[i].deathbringer_attacker, 3, true); // 30 / s, 150 / 5 s
			}
		}

		// check for a player's deathbringer to bring death
		//
		if (player[i].dead && player[i].item_deathbringer) {
			//delta time since shoot
			double delta = (frame - player[i].item_deathbringer_time) * 0.1;
			//figure out new radius
			int rad;
			if (delta < 1.0)
				rad = (int)(delta * 100);
			else
				rad = 100 + (int)((delta - 1.0) * (delta - 1.0) * 800);

			//check enemy players onscreen that are not hit by it yet and are inside
			// the donut radius...radius-50
			for (int v=0;v<maxplayers;v++)
			if (v/TSIZE != i/TSIZE)		//enemy players only
			if (player[v].used)	//used
			if (player[v].health > 0)	//alive
			if (player[v].roomx == player[i].roomx)	// in the same screen of the deathbringer
			if (player[v].roomy == player[i].roomy)
			if (player[v].deathbringer_end < get_time())		// deathbringer fx end time -- not already hit?
			{
				//calculate player distance to the deathbringer core
				double ex = player[i].lx;
				double ey = player[i].ly;
				double rx = player[v].lx;
				double ry = player[v].ly;
				double dt = sqrt( (ex - rx)*(ex - rx) + (ey - ry)*(ey - ry) );

				// hit distance: if dt == rad, hit, if rad
				if ((rad <= dt + 20) && (rad >= dt - 60)) {
					player[v].item_deathbringer = false;
					net->broadcast_screen_sample(v, SAMPLE_HITDEATHBRINGER);
					player[v].deathbringer_attacker = i;
					// time of effect ; also freeze his gun for this same amount of time
					player[v].deathbringer_end = player[v].next_shoot_time = get_time() + 4.5 + ((double)(rand() % 1000) / 1000.0);

					// calc recoil:
					double tx = player[v].lx - player[i].lx;
					double ty = player[v].ly - player[i].ly;

					double div = sqrt( tx*tx + ty*ty );
					if (div != 0) {
						double mul = 40. / div;	// set speed to 40
						player[v].sx = tx * mul;
						player[v].sy = ty * mul;
					}
				}
			}
		}

		// check for player weapons fire time
		//
		if (player[i].attack)	// player holding attack button
		if (player[i].health > 0)		// check if player alive
		if (get_time() > player[i].next_shoot_time)  // check if time allowed to fire again
		{
			//gasta 7 + 2 * tiros energy, se tem energy
			int numshots = 1;
			player[i].energy -= 7;			//gasta normal
			if (player[i].energy < 0)	//se ficou menor que zero, atira 1 so
				player[i].energy = 0;
			else {
				for (int k=1;k<player[i].weapon+1;k++) {
					//try add one shot
					player[i].energy -= 1;		//v0.4.7: diminuí METADE do gasto per shot!
					if (player[i].energy < 0)
						player[i].energy = 0;
					else
						numshots++;
				}
			}

			player[i].next_shoot_time = get_time() + 0.5;		// add minimum interval (in secs)

			//show helm
			if (player[i].item_helm())
				player[i].visibility = 254;

			shootRockets(i, numshots);
		}
	}

	ServerPhysicsCallbacks cb(*this);
	applyPhysics(cb, PLAYER_RADIUS, 1.);	// 1. means apply the whole frame at once

	// for each player, do misc stuff
	for (int i=0;i<maxplayers;i++) {
		if (!player[i].used)
			continue;

		ServerPlayer* h = &(player[i]);

		//check if dead/respawn
		if (player[i].health <= 0) {
			if (player[i].respawn_time < get_time())
				respawnPlayer(i);		//time to respawn player
			else
				continue;
		}

		// check don't regen because of deathbringer
		//v0.4.0: do not regen if has deathbringer and both health/energy are at no less than 100
		bool deathbringer_penalty =
				((player[i].item_deathbringer) && (player[i].health >= 100) && (player[i].energy >= 100))			//rand() % 100 < 50
				||
				(player[i].deathbringer_end > get_time());

		// regen?
		if (!deathbringer_penalty) {
			// regenerate +1 health or +1 energy
			if (player[i].health < 100)
				player[i].health++;
			else {
				//caso energy > 100, regenera mais devagar (-33%)
				if (player[i].energy < 100)
					player[i].energy++;
				else if (player[i].energy < 200) {
					if (frame % 2)
						player[i].energy++;
				}
				//MEGA health vagarosamente...
				else if ((player[i].health < 200) && (frame % 10 == 0))
					player[i].health++;
			}
		}
		//lose health & energy if running
		if (h->controls.isRun()) {
			if (player[i].energy <= 0) {
				//if (!player[i].item_speed)	// se ta com SPEED, faz nao hurt
				if (player[i].health > MIN_HEALTH_FOR_RUN_PENALTY) {	// se health > 30, desconta
					if (frame % 2 == 0)
						player[i].health -= 2;	//desconta 2 (o normal)
					else
						player[i].health -= 1;	//desconta 1 (menos)
					if (player[i].health < MIN_HEALTH_FOR_RUN_PENALTY)		// garante minimo 30
						player[i].health = MIN_HEALTH_FOR_RUN_PENALTY;
				}
			} else {
				if (frame % 2 == 0)
					player[i].energy -= 2; //desconta 2 (o normal)
				else
					player[i].energy -= 1; //desconta 1 (menos)
				if (player[i].energy == -1) { // special case
					player[i].energy++;
					if (player[i].health > MIN_HEALTH_FOR_RUN_PENALTY) {	// se health > 30, desconta
						player[i].health--;
						if (player[i].health < MIN_HEALTH_FOR_RUN_PENALTY)		// garante minimo 30
							player[i].health = MIN_HEALTH_FOR_RUN_PENALTY;
					}
				}
			}
		}
		//rot health to 100 if has deathbringer
		if ((player[i].item_deathbringer) && (player[i].health > 100) && (frame % 4 == 0))
			player[i].health--;
		//rot energy to 100 if has deathbringer
		if ((player[i].item_deathbringer) && (player[i].energy > 100) && (frame % 4 == 0))
			player[i].energy--;
		//megahealth bonus:
		if (player[i].megabonus > 0)
		if ((player[i].health == 300) && (player[i].energy == 300))
			player[i].megabonus--;	// realiza um certo "guardamento" de energia mas nao muito...
		else
			for (int mh=0;mh<5;mh++) {
				if (player[i].megabonus > 0 && player[i].health < 300) {
					player[i].health++;
					player[i].megabonus--;
				}
				if (player[i].megabonus > 0 && player[i].energy < 300) {
					player[i].energy++;
					player[i].megabonus--;
				}
			}
		// new limit - don't store megabonuses
		if (player[i].health == 300 && player[i].energy == 300)
			player[i].megabonus = 0;

		//limit health 0 .. 300
		if (player[i].health < 0)
			player[i].health = 0;
		else if (player[i].health > 300)
			player[i].health = 300;

		//limit energy 0 .. 300
		if (player[i].energy < 0)
			player[i].energy = 0;
		else if (player[i].energy > 300)
			player[i].energy = 300;

		//---------------------------------
		// check game object collisions
		//---------------------------------

		int myteam = i/TSIZE;
		int enemyteam = 1 - myteam;

		// --> ITEM PICKUP
		//
		int prad = 10;	//pickup item radius

		for (int k=0;k<MAX_PICKUPS;k++)
			if (item[k].kind > 0)	//valid item
			if (item[k].kind != 255) // not respawning
			if (item[k].px == player[i].roomx)		// player's screen
			if (item[k].py == player[i].roomy)
			//x,y == center of powerup!
			if (item[k].x + prad > player[i].lx - PLAYER_RADIUS)
			if (item[k].x - prad < player[i].lx + PLAYER_RADIUS)
			if (item[k].y + prad > player[i].ly - PLAYER_RADIUS)
			if (item[k].y - prad < player[i].ly + PLAYER_RADIUS)
			{
				//pick pickup
				game_touch_pickup(i, k);		//COOL!
			}

		/* ### REMOVE ###
		bool already_carrying = false;
		for (vector<Flag>::const_iterator fi = teams[enemyteam].flags().begin(); fi != teams[enemyteam].flags().end(); ++fi)
			if (fi->carrier() == i) {
				already_carrying = true;
				break;
			}*/
		// --> CTF FLAG STEAL touch other team's flag
		// enemy flag dropped (at base or somewhere)
		bool touches_flag = false;
		//if (!already_carrying) {
		if (!player[i].flag()) {
			int f = 0;
			for (vector<Flag>::const_iterator fi = teams[enemyteam].flags().begin(); fi != teams[enemyteam].flags().end(); ++fi, ++f)
				if (!fi->carried() && check_flag_touch(*fi, player[i].roomx, player[i].roomy, (int)h->lx, (int)h->ly)) {
					touches_flag = true;
					// Has player just dropped the flag or not?
					if (!player[i].dropped_flag && !player[i].drop_key) {
						//FLAG STOLEN!
						host->score_frag(i, 1);	// just add some frags
						player[i].total_flags_taken++;
						player[i].stats().add_flag_take();
						teams[myteam].add_flag_take();
						net->bprintf("@I%s GOT THE %s FLAG!", player[i].name.c_str(), teamname[enemyteam]);
						net->broadcast_flag_take(player[i]);
						stealFlag(enemyteam, f, i);  //flag stolen!
						player[i].take_flag();
						//HELM powerup: show player
						if (player[i].item_helm())
							player[i].visibility = 254;
						break;	// only take one flag
					}
				}
		}
		if (!player[i].drop_key && !touches_flag)
			player[i].dropped_flag = false;
		// --> CTF FLAG RETURN
		int f = 0;
		for (vector<Flag>::const_iterator fi = teams[myteam].flags().begin(); fi != teams[myteam].flags().end(); ++fi, ++f)
			if (!fi->carried() && !fi->at_base() && check_flag_touch(*fi, player[i].roomx, player[i].roomy, (int)h->lx, (int)h->ly)) {
				//FLAG RETURNED!
				host->score_frag(i, 1);	// just add some frags
				player[i].total_flags_returned++;
				player[i].stats().add_flag_return();
				teams[myteam].add_flag_return();
				net->bprintf("@I%s RETURNED THE %s FLAG!", player[i].name.c_str(), teamname[myteam]);
				net->broadcast_flag_return(player[i]);
				returnFlag(myteam, f);  //flag returned
				net->broadcast_sample(SAMPLE_CTF_RETURN);
			}

		// --> CTF FLAG CAPTURE
		for (vector<Flag>::const_iterator fmy = teams[myteam].flags().begin(); fmy != teams[myteam].flags().end(); ++fmy) {
			int f = 0;
			for (vector<Flag>::const_iterator fen = teams[enemyteam].flags().begin(); fen != teams[enemyteam].flags().end(); ++fen, ++f)
				if (fen->carrier() == i && fmy->at_base() && check_flag_touch(*fmy, player[i].roomx, player[i].roomy, (int)h->lx, (int)h->ly)) {
					//v0.4.7: detect degenerated maps
					if (map.valid_for_scoring && get_time() - fen->grab_time() <= MINIMUM_GRAB_TO_CAPTURE_TIME) {
						//this map is bogus, ignore all scoring for it.
						map.valid_for_scoring = false;
						net->broadcast_message("@WThis map is too small. Scoring for World Ranking disabled.");
						host->clearWorldRankingDeltas();
					}
					//add frags to all players of the team
					// V0.4.8: PENALIZE every player of the other team
					for (int h = 0; h < MAX_PLAYERS; h++)
						if (player[h].used) {
							if (h / TSIZE == myteam)
								host->score_frag(h, 2);				//small two-frag bonus
							else
								host->score_neg(h, 1);		//v0.4.8 : small NEG POINT penalty for YOUR FLAG BEING CAPTURED
						}
					host->score_frag(i, 3);
					player[i].total_captures++;
					player[i].stats().add_capture();
					player[i].drop_flag();
					teams[myteam].add_score();
					returnFlag(enemyteam, f);

					string one_more;
					if (teams[myteam].score() == config.getCaptureLimit() - 1)
						one_more = " One more to win!";
					net->bprintf("@I%s CAPTURED THE %s FLAG!%s", player[i].name.c_str(), teamname[enemyteam], one_more.c_str());
					net->broadcast_capture(player[i]);

					net->ctf_update_teamscore(myteam);		//this function can decide to restart the game . (?)
					net->broadcast_sample(SAMPLE_CTF_CAPTURE);
					if (teams[myteam].score() >= config.getCaptureLimit()) {
						host->server_next_map(NEXTMAP_CAPTURE_LIMIT);	// ignore return value
						host->ctf_game_restart();
					}
				}
		}
	}

	// check timelimit
	int players = 0;
	for (int i=0; i<maxplayers; ++i)
		if (player[i].used)
			++players;
	NLulong time_limit = config.getTimeLimit();
	if (players > 1 && time_limit > 0) {
		int timeLeft = getTimeLeft();
		if      (time_limit >= 10*60 * 10 && timeLeft == 5*60 * 10)
			net->bprintf("@I*** Five minutes left in the game");
		else if (time_limit >=  2*60 * 10 && timeLeft ==   60 * 10)
			net->bprintf("@I*** One minute left in the game");
		else if (time_limit >=    60 * 10 && timeLeft ==   30 * 10)
			net->bprintf("@I*** 30 seconds left in the game");
		else if (timeLeft <= 0) {
			net->bprintf("@I*** Time out - CTF game over");
			host->server_next_map(NEXTMAP_CAPTURE_LIMIT);	// ignore return value
			host->ctf_game_restart();
		}
	}
}

// extrapolate : advances from source, a frame per every ctrl listed except the last one which gets subFrameAfter, controls are for player me
void ClientWorld::extrapolate(ClientWorld& source, PhysicsCallbacksBase& physCallbacks, int me,
						ClientControls* ctrlTab, NLubyte ctrlFirst, NLubyte ctrlLast, float subFrameAfter) {
	if (source.skipped) {
		skipped = true;
		return;
	}
	nAssert(source.frame > 0);

	frame = source.frame;

	for (int i=0; i<2; ++i)
		teams[i] = source.teams[i];

	for (int i=0; i<maxplayers; i++) {
		if (source.player[i].onscreen)
			player[i] = source.player[i];
		else
			player[i].used = false;
	}
	for (int i=0; i<MAX_ROCKETS; i++) {
		if (source.rock[i].owner == -1)
			rock[i].owner = -1;
		else
			rock[i] = source.rock[i];
	}

	for (NLubyte ctrli = ctrlFirst; ctrli != ctrlLast; ++ctrli) {	// note: it is OK to wrap around in the middle of the sequence
		player[me].controls = ctrlTab[ctrli];
		applyPhysics(physCallbacks, PLAYER_RADIUS-1., 1.);	// 1 is full frame
		++frame;
	}
	if (ctrlFirst == ctrlLast)	// no other data in this case
		player[me].controls = ctrlTab[ctrlLast];
	applyPhysics(physCallbacks, PLAYER_RADIUS-1., subFrameAfter);
	frame += subFrameAfter;
	// PLAYER_RADIUS-1. is used to counter problems in bouncing caused by inaccurate positions over network
}

// Team

Team::Team() {
	clear();
}

void Team::clear() {
	points = 0;
	total_kills = 0;
	total_deaths = 0;
	total_suicides = 0;
	total_flags_taken = 0;
	total_flags_dropped = 0;
	total_flags_returned = 0;
	total_shots = 0;
	total_hits = 0;
}

void Team::add_flag(const spoint_t& pos) {
	team_flags.push_back(Flag(pos));
}

void Team::remove_flags() {
	team_flags.clear();
}

void Team::steal_flag(int n, int carrier) {
	team_flags[n].take(carrier);
}

void Team::steal_flag(int n, int carrier, double time) {
	team_flags[n].take(carrier, time);
}

void Team::return_all_flags() {
	for (vector<Flag>::iterator fi = team_flags.begin(); fi != team_flags.end(); ++fi)
		fi->return_to_base();
}

void Team::return_flag(int n) {
	team_flags[n].return_to_base();
}

void Team::drop_flag(int n, const spoint_t& pos) {
	team_flags[n].move(pos);
	team_flags[n].drop();
}

void Team::move_flag(int n, const spoint_t& pos) {
	team_flags[n].move(pos);
}

// Flag

Flag::Flag(const spoint_t& pos_):
	status(status_at_base),
	carrier_id(-1),
	home_pos(pos_),
	pos(pos_)
{ }

void Flag::take(int carr) {
	status = status_carried;
	carrier_id = carr;
}

void Flag::take(int carr, double time) {
	if (status == status_at_base)
		grab_t = time;
	status = status_carried;
	carrier_id = carr;
}

void Flag::return_to_base() {
	status = status_at_base;
	pos = home_pos;
	carrier_id = -1;
}

void Flag::drop() {
	status = status_dropped;
	carrier_id = -1;
}

// Statistics

Statistics::Statistics():
	total_kills(0),
	total_deaths(0),
	total_deathbringer_kills(0),
	total_deathbringer_deaths(0),
	most_consecutive_kills(0),
	current_consecutive_kills(0),
	most_consecutive_deaths(0),
	current_consecutive_deaths(0),
	total_suicides(0),
	total_captures(0),
	total_flags_taken(0),
	total_flags_dropped(0),
	total_flags_returned(0),
	total_flag_carriers_killed(0),
	total_shots(0),
	total_hits(0),
	total_shots_taken(0),
	last_spawn_time(0),
	total_lifetime(0),
	total_movement(0),
	starttime(0),
	dead(false)
{ }

void Statistics::add_kill(bool deathbringer) {
	++total_kills;
	most_consecutive_kills = max(most_consecutive_kills, ++current_consecutive_kills);
	current_consecutive_deaths = 0;
	if (deathbringer)
		++total_deathbringer_kills;
}

void Statistics::add_death(bool deathbringer, int time) {
	++total_deaths;
	most_consecutive_deaths = max(most_consecutive_deaths, ++current_consecutive_deaths);
	current_consecutive_kills = 0;
	if (deathbringer)
		++total_deathbringer_deaths;
	dead = true;
	total_lifetime += time - last_spawn_time;
}

void Statistics::add_suicide(int time) {
	add_death(false, time);
	++total_suicides;
}

float Statistics::accuracy() const {
	if (total_shots == 0)
		return 0;
	else
		return static_cast<float>(total_hits) / total_shots;
}

int Statistics::lifetime(int time) const {
	if (dead)
		return total_lifetime;
	else
		return total_lifetime + time - last_spawn_time;
}

int Statistics::average_lifetime(int time) const {
	return lifetime(time) / (total_deaths + 1);
}

int Statistics::playtime(int time) const {
	return time - starttime;
}

double Statistics::movement() const {
	return total_movement;
}

float Statistics::speed(int time) const {
	return movement() / lifetime(time) / PLAYER_RADIUS / 2.;
}

