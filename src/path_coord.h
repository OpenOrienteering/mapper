/*
 *    Copyright 2012, 2013 Thomas Schöps
 *
 *    This file is part of OpenOrienteering.
 *
 *    OpenOrienteering is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    OpenOrienteering is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with OpenOrienteering.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef _OPENORIENTEERING_PATH_COORD_H_
#define _OPENORIENTEERING_PATH_COORD_H_

#include "map_coord.h"

struct PathCoord;
typedef std::vector<PathCoord> PathCoordVector;

/// Paths that may consist of polygonal segments and curves are processed into PathCoordVectors,
/// approximating the path only with polygonal segments and containing information about lengths etc.
struct PathCoord
{
	MapCoordF pos;
	float clen;		// cumulative length since path part start
	int index;		// index into the MapCoordVector(F) to the first coordinate of the segment which contains this PathCoord
	float param;	// value of the curve parameter for this position
	
	// TODO: make configurable
	static const float bezier_error;
	static const float bezier_segment_maxlen;
	
	static void calculatePathCoords(const MapCoordVector& flags, const MapCoordVectorF& coords, PathCoordVector* path_coords);
	static bool getNextPathPart(const MapCoordVector& flags, const MapCoordVectorF& coords, int& part_start, int& part_end, PathCoordVector* path_coords, bool break_at_dash_points, bool append_path_coords);
	static PathCoord findPathCoordForCoorinate(const PathCoordVector* path_coords, int index);
	
	static void calculatePositionAt(const MapCoordVector& flags, const MapCoordVectorF& coords, const PathCoordVector& path_coords, float length, int& line_coord_search_start, MapCoordF* out_pos, MapCoordF* out_right_vector);
	static MapCoordF calculateRightVector(const MapCoordVector& flags, const MapCoordVectorF& coords, bool path_closed, int i, float* scaling);
	static MapCoordF calculateTangent(const MapCoordVector& coords, int i, bool backward, bool& ok);
	static MapCoordF calculateTangent(const MapCoordVectorF& coords, int i, bool backward, bool& ok);
	static MapCoordF calculateIncomingTangent(const MapCoordVectorF& coords, bool path_closed, int i, bool& ok);
	static MapCoordF calculateOutgoingTangent(const MapCoordVectorF& coords, bool path_closed, int i, bool& ok);
	static void splitBezierCurve(MapCoordF c0, MapCoordF c1, MapCoordF c2, MapCoordF c3, float p, MapCoordF& o0, MapCoordF& o1, MapCoordF& o2, MapCoordF& o3, MapCoordF& o4);
	
private:
	static void curveToPathCoordRec(MapCoordF c0, MapCoordF c1, MapCoordF c2, MapCoordF c3, int coord_index, float max_error, float max_segment_len, PathCoordVector* path_coords, float p0, float p1);
	static void curveToPathCoord(MapCoordF c0, MapCoordF c1, MapCoordF c2, MapCoordF c3, int coord_index, float max_error, float max_segment_len, PathCoordVector* path_coords);
};

#endif
