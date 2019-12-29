/*
 * Copyright (c) 2005-2019 Libor Pecháček.
 *
 * This file is part of CoVe 
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Polygons.h"

#include <algorithm>
#include <cerrno>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iosfwd>
#include <limits>
#include <memory>
#include <stdexcept>

#include <QByteArray>
#include <QString>
#include <QtGlobal>

#include <QImage>
#include <QRect>

#include "Vectorizer.h"

#define JOIN_DEBUG 0
#define JOIN_DEBUG_PRINT(...)            \
do                                       \
{                                        \
	if (JOIN_DEBUG) qDebug(__VA_ARGS__); \
} while (0)

using namespace std;

namespace cove {
const int Polygons::NPOINTS_MAX = 350;

//@{
//! \ingroup libvectorizer

/*! \class Polygons
 * \brief This class encapsulates vectorization of 4-connected 1px wide lines
 * in given image. */

/*! \enum Polygons::DIRECTION
  Internal enum for directions having values EAST, WEST, NORTH, SOUTH and NONE.
  */

/*! \class Polygons::POLYGON_POINT
   \brief One point of the polygon.

   Just a coordinate pair.
   */

/*! \var int Polygons::POLYGON_POINT::x
   \brief The x-coordinate. */

/*! \var int Polygons::POLYGON_POINT::y
   \brief The y-coordinate. */

/*! \class Polygons::PATH_POINT
   \brief One point of the path.

   A coordinate pair and a direction.  Direction is the direction where the
   path follows from this point to the next point. The last point of the path
   has direction NONE.
   */

/*! \var int Polygons::PATH_POINT::x
   \brief The x-coordinate. */

/*! \var int Polygons::PATH_POINT::y
   \brief The y-coordinate. */

/*! \var DIRECTION Polygons::PATH_POINT::d
   \brief The direction to the next point.  The last point has direction NONE.
   */

/*! \class Polygons::PolygonList
   \brief List of polygons in an image.

 Instance of STL template vector.*/

/*! \class Polygons::PathList
   \brief List of paths in an image.

 Instance of STL template vector.*/

// -PATH-----------------------
/*! \class Polygons::Path
   \brief Represents path in an image.

   The path in an image is a set of neighboring points making up a line.  Each
   point has exactly two neighbors. The only exception is an unclosed path
   where the end points have only one neighbor.

   The path is later being examined for straight segments by \a
   Polygons::getPathPolygon.
   */
/*! Default constructor. */
Polygons::Path::Path()
	: pathClosed(false)
{
}

/*! \brief Sets the path-closedness flag.
 *
 * Path is closed when and only when its first point is identical with its
 * last point. I.e. it forms a cycle in graph.*/
void Polygons::Path::setClosed(bool closed)
{
	pathClosed = closed;
}

/*! \brief Returns value of the path-closedness flag.

  \see setClosed
*/
bool Polygons::Path::isClosed() const
{
	return pathClosed;
}

// -POLYGON--------------------
/*! \class Polygons::Polygon
   \brief Represents polygon in an image.

   The polygon is a set of straight lines.  The first point represents the
   beginning of the polygon, following points represent vertexes of the polygon
   and the last point is the end in case the polygon is not closed.

   The polygon is usually being built as a result of path decomposition into
   straight segments.
   */
Polygons::Polygon::Polygon()
	: polygonClosed(false)
	, minX(INT_MAX)
	, minY(INT_MAX)
	, maxX(INT_MIN)
	, maxY(INT_MIN)
{
}

/*! \brief Reimplemented push_back.

  Reimplemented push_back updates min/max X/Y (bounding rectangle). */
void Polygons::Polygon::push_back(const value_type& p)
{
	vector<POLYGON_POINT>::push_back(p);
	if (p.x > maxX) maxX = p.x;
	if (p.x < minX) minX = p.x;
	if (p.y > maxY) maxY = p.y;
	if (p.y < minY) minY = p.y;
}

/*! Returns bounding rectangle of the polygon. */
QRect Polygons::Polygon::boundingRect() const
{
	return QRect(int(minX), int(minY), int(maxX - minX + 1),
				 int(maxY - minY + 1));
}

/*! Updates bounding rectangle of the polygon. */
void Polygons::Polygon::recheckBounds()
{
	minX = INT_MAX;
	minY = INT_MAX;
	maxX = INT_MIN;
	maxY = INT_MIN;
	for (const_iterator p = begin(); p != end(); ++p)
	{
		if (p->x > maxX) maxX = p->x;
		if (p->x < minX) minX = p->x;
		if (p->y > maxY) maxY = p->y;
		if (p->y < minY) minY = p->y;
	}
}

/*! \brief Sets the polygon-closedness flag.
 *
 * Polygon is closed when and only when its first point is identical with its
 * last point. */
void Polygons::Polygon::setClosed(bool closed)
{
	polygonClosed = closed;
}

/*! \brief Returns value of the polygon-closedness flag.

  \see setClosed
*/
bool Polygons::Polygon::isClosed() const
{
	return polygonClosed;
}

//-POLYGONS-------------------------------------------------------
Polygons::Polygons()
	: specklesize(9)
	, maxdist(9)
	, distdirratio(0.8)
{
}

void Polygons::setSpeckleSize(int val)
{
	specklesize = val;
}

void Polygons::setMaxDistance(double val)
{
	maxdist = val;
}

void Polygons::setSimpleOnly(bool val)
{
	simpleonly = val;
}

void Polygons::setDistDirRatio(double val)
{
	distdirratio = val;
}

int Polygons::speckleSize() const
{
	return specklesize;
}

double Polygons::maxDistance() const
{
	return maxdist;
}

bool Polygons::simpleOnly() const
{
	return simpleonly;
}

double Polygons::distDirRatio() const
{
	return distdirratio;
}

/*! \brief Looks for any pixel in image.

  Updates reference variables xp, yp so that they contain next black pixel
  position in the image. The scanning starts from point [xp, yp].  Returns
  true when a black pixel was found, false otherwise.
 \param[in] image Input image.
 \param[in,out] xp x coordinate where to start, will be modified so that it
 contains x coordinate of next black pixel
 \param[in,out] yp y coordinate, similar to xp
 \return true when xp, yp contain valid coordinates of black pixel, false
 otherwise */
bool Polygons::findNextPixel(const QImage& image, int& xp, int& yp)
{
	int x = xp;
	for (int y = yp; y < image.height(); y++)
	{
		for (; x < image.width(); x++)
		{
			if (image.pixelIndex(x, y))
			{
				// we have found one
				xp = x;
				yp = y;
				return true;
			}
		}
		x = 0;
	}
	// there are no pixels left
	return false;
}

/*! \brief Follows path in an image.

  Follows path in an image and records it into \a path.
  \param[in] image Image to be examined.
  \param[in,out] x X-ccordinate of the first pixel of the path.
  \param[in,out] y Y-ccordinate of the first pixel of the path.
  \param[in,out] path Pointer to Polygons::Path where the pixel coordinates will
  be stored. The variable can be nullptr in which case no coordinates are recorded.
 */
void Polygons::followPath(const QImage& image, int& x, int& y, Path* path)
{
	const int origX = x, origY = y;
	int newx = 0, newy = 0;
	int imWidth = image.width(), imHeight = image.height();
	DIRECTION direction = NONE, newDirection;
	int canProceed;
	bool firstCycle = true;

	for (;;)
	{
		canProceed = 0;
		newDirection = NONE;

		if (x < imWidth - 1 && image.pixelIndex(x + 1, y) && direction != WEST)
		{
			newx = x + 1;
			newy = y;
			newDirection = EAST;
			canProceed++;
		}
		if (x && image.pixelIndex(x - 1, y) && direction != EAST)
		{
			newx = x - 1;
			newy = y;
			newDirection = WEST;
			canProceed++;
		}
		if (y < imHeight - 1 && image.pixelIndex(x, y + 1) &&
			direction != NORTH)
		{
			newx = x;
			newy = y + 1;
			newDirection = SOUTH;
			canProceed++;
		}
		if (y && image.pixelIndex(x, y - 1) && direction != SOUTH)
		{
			newx = x;
			newy = y - 1;
			newDirection = NORTH;
			canProceed++;
		}

		if (path)
		{
			PATH_POINT p = {x, y, newDirection};
			path->push_back(p);
		}

		// is the path closed?  (we are in the starting point)
		if (canProceed && newx == origX && newy == origY)
		{
			if (path) path->setClosed(true);
			break;
		}

		if (canProceed == 1 || (firstCycle && canProceed == 2))
		{
			x = newx;
			y = newy;
			direction = newDirection;
			firstCycle = false;
		}
		else
			break;
	}
}

/*! Find a path in image. The method traverses the path to its end (or around
 * when cyclic), then stores its pixel coordinates into returned Path.
 * \param image the image where to look for path
 \param initX pixel coordinates where to start traversal
 \param initY see initX
 \return path found */
Polygons::Path Polygons::recordPath(const QImage& image, const int initX,
									const int initY)
{
	int x = initX, y = initY;
	// find the end of the path
	followPath(image, x, y);
	// record path into newPath
	Path newPath;
	followPath(image, x, y, &newPath);
	// and happily return it...
	return newPath;
}

/*! Draw into the image so that original path disappears. */
void Polygons::removePathFromImage(QImage& image, const Path& path)
{
	for (auto const& i : path)
	{
		image.setPixel(i.x, i.y, 0); // delete the pixel
	}
}

/*! Finds all paths in image and returns them in PathList. */
Polygons::PathList
Polygons::decomposeImageIntoPaths(const QImage& sourceImage,
								  ProgressObserver* progressObserver) const
{
	PathList pathList;
	QImage image = sourceImage;
	int height = sourceImage.height(),
		progressHowOften = (height > 100) ? height / 45 : 1;
	int x = 0, y = 0;
	bool cancel = false;
	while (!cancel && findNextPixel(image, x, y))
	{
		Path pathFound = recordPath(image, x, y);
		removePathFromImage(image, pathFound);
		if (pathFound.size() > specklesize) pathList.push_back(pathFound);
		if (progressObserver && !(y % progressHowOften))
		{
			// FIXME hardcoded value 25!!!
			progressObserver->percentageChanged(y * 25 / height);
			cancel = progressObserver->getCancelPressed();
		}
	}

	return !cancel ? pathList : PathList();
}

/*! Identifies straight segments of path and returns them as list of vertices
 * between them.  Prepares data for libpotrace, then calls its functions. */
Polygons::PolygonList
Polygons::getPathPolygons(const Polygons::PathList& constpaths,
						  ProgressObserver* progressObserver) const
{
	path_t* p;
	path_t* plist = nullptr;   /* linked list of path objects */
	path_t** hook = &plist; /* used to speed up appending to linked list */

	Polygon list;
	PolygonList polylist;
	polylist.reserve(constpaths.size());

	// convert Polygons::Paths into Potrace paths
	bool cancel = false;
	int tpolys = constpaths.size(), cntr = 0;
	int progressHowOften = (tpolys > 100) ? tpolys / 35 : 1;
	for (PathList::const_iterator pathsiterator = constpaths.begin();
		 !cancel && pathsiterator != constpaths.end(); ++pathsiterator)
	{
		int len = pathsiterator->size();
		point_t* pt = (point_t*)malloc(len * sizeof(point_t));
		p = path_new();
		if (!p || !pt)
		{
			throw runtime_error("memory allocation failed");
		}
		privpath_t* pp = p->priv;
		pp->pt = pt;
		pp->len = len;
		pp->closed = pathsiterator->isClosed();
		p->area = 100; // no speckle
		p->sign = '+';

		len = 0;
		for (auto const& i : *pathsiterator)
		{
			pt[len].x = i.x;
			pt[len].y = i.y;
			len++;
		}

		list_insert_beforehook(p, hook);

		if (progressObserver && !((++cntr) % progressHowOften))
		{
			progressObserver->percentageChanged(25 + cntr * 25 / tpolys);
			cancel = progressObserver->getCancelPressed();
		}
	}

// create polygons for all paths
#define TRY(x) \
	if (x) goto try_error
	list_forall(p, plist)
	{
		TRY(calc_sums(p->priv));
		TRY(calc_lon(p->priv));
		TRY(bestpolygon(p->priv));
		TRY(adjust_vertices(p->priv));
	}

	// do joining...
	if (maxdist) joinPolygons(plist, progressObserver);

	// convert potrace line segments into CoVe Polygons
	// working on p->priv->curve
	cntr = 0;
	list_forall(p, plist)
	{
		privpath_t* pp = p->priv;
		POLYGON_POINT p;
		int n = pp->curve.n;
		bool curveclosed = pp->curve.closed;

		list.reserve(n);

		for (int i = 0; i < n; i++)
		{
			p.x = pp->curve.vertex[i].x;
			p.y = pp->curve.vertex[i].y;
			list.push_back(p);
		}

		list.setClosed(curveclosed);

		polylist.push_back(list);
		list.clear();

		if (progressObserver && !((++cntr) % progressHowOften))
		{
			progressObserver->percentageChanged(90 + cntr * 10 / tpolys);
			cancel = progressObserver->getCancelPressed();
		}
	}

	list_forall_unlink(p, plist)
	{
		path_free(p);
	}

	return polylist;

try_error:
	qWarning("process_path failed with errno %d", errno);
	path_free(p);
	return PolygonList();
}

/*! Euclidean distance of two POLYGON_POINTs. */
float Polygons::distance(const POLYGON_POINT& a, const POLYGON_POINT& b)
{
	double x = a.x - b.x;
	double y = a.y - b.y;
	return std::sqrt(x * x + y * y);
}

/*! \brief Returns set of sequences of straight line segments in the image.

 Decomposes image into paths \a decomposeImageIntoPaths and every path longer
 than \a speckleSize is transformed into straight line segments by \a
 getPathPolygon.
 \param[in] image Input image to be analyzed.
 \param[in] speckleSize Minimum length of path to be vectorized. */
Polygons::PolygonList
Polygons::createPolygonsFromImage(const QImage& image,
								  ProgressObserver* progressObserver) const
{
	PathList p = decomposeImageIntoPaths(image, progressObserver);
	if (p.empty()) return PolygonList();
	return getPathPolygons(p, progressObserver);
}

/*! \brief Return list of polygons where close enough polygons are joined.

 Close enough means that polygons have ends closer than maxDist.
 \param[in] fromPL Input polygon list.
 \param[in] maxDist Maximum distance of two ends that will be joined.
 \param[in] distDirRatio Parameter controlling a not yet implemented feature. */

//! Fast computation of distance square of two points.  It is used in
// comparisions where the monotonic transformation makes no problem. It saves
// one call to sqrt(3).
inline float Polygons::distSqr(const dpoint_t* a, const dpoint_t* b) const
{
	float p = a->x - b->x, q = a->y - b->y;
	return p * p + q * q;
}

inline Polygons::JOINEND Polygons::joinEndA(Polygons::JOINTYPE j) const
{
	return (j == FF || j == FB) ? FRONT : ((j == BF || j == BB) ? BACK : NOEND);
}

inline Polygons::JOINEND Polygons::joinEndB(Polygons::JOINTYPE j) const
{
	return (j == FF || j == BF) ? FRONT : ((j == FB || j == BB) ? BACK : NOEND);
}

inline Polygons::JOINEND Polygons::oppositeEnd(Polygons::JOINEND j) const
{
	return (j == BACK) ? FRONT : ((j == FRONT) ? BACK : NOEND);
}

inline Polygons::JOINTYPE Polygons::endsToType(Polygons::JOINEND ea,
											   Polygons::JOINEND eb) const
{
	if (ea == FRONT)
	{
		if (eb == FRONT) return FF;
		if (eb == BACK) return FB;
	}
	if (ea == BACK)
	{
		if (eb == FRONT) return BF;
		if (eb == BACK) return BB;
	}
	return NOJOIN;
}

/*
   Distance function, evaluates possible joins with respect to
   distance/direction ratio parameter.

   a - point before end of line 1
   b - end of line 1
   c - end of line 2
   d - point before end of line 2

           v2
      b----------c
     /            \
 v1 /              \ v3
   /                \
  a                  d

precondition: || v2 || < maxdist
postcondition: return value is from <0,1>
 */
inline float Polygons::dstfun(const dpoint_t* a, const dpoint_t* b,
							  const dpoint_t* c, const dpoint_t* d) const
{
	dpoint_t v1, v2, v3;
	v1.x = b->x - a->x;
	v1.y = b->y - a->y;
	v2.x = c->x - b->x;
	v2.y = c->y - b->y;
	v3.x = d->x - c->x;
	v3.y = d->y - c->y;
	float dotprod12 = v1.x * v2.x + v1.y * v2.y;
	float dotprod23 = v2.x * v3.x + v2.y * v3.y;
	float norm1 = sqrt(v1.x * v1.x + v1.y * v1.y);
	float norm2 = sqrt(v2.x * v2.x + v2.y * v2.y);
	float norm3 = sqrt(v3.x * v3.x + v3.y * v3.y);
	// direction cosine - 1 when the vectors have identical direction, -1 when
	// opposite, 0 when orthogonal
	float dircos12 = dotprod12 / (norm1 * norm2);
	float dircos23 = dotprod23 / (norm2 * norm3);

	return (1 - distdirratio) * (dircos12 + dircos23 + 2) / 4 +
		   distdirratio * (1 - norm2 / maxdist);
}

#define jt2string(j)                                                   \
	((j) == FF                                                         \
		 ? "FF"                                                        \
		 : ((j) == FB                                                  \
				? "FB"                                                 \
				: ((j) == BF                                           \
					   ? "BF"                                          \
					   : ((j) == BB ? "BB" : ((j) == NOJOIN ? "NOJOIN" \
															: "!!invalid")))))

// FIXME !!! - remove the recursion, then lower the NPOINTS_MAX to its optimal
// value 35
// stack is not always big enough...
bool Polygons::splitlist(JOINENDPOINTLIST& pl, JOINOPLIST& ops,
						 const dpoint_t& min, const dpoint_t& max,
						 bool vertical, ProgressObserver* progressObserver,
						 const double pBase, const double piece) const
{
	dpoint_t min1, max1, min2, max2;
	JOINENDPOINTLIST pl1, pl2;

	pl1.reserve(pl.size() / 2);
	pl2.reserve(pl.size() / 2);

	if (vertical)
	{
		min1.x = min.x;
		max1.x = min2.x = min.x + (max.x - min.x) / 2;
		max2.x = max.x;
		min1.y = min2.y = min.y;
		max1.y = max2.y = max.y;
	}
	else
	{
		min1.y = min.y;
		max1.y = min2.y = min.y + (max.y - min.y) / 2;
		max2.y = max.y;
		min1.x = min2.x = min.x;
		max1.x = max2.x = max.x;
	}

	if (vertical)
	{
		double x1bound = max1.x + maxdist / 2, x2bound = min2.x - maxdist / 2;
		for (JOINENDPOINTLIST::const_iterator i = pl.begin(); i != pl.end();
			 ++i)
		{
			if (i->coords.x <= x1bound) pl1.push_back(*i);
			if (i->coords.x >= x2bound) pl2.push_back(*i);
		}
	}
	else
	{
		double y1bound = max1.y + maxdist / 2, y2bound = min2.y - maxdist / 2;
		for (JOINENDPOINTLIST::const_iterator i = pl.begin(); i != pl.end();
			 ++i)
		{
			if (i->coords.y <= y1bound) pl1.push_back(*i);
			if (i->coords.y >= y2bound) pl2.push_back(*i);
		}
	}

	JOIN_DEBUG_PRINT(
		"[%f, %f, %f, %f] -> [%f, %f, %f, %f]v[%f, %f, %f, %f] %d=>%d+%d",
		min.x, min.y, max.x, max.y, min1.x, min1.y, max1.x, max1.y, min2.x,
		min2.y, max2.x, max2.y, int(pl.size()), int(pl1.size()),
		int(pl2.size()));

	pl.clear();

	vertical = !vertical;
	return compdists(pl1, ops, min1, max1, vertical, progressObserver, pBase,
					 piece > 1 ? piece / 2 : 1) &&
		   compdists(pl2, ops, min2, max2, vertical, progressObserver,
					 pBase + piece / 2.0, piece > 1 ? piece / 2 : 1);
}

bool Polygons::compdists(JOINENDPOINTLIST& pl, JOINOPLIST& ops,
						 const dpoint_t& min, const dpoint_t& max,
						 bool vertical, ProgressObserver* progressObserver,
						 const double pBase, const double piece) const
{
	int cntr = 0, npoints = pl.size();
	int progressHowOften = (npoints / piece >= 1) ? int(npoints / piece) : 1;
	bool cancel = false;

	// split the list whenever it's too long
	if (npoints > NPOINTS_MAX)
	{
		JOIN_DEBUG_PRINT("splitting");
		splitlist(pl, ops, min, max, vertical, progressObserver, pBase, piece);
	}
	else
	{
		double maxDistSqr = maxdist * maxdist;
		vector<bool> alreadyUsed(npoints, false);
		vector<bool>::iterator ai = alreadyUsed.begin(), aj;

		JOIN_DEBUG_PRINT("computing set of %d points", npoints);
		for (JOINENDPOINTLIST::const_iterator i = pl.begin();
			 i != pl.end() && !cancel; ++i, ++ai)
		{
			privcurve_t* curve = &i->path->priv->curve;
			const dpoint_t* start = &i->coords;
			int nJoins = 0;

			// Closed curves cannot be joined. Points outside the bounding
			// rectangle
			// are not considered for joining.
			if (curve->closed || start->x < min.x || start->x > max.x ||
				start->y < min.y || start->y > max.y)
				continue;

			aj = ai + 1;
			for (JOINENDPOINTLIST::const_iterator j = i + 1; j != pl.end();
				 ++j, ++aj)
			{
				if (distSqr(start, &j->coords) < maxDistSqr)
				{
					dpoint_t *a, *b, *c, *d;
					privcurve_t* pp_curve = &j->path->priv->curve;

					switch (i->end)
					{
					case FRONT:
						b = &curve->vertex[0];
						a = b + 1;
						break;
					case BACK:
						b = &curve->vertex[curve->n - 1];
						a = b - 1;
						break;
					default:
						throw logic_error("NOEND in JOINENDPOINT list");
					}
					switch (j->end)
					{
					case FRONT:
						c = &pp_curve->vertex[0];
						d = c + 1;
						break;
					case BACK:
						c = &pp_curve->vertex[pp_curve->n - 1];
						d = c - 1;
						break;
					default:
						throw logic_error("NOEND in JOINENDPOINT list");
					}

					ops.push_back(JOINOP(dstfun(a, b, c, d)
											 // self-connection penalization
											 - (i->path == j->path),
										 endsToType(i->end, j->end), i->path,
										 j->path));
					nJoins++;
					*aj = true;
				}
			}

			if (nJoins == 1 && !*ai)
			{
				// simple connection
				ops.back().simple = true;
			}

			if (progressObserver && !(++cntr % progressHowOften))
			{
				progressObserver->percentageChanged(
					int(cntr * piece / npoints + pBase));
				cancel = progressObserver->getCancelPressed();
			}
			if (cancel) return false;
		}
	}
	if (progressObserver)
	{
		progressObserver->percentageChanged(int(piece + pBase));
	}
	return true;
}

//! \brief Joining of polygons.
// \alert Side effect: plist variable can be changed when removing the fist
// element on list.
bool Polygons::joinPolygons(path_t*& plist,
							ProgressObserver* progressObserver) const
{
	JOINOPLIST ops;
	JOINENDPOINTLIST pointlist;
	path_t* p;
	int nops, cntr, progressHowOften;
	bool cancel = false;
	double inf = std::numeric_limits<double>::infinity();
	dpoint_t min = {inf, inf}, max = {-inf, -inf};

	list_forall(p, plist)
	{
		privcurve_t* p_curve = &p->priv->curve;
		int p_n = p_curve->n;
		dpoint_t f = p_curve->vertex[0], b = p_curve->vertex[p_n - 1];
		pointlist.push_back(JOINENDPOINT(f, FRONT, p));
		pointlist.push_back(JOINENDPOINT(b, BACK, p));
		if (f.x > max.x) max.x = f.x;
		if (f.x < min.x) min.x = f.x;
		if (f.y > max.y) max.y = f.y;
		if (f.y < min.y) min.y = f.y;
		if (b.x > max.x) max.x = b.x;
		if (b.x < min.x) min.x = b.x;
		if (b.y > max.y) max.y = b.y;
		if (b.y < min.y) min.y = b.y;
	}

	compdists(pointlist, ops, min, max, true, progressObserver, 50, 12);

	sort(ops.begin(), ops.end(), greater_weight());

	nops = ops.size();
	cntr = 0;
	progressHowOften = (nops > 28) ? nops / 28 : 1;
	for (unsigned i = 0; i < ops.size() && !cancel; ++i)
	{
		auto& currOp = ops[i];

		if (progressObserver && !((++cntr) % progressHowOften))
		{
			progressObserver->percentageChanged(cntr * 28 / nops + 62);
			cancel = progressObserver->getCancelPressed();
		}

		if (simpleonly && !currOp.simple) continue;

		JOINEND jeA = joinEndA(currOp.joinType);
		JOINEND jeB = joinEndB(currOp.joinType);

		QString oshead, os;
		if (JOIN_DEBUG)
		{
			oshead = QString("%1. %2 %3")
						 .arg(i)
						 .arg(jt2string(currOp.joinType))
						 .arg(currOp.weight);
		}

		if (i)
		{
			bool cantJoin = false;

			// check how previous operations interfere with the current one
			for (unsigned r = 0; r < i && !cantJoin; ++r)
			{
				auto& checkedOp = ops[r];

				if (simpleonly && !checkedOp.simple) continue;

				int oslen;
				if (JOIN_DEBUG)
				{
					os = QString("  %1. %2 %3 ")
							 .arg(r)
							 .arg(jt2string(checkedOp.joinType))
							 .arg(checkedOp.weight);
					oslen = os.length();
				}

				// Curve A never reverses. There can only the "supposed to join"
				// end disappear.
				if ((currOp.a == checkedOp.a &&
					 joinEndA(checkedOp.joinType) == jeA) ||
					(currOp.b == checkedOp.a &&
					 joinEndA(checkedOp.joinType) == jeB) ||
					// Also curve B end can disappear
					(currOp.a == checkedOp.b &&
					 joinEndB(checkedOp.joinType) == jeA) ||
					(currOp.b == checkedOp.b &&
					 joinEndB(checkedOp.joinType) == jeB))
				{
					if (JOIN_DEBUG) os += "end destroyed; ";
					cantJoin = true;
				}
				// In case the curve is operation's curve B it becomes a part of
				// curve A
				// and it can be reversed (see bellow)
				else if (currOp.a == checkedOp.b)
				{
					// Curve B always becomes a part of curve A
					currOp.a = checkedOp.a;
					if (JOIN_DEBUG) os += "A curve merged";

					// in case of FF and BB joins, curve B becomes reversed
					if (joinEndA(checkedOp.joinType) ==
						joinEndB(checkedOp.joinType))
					{
						jeA = oppositeEnd(jeA);
						if (JOIN_DEBUG) os += " and reversed";
					}
					if (JOIN_DEBUG) os += "; ";
				}
				else if (currOp.b == checkedOp.b)
				{
					currOp.b = checkedOp.a;
					if (JOIN_DEBUG) os += "B curve merged";

					if (joinEndA(checkedOp.joinType) ==
						joinEndB(checkedOp.joinType))
					{
						jeB = oppositeEnd(jeB);
						if (JOIN_DEBUG) os += " and reversed";
					}
					if (JOIN_DEBUG) os += "; ";
				}

				if (JOIN_DEBUG && oslen != os.length())
				{
					if (!oshead.isEmpty())
					{
						qDebug("%s", oshead.toLatin1().constData());
						oshead = QString();
					}
					qDebug("%s", os.toLatin1().constData());
				}
			}

			if (cantJoin)
			{
				// remove the unrealized operation from list
				ops.erase(ops.begin() + i);
				i--;
				continue;
			}
		}

		// pointers to s path parts were updated earlier
		// update the join type
		currOp.joinType = endsToType(jeA, jeB);

		// are we operating on two ends of a single segment?
		// if yes, give preference to joining with another segment to
		// prevent loop formation
		if (currOp.a == currOp.b)
		{
			if (JOIN_DEBUG && !oshead.isEmpty())
				qDebug("%s", oshead.toLatin1().constData());

			if (!currOp.rescheduled)
			{
				JOIN_DEBUG_PRINT("  curve closing delayed");
				currOp.rescheduled = true;
				ops.push_back(currOp);
				ops.erase(ops.begin() + i);
				i--;
			}
			else
			{
				JOIN_DEBUG_PRINT("  curve closed, jointype %s",
								 jt2string(currOp.joinType));
				currOp.a->priv->curve.closed = 1;
			}
			continue;
		}

		// perform the actual join
		privcurve_t* a_curve = &currOp.a->priv->curve;
		privcurve_t* b_curve = &currOp.b->priv->curve;

		privcurve_t newcurve;
		privcurve_init(&newcurve, a_curve->n + b_curve->n);

		switch (currOp.joinType)
		{
		case FF: // reverse the segment prior to joining
			for (int d = 0; d < b_curve->n; d++)
				newcurve.vertex[d] = b_curve->vertex[b_curve->n - d - 1];
			Q_FALLTHROUGH();
		case FB:
			if (currOp.joinType == FB)
				for (int d = 0; d < b_curve->n; d++)
					newcurve.vertex[d] = b_curve->vertex[d];
			for (int d = 0; d < a_curve->n; d++)
				newcurve.vertex[d + b_curve->n] = a_curve->vertex[d];
			break;
		case BB:
			for (int d = 0; d < b_curve->n; d++)
				newcurve.vertex[d + a_curve->n] =
					b_curve->vertex[b_curve->n - d - 1];
			Q_FALLTHROUGH();
		case BF:
			if (currOp.joinType == BF)
				for (int d = 0; d < b_curve->n; d++)
					newcurve.vertex[d + a_curve->n] = b_curve->vertex[d];
			for (int d = 0; d < a_curve->n; d++)
				newcurve.vertex[d] = a_curve->vertex[d];
			break;
		case NOJOIN:
			throw logic_error("NOJOIN operation in JOINOP list");
		default:
			throw logic_error("invalid JOINOP");
		}

		privcurve_free_members(a_curve);
		memcpy(a_curve, &newcurve, sizeof(privcurve_t));

		path_t* ib = currOp.b;
		list_unlink(path_t, plist, ib);
		if (!ib) throw logic_error("unlinked ib not in list");
		path_free(ib);
	}

	return !cancel;
}
} // cove

//@}
