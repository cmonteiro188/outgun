#include "commont.h"
#include "world.h"

/* subIntersection:
 * returns true if the area between lines (lx1,ly1)-(lx2,ly2) and (rx1,ry1)-(rx2,ry2) intersects the rectangle (rectx1,recty1)-(rectx2,recty2)
 * every line must be y-ordered : ly1<=ly2, ry1<=ry2, recty1<=recty2 ; additionally rectx1<=rectx2 and lx(y)<=rx(y) for all applicable y
 *
 * how it works:
 * for the appropriate range of y, if there exists an y so that lx(y)<=rectx2 AND rx(y)>=rectx1 , there is an intersection in that range
 * those ranges are solved with simple linear equasions since lx and rx are linear
 */
bool subIntersection(float lx1, float ly1,  float lx2, float ly2,  float rx1, float ry1,  float rx2, float ry2,
				float rectx1, float recty1, float rectx2, float recty2) {
	assert(ly1<=ly2 && ry1<=ry2);
	float miny = max(max(ly1, ry1), recty1), maxy=min(min(ly2, ry2), recty2);
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
		float intersect_y = (rectx2 - lx1) * (ly2 - ly1) / (lx2 - lx1) + ly1;
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
		float intersect_y = (rectx1 - rx1) * (ry2 - ry1) / (rx2 - rx1) + ry1;
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

bool TriWall::intersects_rect(float rx1, float ry1, float rx2, float ry2) const {
	assert(ry1<=ry2 && rx1<=rx2);
	assert(y1<=y2 && y2<=y3);
	if (rx1>boundx2 || rx2<boundx1 || ry1>boundy2 || ry2<boundy1)
		return false;
	/* idea: triangle is split in two triangles: y<=y2 and y>=y2
	 * for both parts, the right and left edge are checked separately
	 * for each edge there may be a region [yi0, yi1] where (for a right side edge) xr(y)>=x1
	 * if those regions overlap with each other and [ry1, ry2], there exists an intersection
	 */
	if (x2 < x1 + (y2-y1) * (x3-x1) / (y3-y1)) {	// p2 is left to the p1-p3 line
		if (subIntersection(x1,y1, x2,y2, x1,y1, x3,y3, rx1, ry1, rx2, ry2))	// part y<=y2 : L,R sides p1-p2, p1-p3
			return true;
		if (subIntersection(x2,y2, x3,y3, x1,y1, x3,y3, rx1, ry1, rx2, ry2))	// part y>=y2 : L,R sides p2-p3, p1-p3
			return true;
	}
	else {
		if (subIntersection(x1,y1, x3,y3, x1,y1, x2,y2, rx1, ry1, rx2, ry2))	// part y<=y2 : L,R sides p1-p3, p1-p2
			return true;
		if (subIntersection(x1,y1, x3,y3, x2,y2, x3,y3, rx1, ry1, rx2, ry2))	// part y>=y2 : L,R sides p1-p3, p2-p3
			return true;
	}
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
		bool ok = parse_label(fmap, 0, 0, 0);
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

bool Map::parse_label(FILE *f, const char *scan_label, int crx=0, int cry=0) {	// crx,cry = "current room pointer"
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
		s[strlen(s)-1] = '\0';	// erase \n
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
			// required: x1<x2, y1<y2
			float x1, y1, x2, y2;
			int texid, alpha;
			int n = sscanf(s+1, " %f %f %f %f %i %i %c", &x1, &y1, &x2, &y2, &texid, &alpha, &nullc);
			if (n == 4) {
				texid = -1;
				alpha = 255;
			}
			if ((n!=4 && n!=6) || crx<0 || cry<0 || crx>=w || cry>=h) {
				LOG1("Invalid map line: %s\n", s);
				return false;
			}
			x1 *= plw; x2 *= plw;
			y1 *= plh; y2 *= plh;
			Room& rm = room[crx][cry];
			vector<RectWall>& wvec = (s[0]=='W') ? rm.rwalls : rm.rground;
			wvec.push_back(RectWall(int(x1), int(y1), int(x2), int(y2), texid, alpha));
			continue;
		}
		if (s[0]=='T') {	// T (W|G) x1 y1 x2 y2 x3 y3 [tex alpha] : triangular wall (W) or ground tex (G) (x1,y1)-(x2,y2)-(x3,y3) using given texture and alpha
			// required: y1<=y2, y2<=y3
			char type;
			float x1, y1, x2, y2, x3, y3;
			int texid, alpha;
			int n = sscanf(s+1, " %c %f %f %f %f %f %f %i %i %c", &type, &x1, &y1, &x2, &y2, &x3, &y3, &texid, &alpha, &nullc);
			if (n == 7) {
				texid = -1;
				alpha = 255;
			}
			if ((n!=7 && n!=9) || (type!='W' && type!='G') || crx<0 || cry<0 || crx>=w || cry>=h) {
				LOG1("Invalid map line: %s\n", s);
				return false;
			}
			x1 *= plw; x2 *= plw; x3 *= plw;
			y1 *= plh; y2 *= plh; y3 *= plh;
			Room& rm = room[crx][cry];
			vector<TriWall>& wvec = (type=='W') ? rm.twalls : rm.tground;
			wvec.push_back(TriWall(int(x1), int(y1), int(x2), int(y2), int(x3), int(y3), texid, alpha));
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
					if (!parse_label(f, nextlabel, rx, ry))
						return false;
			fseek(f, filepos, SEEK_SET);
			crx=rx2; cry=ry2;	// compatibility with original sloppy specs (needed?)
			continue;
		}
		if (!strncmp(s, "P width ", 8)) {	// P width w : set map width to w rooms
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
			if (title.length()) {
				LOG("Redefined map title\n");
				return false;
			}
			title=string(s+8);
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
			spot.x = (int)(x * (double)plw);
			spot.y = (int)(y * (double)plh);
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
			tinfo[team].flag.px = bound(rx, 0, w-1);
			tinfo[team].flag.py = bound(ry, 0, h-1);
			tinfo[team].flag.x = (int)(x * (double)plw);
			tinfo[team].flag.y = (int)(y * (double)plh);
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
		LOG1("Unrecognized map line: %s\n", s);
	}
}

class BITMAP;
void Map::draw_minimap(BITMAP* buffer, bool flagPaintSimple) const {
/*	//#fix: can't use colors in commont, they're client stuff
	#ifndef CL_MINIMAP_FLAGPOS
	(void)flagPaintSimple;
	#endif

	//black bg
	clear_to_color(buffer, 0);

	//draw screen boundaries
	int MMSCRW = (int)(98.0/float(w));
	int MMSCRH = (int)(98.0/float(h));
	int j;
	for (j=1;j<w;j++)
		line(buffer, 2+MMSCRW * j, 1, 2+MMSCRW * j, 100, col[COLSHADOW]);
	for (j=1;j<h;j++)
		line(buffer, 1, 2+MMSCRH * j, 100, 2+MMSCRH * j, col[COLSHADOW]);

	double maxx = plw*w;
	double maxy = plh*h;

	//draw bases
	#ifdef CL_MINIMAP_FLAGPOS
	if (flagPaintSimple) {
		int  red_rx = tinfo[0].flag.px,  red_ry = tinfo[0].flag.py;
		int blue_rx = tinfo[1].flag.px, blue_ry = tinfo[1].flag.py;
		if (red_rx==blue_rx && red_ry==blue_ry) {
			// for lack of mathematical enthusiasm, the half-way line is not calculated analytically; instead, determine each pixel individually (slow)
			// this is OK since this function is called once per map instead of every frame
			int xmin = 2+MMSCRW*red_rx, xmax = MMSCRW*(red_rx+1);
			int ymin = 2+MMSCRH*red_ry, ymax = MMSCRH*(red_ry+1);
			for (int y=ymin; y<=ymax; ++y) {
				float roomy = float(y + 1 - ymin) / float(MMSCRH) * plh;
				float ydist_r2 = pow(tinfo[0].flag.y-roomy, 2);
				float ydist_b2 = pow(tinfo[1].flag.y-roomy, 2);
				for (int x=xmin; x<=xmax; ++x) {
					float roomx = float(x + 1 - xmin) / float(MMSCRW) * plw;
					float xdist_r2 = pow(tinfo[0].flag.x-roomx, 2);
					float xdist_b2 = pow(tinfo[1].flag.x-roomx, 2);
					int color = (xdist_r2 + ydist_r2 < xdist_b2 + ydist_b2) ? col[COLBRED] : col[COLBBLUE];
					putpixel(buffer, x, y, color);
				}
			}
		}
		else {
			rectfill(buffer, 2+MMSCRW* red_rx, 2+MMSCRH* red_ry, MMSCRW*( red_rx+1), MMSCRH*( red_ry+1), col[COLBRED ]);
			rectfill(buffer, 2+MMSCRW*blue_rx, 2+MMSCRH*blue_ry, MMSCRW*(blue_rx+1), MMSCRH*(blue_ry+1), col[COLBBLUE]);
		}
	}
	#else
	int fx = tinfo[0].flag.px;
	int fy = tinfo[0].flag.py;
	rectfill(buffer, 2+ MMSCRW * fx, 2+ MMSCRH * fy, MMSCRW * (fx + 1), MMSCRH * (fy + 1), col[COLBRED]);
	fx = tinfo[1].flag.px;
	fy = tinfo[1].flag.py;
	rectfill(buffer, 2+ MMSCRW * fx, 2+ MMSCRH * fy, MMSCRW * (fx + 1), MMSCRH * (fy + 1), col[COLBBLUE]);
	#endif

	//draw solid walls
	float xmul=98./maxx, ymul=98./maxy;
	for (int y=0; y<h; y++) {
		float by=1.+y*plh*ymul;
		for (int x=0; x<w; x++) {
			float bx=1.+x*plw*xmul;
			room[x][y].draw(buffer, bx, by, xmul, ymul, makecol(0x00, 0x77, 0x00));
		}
	}

	//green border
	rect(buffer, 0, 0, buffer->w -1, buffer->h -1, col[COLGREEN]);

	#ifdef CL_MINIMAP_FLAGPOS
	int  red_px = int(1 + (tinfo[0].flag.px*plw + tinfo[0].flag.x)/maxx*98.);
	int  red_py = int(1 + (tinfo[0].flag.py*plh + tinfo[0].flag.y)/maxy*98.);
	int blue_px = int(1 + (tinfo[1].flag.px*plw + tinfo[1].flag.x)/maxx*98.);
	int blue_py = int(1 + (tinfo[1].flag.py*plh + tinfo[1].flag.y)/maxy*98.);
	if (!flagPaintSimple) {
		if (getpixel(buffer,  red_px,  red_py) != 0)	// is painted with any color
			return draw_minimap(buffer, true);	// restart with basic painting
		floodfill(buffer,  red_px,  red_py, col[COLBRED ]);

		if (getpixel(buffer, blue_px, blue_py) != 0)	// is painted with any color (including by previous red floodfill)
			return draw_minimap(buffer, true);	// restart with basic painting
		floodfill(buffer, blue_px, blue_py, col[COLBBLUE]);
	}
	circle(buffer,  red_px,  red_py, 3, col[COLRED ]);
	circle(buffer, blue_px, blue_py, 3, col[COLBLUE]);
	#endif
*/
}

/* calculateDisplacement():
 *
 * calculates how many times the vector (mx,my) can be traveled until wall (dx1,dy1)-(dx2,dy2) is hit by a circle of radius r (max value considered is 1.)
 *
 *
 *        (mx,my)      __--
 *          \    __--^^
 *         __+-^^  + -(dx1,dy1)
 *     +-^^      .>|
 *   (0,0)      /  + -(dx2,dy2)
 *             /
 * p         wall
 * |
 * +--n
 *
 * either
 * A) the circle hits the wall proper with it's center projection on the line
 * B) the circle hits one of the corners where it's center is at distance r from the corner the first time
 *
 * A: | ( t(mx,my)-(dx1,dy1) ) x ( (dx2,dy2)-(dx1,dy1) ) | / | (dx2,dy2)-(dx1,dy1) | = r , taking the smaller solution of t and making sure the point is on the line
 * B: | t(mx,my)-(dx,dy) | = r , taking the smaller solution of t (if any real solution exists)
 *
 * returns: pair( t, pair(collisionn-centern, collisionp-centerp) )
 */
typedef pair<double, double> Coords;
pair<double, Coords> calculateDisplacement(double dx1, double dy1, double dx2, double dy2, double mx, double my, double r) {	// d=distance, m=movement
	// check for solution A (if there is one, there is no need to check B)
	// t * ( mx(dy2-dy1) - my(dx2-dx1) ) = dx1(dy2-dy1) - dy1(dx2-dx1) +-R*|(dx2,dy2)-(dx1,dy1)|
	double diffx = dx2-dx1, diffy = dy2-dy1;
	double div = mx*diffy - my*diffx;
	if (div == 0)	// movement parallel to the line => no collision
		return pair<double, Coords>(999., Coords());
	double rBase = ( dx1*diffy - dy1*diffx ) / div;
	double rAdd = r * sqrt(diffx*diffx+diffy*diffy) / div;
	double t = rBase - fabs(rAdd);	// the collision with smaller t (the other would be going away)
	if (t >= 0) {
		// make sure we are not off an end of the line
		// this can surely be calculated in a simpler way, but this first came to mind
		// collp = p1 + k(p2-p1)	0<=k<=1 if on the line
		// | t*m - collp |  minimum (=r)
		// | t*m - p1 - k(p2-p1) |  minimum (=r)
		// ( t*mx - dx1 - k(dx2-dx1) )^2 + ( t*my - dy1 - k(dy2-dy1) )^2  minimum (=r)
		// (dx2-dx1)*( t*mx - dx1 - k(dx2-dx1) ) + (dy2-dy1)*( t*my - dy1 - k(dy2-dy1) ) = 0  (derivative of the one above)
		// (dx2-dx1)*(t*mx-dx1) + (dy2-dy1)*(t*my-dy1) = k[ (dx2-dx1)(dx2-dx1) + (dy2-dy1)*(dy2-dy1) ]
		double k = ( diffx*(t*mx-dx1) + diffy*(t*my-dy1) ) / (diffx*diffx + diffy*diffy);
		if (k>=0. && k<=1.)
			return pair<double, Coords>(t, Coords(dx1+k*diffx-t*mx, dy1+k*diffy-t*my));
	}

	double dist=1.;
	Coords collisionCoords;
	// check for solution B
	// for dp1:
	double m2=mx*mx+my*my, r2=r*r;	// same for dp2
	double mdotd=mx*dx1+my*dy1;
	double d2=dx1*dx1+dy1*dy1;
	double disc=mdotd*mdotd-m2*(d2-r2);
	if (disc>=0) {	// there are real solutions
		t=(mdotd-sqrt(disc))/m2;	// select smaller t
		if (t<0)
			t=(mdotd+sqrt(disc))/m2;
		if (t>=0 && t<dist) {
			dist=t;
			collisionCoords=Coords(dx1-t*mx, dy1-t*my);
		}
	}
	// for dp2:
	mdotd=mx*dx2+my*dy2;
	d2=dx2*dx2+dy2*dy2;
	disc=mdotd*mdotd-m2*(d2-r2);
	if (disc>=0) {	// there are real solutions
		t=(mdotd-sqrt(disc))/m2;	// select smaller t
		if (t<0)
			t=(mdotd+sqrt(disc))/m2;
		if (t>=0 && t<dist) {
			dist=t;
			collisionCoords=Coords(dx2-t*mx, dy2-t*my);
		}
	}
	return pair<double, Coords>(dist, collisionCoords);
}

bool new_wallcorrect(const Room& r, double fraction, double *x, double *y, double *sx, double *sy) {
	static const double plyRadius=15;

	double stx=*x, sty=*y-PHYS_SHIFTY;	// position in real coordinates
	double mx=*sx, my=*sy;	// speed

	bool bounced=false;
	double movementLeft=fraction;

	for (;;) {
		double minMovement=movementLeft;
		Coords bounceVec;
		Coords bbox0(min(stx-plyRadius, stx+mx-plyRadius), min(sty-plyRadius, sty+my-plyRadius)),
		       bbox1(max(stx+plyRadius, stx+mx+plyRadius), max(sty+plyRadius, sty+my+plyRadius));
		for (vector<RectWall>::const_iterator wi=r.rwalls.begin(); wi!=r.rwalls.end(); ++wi) {	// go through rectangular walls first
			// fast and crude bounding-box style check first
			if (!wi->intersects_rect(bbox0.first, bbox0.second, bbox1.first, bbox1.second))
				continue;
			// check more carefully
			pair<double, Coords> rv;
			rv.first = 1.;
			if (mx>0 && wi->a>stx)	// check vertical wall a
				rv = calculateDisplacement(wi->a - stx, wi->b - sty, wi->a - stx, wi->d - sty, mx, my, plyRadius);
			else if (mx<0 && wi->c<stx)	// check vertical wall c
				rv = calculateDisplacement(wi->c - stx, wi->b - sty, wi->c - stx, wi->d - sty, mx, my, plyRadius);
			if (rv.first < minMovement) {
				minMovement = rv.first;
				bounceVec = rv.second;
			}
			if (my>0 && wi->b>sty)	// check horizontal wall b
				rv = calculateDisplacement(wi->a - stx, wi->b - sty, wi->c - stx, wi->b - sty, mx, my, plyRadius);
			else if (my<0 && wi->d<sty)	// check horizontal wall d
				rv = calculateDisplacement(wi->a - stx, wi->d - sty, wi->c - stx, wi->d - sty, mx, my, plyRadius);
			if (rv.first < minMovement) {
				minMovement = rv.first;
				bounceVec = rv.second;
			}
			#ifndef NDEBUG
			if (minMovement<movementLeft) {
				double dx=bounceVec.first, dy=bounceVec.second, r=plyRadius;
				assert(fabs(dx*dx+dy*dy-r*r)<.0001);
			}
			#endif
		}
		for (vector<TriWall>::const_iterator wi=r.twalls.begin(); wi!=r.twalls.end(); ++wi) {	// go through triangular walls separately
			// fast and crude bounding-box style check first
			if (!wi->intersects_rect(bbox0.first, bbox0.second, bbox1.first, bbox1.second))
				continue;
			// check more carefully
			pair<double, Coords> rv;
			rv = calculateDisplacement(wi->x1 - stx, wi->y1 - sty, wi->x2 - stx, wi->y2 - sty, mx, my, plyRadius);	// wall p1-p2
			if (rv.first < minMovement) {
				minMovement = rv.first;
				bounceVec = rv.second;
			}
			rv = calculateDisplacement(wi->x1 - stx, wi->y1 - sty, wi->x3 - stx, wi->y3 - sty, mx, my, plyRadius);	// wall p1-p3
			if (rv.first < minMovement) {
				minMovement = rv.first;
				bounceVec = rv.second;
			}
			rv = calculateDisplacement(wi->x2 - stx, wi->y2 - sty, wi->x3 - stx, wi->y3 - sty, mx, my, plyRadius);	// wall p2-p3
			if (rv.first < minMovement) {
				minMovement = rv.first;
				bounceVec = rv.second;
			}
			#ifndef NDEBUG
			if (minMovement<movementLeft) {
				double dx=bounceVec.first, dy=bounceVec.second, r=plyRadius;
				assert(fabs(dx*dx+dy*dy-r*r)<.0001);
			}
			#endif
		}
		assert(minMovement>=0. && minMovement<=movementLeft);
		stx+=mx*minMovement*.999;	// make sure we aren't going the least bit inside a wall :)
		sty+=my*minMovement*.999;
		if (stx < 0)    { sty-=     stx *my/mx; stx=  0; break; }
		if (stx >= plw) { sty-=(stx-plw)*my/mx; stx=plw; break; }
		if (sty < 0)    { stx-=     sty *mx/my; sty=  0; break; }
		if (sty >= plh) { stx-=(sty-plh)*mx/my; sty=plh; break; }
		if (minMovement>=movementLeft*.999)	// not bounced
			break;
		bounced=true;
		// bounce: speed component parallel with bounceVec ( (S dot b / |b|) * b / |b| ) is reversed, while perpendicular component is kept
		// : S -= 2* ( (S dot b) * b / |b|^2 )	; |b| is always plyRadius
		double mul=2.*(mx*bounceVec.first+my*bounceVec.second)/(plyRadius*plyRadius);
		mx -= mul*bounceVec.first;
		my -= mul*bounceVec.second;
		// lose some speed too
		mx *= .95;
		my *= .95;
		movementLeft-=minMovement+.01;	// don't bounce over 100 times in any conditions
		if (movementLeft<0)
			break;
	}
	*x = stx;
	*y = sty + PHYS_SHIFTY;
	*sx = mx;
	*sy = my;
	return bounced;
}

//wall hit?
bool wallhit(double x, double y, const RectWall &w) { int ix=(int)x, iy=(int)y; return w.intersects_rect(ix, iy, ix, iy); }

//wall collision correction
bool old_wallcorrect(const Room& room, double *x, double *y, double *sx, double *sy, double *ox, double *oy) {
	//delta old to new (ok)
	double tx,ty;
	tx = (*ox) - (*x);
	ty = (*oy) - (*y);

	if (tx==0. && ty==0.) {
		*x = *ox;
		*y = *oy;
		return false;
	}

	//deltas for pushing out of walls: normalize
	double dx, dy;
	if (fabs(tx) > fabs(ty)) {
		dx = 2*tx / fabs(tx); // ==1.0
		dy = 2*ty / fabs(tx);		// 0 <= val <= 1
	}
	else {
		dx = 2*tx / fabs(ty);	// 0 <= val <= 1
		dy = 2*ty / fabs(ty);	// ==1.0
	}

	bool ever_had_wall_hit = false;
	bool had_wall_hit; //keep pushing out until no wall hit
	const Room* r = &room;

	int runaway = 10;
	do {

		had_wall_hit = false;
		bool y_solved = false;

		for (int w=0;w<(int)r->rwalls.size();w++) {
			int runaw = 100;
			while (wallhit((*x),(*y)-PHYS_SHIFTY,r->rwalls[w])) {
				had_wall_hit = true;
				(*x) += dx;
				y_solved = false;
				if (!(wallhit((*x),(*y)-PHYS_SHIFTY,r->rwalls[w])))
				break;
				(*y) += dy;
				y_solved = true;
				runaw--;
				if (runaw < 0) {
					(*x) = (*ox);
					(*y) = (*oy);
					return false;
				}
			}
		}
		if (had_wall_hit) {
			ever_had_wall_hit = true;
			if (y_solved)
				(*sy) *= -1;
			else
				(*sx) *= -1;
		}
		runaway--;
		if (runaway < 0) {
			(*x) = (*ox);
			(*y) = (*oy);
			return false;
		}
	} while (had_wall_hit);
	if (ever_had_wall_hit) {
		if (((*x) <= 0.01) || ((*x) >= ((double)plw) - 0.01) || ((*y)-PHYS_SHIFTY <= 0.01) || ((*y)-PHYS_SHIFTY >= ((double)plh) - 0.01)) {
			(*x) = (*ox);
			(*y) = (*oy);
		}
		(*ox) = (*x);
		(*oy) = (*y);
	}
	return ever_had_wall_hit;
}

bool applyNewPhysics(PlayerBase* h, const Room& room, float fraction, bool turbo, bool carryFlag, bool deathbringer_affected) {
	//select effective physics vars for the player
	float player_accel, player_friction, player_maxspeed;
	if (h->run) {
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
	if (h->run && carryFlag)
		player_maxspeed -= svp_flag_penalty;

	int xAcc = (h->r?1:0) - (h->l?1:0), yAcc = (h->d?1:0) - (h->u?1:0);

	#ifdef PHYS_VECTOR_ACC

	// this is a more physically correct model by Nix

	// scale these up by 1.2 so the acceleration is near the average of original (either 1 or sqrt(2) times)
	player_maxspeed *= 1.2;
	player_friction *= 1.2;
	player_accel    *= 1.2;
	player_accel    += player_friction;	// to balance forward acceleration with the original model; backward acceleration is too big however

	// friction
	float spd = sqrt( h->sx*h->sx + h->sy*h->sy );
	float mul;
	if (spd <= player_friction)
		mul = 0.;
	else
		mul = 1. - player_friction/spd;
	h->sx *= mul;
	h->sy *= mul;

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
	if (!xAcc || absx>player_maxspeed) {
		if (absx > player_friction)
			h->sx *= 1. - player_friction/absx;
		else
			h->sx = 0.;
	}
	if (!yAcc || absy>player_maxspeed) {
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

	//wall collision correction
	return new_wallcorrect(room, fraction, &h->lx, &h->ly, &h->sx, &h->sy);
}

bool applyOldPhysics(PlayerBase* h, const Room& room, float fraction, bool turbo, bool carryFlag, bool deathbringer_affected) {
	//select effective physics vars for the player
	//
	float player_accel;
	float player_friction;
	float player_maxspeed;
	if (h->run) {
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
	if (h->run && carryFlag)
		player_maxspeed -= svp_flag_penalty;

	//friction x - apply if l xor r
	#ifndef ALWAYS_FRICTION
	if ( ((int)h->l + (int)h->r != 1) || (fabs(h->sx) > player_maxspeed) ) {
	#endif
		if (h->sx > 0) {
			//h->sx -= sv_frictionx * boots_accel_bonus;
			h->sx -= player_friction * fraction;
			if (h->sx < 0) h->sx = 0;
		}
		else if (h->sx < 0) {
			//h->sx += sv_frictionx * boots_accel_bonus;
			h->sx += player_friction * fraction;
			if (h->sx > 0) h->sx = 0;
		}
	#ifndef ALWAYS_FRICTION
	}
	#endif

	//friction y
	#ifndef ALWAYS_FRICTION
	if ( ((int)h->u + (int)h->d != 1) || (fabs(h->sy) > player_maxspeed) ){
	#endif
		if (h->sy > 0) {
			//h->sy -= sv_frictiony * boots_accel_bonus;
			h->sy -= player_friction * fraction;
			if (h->sy < 0) h->sy = 0;
		}
		else if (h->sy < 0) {
			//h->sy += sv_frictiony * boots_accel_bonus;
			h->sy += player_friction * fraction;
			if (h->sy > 0) h->sy = 0;
		}
	#ifndef ALWAYS_FRICTION
	}
	#endif

	//deathbringer penalty : no movement. move only if not in effect
	if (!deathbringer_affected) {

		//accelerate x if not over maximum speed
		if ((h->l) && (h->sx > -player_maxspeed))
			h->sx -= player_accel * fraction;
		if ((h->r) && (h->sx < +player_maxspeed))
			h->sx += player_accel * fraction;
		//accelerate y if not over maximum speed
		if ((h->u) && (h->sy > -player_maxspeed))
			h->sy -= player_accel * fraction;
		if ((h->d) && (h->sy < +player_maxspeed))
			h->sy += player_accel * fraction;
	}

	//DEBUG
	//if (h->sx > +player_maxspeed) h->sx = +player_maxspeed;
	//if (h->sx < -player_maxspeed) h->sx = -player_maxspeed;
	//if (h->sy > +player_maxspeed) h->sy = +player_maxspeed;
	//if (h->sy < -player_maxspeed) h->sy = -player_maxspeed;

	//save ox,oy
	double ox = h->lx;
	double oy = h->ly;

	//move x
	h->lx += h->sx * fraction;
	if (h->lx < 0) h->lx = 0;
	else if (h->lx > plw) h->lx = plw;

	//move y
	h->ly += h->sy * fraction;
	if (h->ly-PHYS_SHIFTY < 0) h->ly = 0+PHYS_SHIFTY;
	else if (h->ly-PHYS_SHIFTY > plh) h->ly = plh+PHYS_SHIFTY;

	//wall collision correction
	return old_wallcorrect(room, &h->lx, &h->ly, &h->sx, &h->sy, &ox, &oy);
}

bool WorldBase::applyPhysics(int pid, const Room& room, float fraction, bool turbo, bool carryFlag, bool deathbringer_affected) {
	#ifdef PHYS_NEW
	return applyNewPhysics(player[pid].getPtr(), room, fraction, turbo, carryFlag, deathbringer_affected);
	#else
	return applyOldPhysics(player[pid].getPtr(), room, fraction, turbo, carryFlag, deathbringer_affected);
	#endif
}


//run a physics frame simulation step for a player
void WorldBase::run_server_player_physics(int i) {	// player id
	PlayerBase* hd = player[i].getPtr();

	if (hd->roomx<0 || hd->roomy<0 || hd->roomx>=map.w || hd->roomy>=map.h) return;	//#fix: remove this and track why these are given sometimes
	const Room& room = map.room[hd->roomx][hd->roomy];

	bool carryFlag = flag[1-(i/TSIZE)].carried && flag[1-(i/TSIZE)].carrier == i;
	bool deathbringerAffected = hd->under_deathbringer_effect(get_time());

	float startx = hd->lx, starty = hd->ly;

	applyPhysics(i, room, 1., hd->item_speed, carryFlag, deathbringerAffected);

	ServerPlayer* spp = dynamic_cast<ServerPlayer*>(hd);	//#fix
	if (spp) {
		float xd = hd->lx - startx;
		float yd = hd->ly - starty;
		spp->total_movement += sqrt( xd*xd + yd*yd );
	}

	//check room change x
	if (int(hd->lx) == plw) {
		hd->lx = 1;
		if (++hd->roomx >= map.w)
			hd->roomx = 0;
	}
	else if (int(hd->lx) == 0) {
		hd->lx = plw - 1;
		if (--hd->roomx < 0)
			hd->roomx = map.w - 1;
	}

	//check room change y
	if (int(hd->ly)-PHYS_SHIFTY == plh) {
		hd->ly = 1 +PHYS_SHIFTY;
		if (++hd->roomy >= map.h)
			hd->roomy = 0;
	}
	else if (int(hd->ly)-PHYS_SHIFTY == 0) {
		hd->ly = plh - 1 +PHYS_SHIFTY;
		if (--hd->roomy < 0)
			hd->roomy = map.h - 1;
	}
}

void WorldBase::returnFlag(int team) {
	flag[team].carried = false;			// not carried anymore
	flag[team].pos = map.tinfo[team].flag;		// return to original position
	flag[team].atbase = true;		// yes, at base
}

void WorldBase::dropFlag(int team, int px, int py, int x, int y) {
	flag[team].carried = false;		// not carried
	flag[team].pos.px = px;		// dropped somewhere
	flag[team].pos.py = py;
	flag[team].pos.x = x;
	flag[team].pos.y = y;
	flag[team].atbase = false;		// not at base, team must touch to return (or it can be stolen)
}

void WorldBase::stealFlag(int team, int carrier) {
	flag[team].carried = true;		// carried
	flag[team].carrier = carrier;	// who stole it
	flag[team].atbase = false;		// not at base (not needed / paranoia)
}

#include "server.h"
//#fix: include needed for funny callback activities - get rid!

void ServerWorld::returnFlag(int team) {
	WorldBase::returnFlag(team);
	host->ctf_net_flag_status(-1, team);
}

void ServerWorld::dropFlag(int team, int roomx, int roomy, int lx, int ly) {
	WorldBase::dropFlag(team, roomx, roomy, lx, ly);
	host->ctf_net_flag_status(-1, team);
}

void ServerWorld::stealFlag(int team, int carrier) {
	WorldBase::stealFlag(team, carrier);
	host->ctf_net_flag_status(-1, team);
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
	if (!player[pid].respawn_to_base || map.fall_on_wall(pos.px, pos.py, pos.x-20, pos.y-PHYS_SHIFTY-20, pos.x+20, pos.y-PHYS_SHIFTY+20)) {
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
			pos.y = plh / 8 + rand() % (3 * plh / 4) +PHYS_SHIFTY;

			//do a check for walls, maybe retrying another screen if hits a wall
			if (!map.fall_on_wall(pos.px, pos.py, pos.x-20, pos.y-PHYS_SHIFTY-20, pos.x+20, pos.y-PHYS_SHIFTY+20))
				break;	//success!

			//fall on wall true, keep trying...

		} while (runaway-- > 0);

		if (runaway <= 0)
			host->broadcast_message("PLAYER SPAWN RUNAWAY");
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

	host->sendWeaponPower(pid);

	player[pid].item_shield = false;
	player[pid].item_quad = false;
	player[pid].item_speed = false;
	player[pid].item_helm = 0;
	player[pid].item_deathbringer = false;
	player[pid].deathbringer_end = 0;

	player[pid].respawn_to_base = false;

	player[pid].last_spawn_time = (int)get_time();
	player[pid].dead = false;

	//for all effects, player screen changed
	host->game_player_screen_change(pid);
}

