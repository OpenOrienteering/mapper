/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012, 2014, 2016 Kai Pastor
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


#ifndef OPENORIENTEERING_TRANSFORMATION_H
#define OPENORIENTEERING_TRANSFORMATION_H

#include <vector>

#include "core/map_coord.h"

class QTransform;
class QXmlStreamReader;
class QXmlStreamWriter;

namespace OpenOrienteering {

struct TemplateTransform;


/** Pair of source and destination coordinates used to calculate transformations. */
struct PassPoint
{
	void save(QXmlStreamWriter& xml) const;
	static PassPoint load(QXmlStreamReader& xml);
	
	/** Start position specified by the user */
	MapCoordF src_coords;
	
	/** End position specified by the user */
	MapCoordF dest_coords;
	
	/** Position where the point really ended up */
	MapCoordF calculated_coords;
	
	/** Distance between dest_coords_map and calculated_coords;
	 *  negative if not calculated yet */
	double error;
};

/** List of pass points with methods for transformation calculation. */
class PassPointList : public std::vector< PassPoint >
{
public:
	/**
	 * Indicates arguments which must not be nullptr.
	 * \todo Use the Guideline Support Library
	 */
	template <typename T>
	using not_null = T;
	
	
	/** Estimates a similarity transformation based on the contained pass points
	 *  and applies it to the transformation passed in. */
	bool estimateSimilarityTransformation(not_null<TemplateTransform*> transform);
	
	bool estimateSimilarityTransformation(not_null<QTransform*> out);
	
	/** Estimates an affine transformation without shearing. */
	bool estimateNonIsometricSimilarityTransform(not_null<QTransform*> out);
};


}  // namespace OpenOrienteering

#endif
