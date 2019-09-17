/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2015-2017 Kai Pastor
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

#ifndef OPENORIENTEERING_OBJECT_SELECTOR_H
#define OPENORIENTEERING_OBJECT_SELECTOR_H

#include <utility>
#include <vector>

namespace OpenOrienteering {

class Map;
class MapCoordF;
class Object;

using SelectionInfoVector = std::vector<std::pair<int, Object*>>;


/**
 * Implements the object selection logic for edit tools.
 */
class ObjectSelector
{
public:
	/** Creates a selector for the given map. */
	ObjectSelector(Map* map);
	
	/**
	 * Selects an object at the given position.
	 * If there is already an object selected at this position, switches through
	 * the available objects.
	 * @param tolerance maximum, normal selection distance in map units.
	 *    It is enlarged by 1.5 if no objects are found with the normal distance.
	 * @param toggle corresponds to the shift key modifier.
	 * @return true if the selection has changed.
	 */
	bool selectAt(const MapCoordF& position, double tolerance, bool toggle);
	
	/**
	 * Applies box selection.
	 * @param toggle corresponds to the shift key modifier.
	 * @return true if the selection has changed.
	 */
	bool selectBox(const MapCoordF& corner1, const MapCoordF& corner2, bool toggle);
	
	// TODO: move to other place? util.h/cpp or object.h/cpp
	static bool sortObjects(const std::pair<int, Object*>& a, const std::pair<int, Object*>& b);
	
private:
	bool selectionInfosEqual(const SelectionInfoVector& a, const SelectionInfoVector& b);
	
	// Information about the last click
	SelectionInfoVector last_results;
	SelectionInfoVector last_results_ordered;
	SelectionInfoVector::size_type next_object_to_select;
	
	Map* map;
};


}  // namespace OpenOrienteering

#endif
