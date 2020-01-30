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

#ifndef COVE_POLYGONS_H
#define COVE_POLYGONS_H

#include <algorithm>
#include <functional>
#include <vector>

#include <QPointF>

#include "cove-potrace.h"

class QImage;
// IWYU pragma: no_forward_declare QPointF
class QRect;

namespace cove {
class ProgressObserver;

class Polygons
{
protected:
	bool simpleonly;
	unsigned specklesize;
	double maxdist, distdirratio;
	enum DIRECTION
	{
		NONE = 0,
		NORTH,
		EAST,
		SOUTH,
		WEST
	};
	struct PATH_POINT
	{
		int x, y;
	};

	class Path : public std::vector<PATH_POINT>
	{
		bool pathClosed;

	public:
		Path();
		void setClosed(bool closed);
		bool isClosed() const;
	};

	class PathList : public std::vector<Path>
	{
	};

public:
	class Polygon : public std::vector<QPointF>
	{
		bool polygonClosed;
		double minX, minY, maxX, maxY;

	public:
		Polygon();
		void push_back(const value_type& p);
		QRect boundingRect() const;
		void recheckBounds();
		void setClosed(bool closed);
		bool isClosed() const;
	};

	class PolygonList : public std::vector<Polygon>
	{
	};

private:
	enum JOINEND
	{
		NOEND,
		FRONT,
		BACK
	};

	enum JOINTYPE
	{
		NOJOIN,
		FF,
		FB,
		BF,
		BB
	};

	struct JOINOP
	{
		float weight;
		JOINTYPE joinType;
		path_t *a, *b;
		bool simple, rescheduled;
		JOINOP(float weight, JOINTYPE joinType, path_t* a, path_t* b)
			: weight(weight)
			, joinType(joinType)
			, a(a)
			, b(b)
			, simple(false)
			, rescheduled(false)
		{
		}
	};

	typedef std::vector<JOINOP> JOINOPLIST;

	struct JOINENDPOINT
	{
		dpoint_t coords;
		JOINEND end;
		path_t* path;
		JOINENDPOINT(const dpoint_t& a, JOINEND b, path_t* c)
			: coords(a)
			, end(b)
			, path(c)
		{
		}
	};

	typedef std::vector<JOINENDPOINT> JOINENDPOINTLIST;

	struct greater_weight
		: public std::binary_function<const JOINOP&, const JOINOP&, bool>
	{
		bool operator()(const JOINOP& x, const JOINOP& y)
		{
			return x.weight > y.weight;
		}
	};

	// implementation dependent value...
	static const int NPOINTS_MAX;

	bool compdists(JOINENDPOINTLIST& pl, JOINOPLIST& ops, const dpoint_t& min,
	               const dpoint_t& max, bool vertical,
	               ProgressObserver* progressObserver, double pBase,
	               double piece) const;
	bool splitlist(JOINENDPOINTLIST& pl, JOINOPLIST& ops, const dpoint_t& min,
	               const dpoint_t& max, bool vertical,
	               ProgressObserver* progressObserver, double pBase,
	               double piece) const;
	inline double distSqr(const dpoint_t* a, const dpoint_t* b) const;
	inline JOINEND joinEndA(JOINTYPE j) const;
	inline JOINEND joinEndB(JOINTYPE j) const;
	inline JOINEND oppositeEnd(JOINEND j) const;
	inline JOINTYPE endsToType(JOINEND ea, JOINEND eb) const;
	inline double dstfun(const dpoint_t* a, const dpoint_t* b, const dpoint_t* c,
	                     const dpoint_t* d) const;
	bool joinPolygons(path_t*& plist,
	                  ProgressObserver* progressObserver = nullptr) const;

protected:
	static bool findNextPixel(const QImage& image, int& xp, int& yp);
	static void followPath(const QImage& image, int& x, int& y, Path* path = nullptr);
	static Path recordPath(const QImage& image, int initX, int initY);
	static void removePathFromImage(QImage& image, const Path& path);
	PathList decomposeImageIntoPaths(const QImage& sourceImage,
	                                 ProgressObserver* progressObserver = nullptr) const;
	PolygonList getPathPolygons(const PathList& constpaths,
	                            ProgressObserver* progressObserver = nullptr) const;
	static double distance(const QPointF& a, const QPointF& b);

public:
	Polygons();
	void setSpeckleSize(int val);
	void setMaxDistance(double val);
	void setSimpleOnly(bool val);
	void setDistDirRatio(double val);
	int speckleSize() const;
	double maxDistance() const;
	bool simpleOnly() const;
	double distDirRatio() const;
	PolygonList
	createPolygonsFromImage(const QImage& image,
	                        ProgressObserver* progressObserver = nullptr) const;
	// search for best value of NPOINTS_MAX - optinpoints.cpp
	friend void perfcheck();
};
} // cove

#endif
