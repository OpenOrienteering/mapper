/*
 *    Copyright 2013 Thomas Schöps
 *    Copyright 2013, 2014, 2017-2019 Kai Pastor
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


#ifndef OPENORIENTEERING_CUTOUT_OPERATION_H
#define OPENORIENTEERING_CUTOUT_OPERATION_H

#include <vector>

#include "core/objects/boolean_tool.h"

namespace OpenOrienteering {

class AddObjectsUndoStep;
class Map;
class Object;
class PathObject;
class UndoStep;


/**
 * Operation to make map cutouts.
 * 
 * This functor must not be applied to map parts other than the current one.
 * 
 * See CutoutTool::apply for usage example.
 */
class CutoutOperation
{
public:
	/**
	 * Function object constructor.
	 * 
	 * Setting cut_away to true inverts the tool's effect, i.e. it will remove
	 * the object parts inside the outline of cutout_object instead of the
	 * object parts outside.
	 */
	CutoutOperation(Map* map, PathObject* cutout_object, bool cut_away);
	
	/** 
	 * This functor must not be copied.
	 * 
	 * If it is to be used via std::function, std::ref can be used to explicitly
	 * create a copyable reference.
	 */
	CutoutOperation(const CutoutOperation&) = delete;
	
	/**
	 * Destructs the object.
	 * 
	 * commit() must be called before.
	 */
	~CutoutOperation();
	
	/** 
	 * This functor must not be assigned to.
	 */
	CutoutOperation& operator=(const CutoutOperation&) = delete;
	
	/**
	 * Applies the configured cutting operation on the given object.
	 */
	void operator()(Object* object);
	
	/**
	 * Commits the changes.
	 * 
	 * This must always be called before the destructor.
	 * No other operations may be called after commit.
	 */
	void commit();
	
private:
	UndoStep* finish();
	
	Map* map;
	PathObject* cutout_object;
	std::vector<PathObject*> new_objects;
	AddObjectsUndoStep* add_step;
	BooleanTool boolean_tool;
	BooleanTool::PathObjects out_objects;
	bool cut_away;
};


}  // namespace OpenOrienteering

#endif
