/*
 *    Copyright 2012, 2013 Thomas Schoeps
 *    Copyright 2017 Kai Pastor
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


#ifndef OPENORIENTEERING_OBJECT_OPERATIONS_H
#define OPENORIENTEERING_OBJECT_OPERATIONS_H

#include "core/map.h"
#include "core/objects/object.h"
#include "core/symbols/symbol.h"

namespace OpenOrienteering {

/**
 * Object conditions and processors,
 * see methods Map::applyOnAllObjects() and MapPart::applyOnAllObjects()
 */
namespace ObjectOp
{
	// Conditions
	
	/** Returns true for objects with the given symbol. */
	struct HasSymbol
	{
		const Symbol* symbol;
		
		bool operator()(const Object* object) const noexcept
		{
			return object->getSymbol() == symbol;
		}
	};
	
	/** Returns true for objects with the given symbol type. */
	struct HasSymbolType
	{
		Symbol::Type type;
		
		bool operator()(const Object* object) const noexcept
		{
			return object->getSymbol()->getType() == type;
		}
	};
	
	/** Returns true for objects where the symbol type contains the given type. */
	struct ContainsSymbolType
	{
		Symbol::Type type;
		
		bool operator()(const Object* object) const noexcept
		{
			return object->getSymbol()->getContainedTypes() & type;
		}
	};
	
	
	// Operations
	
	/** Scales objects by the given factor. */
	struct Scale
	{
		double factor;
		MapCoordF center;
		
		void operator()(Object* object) const
		{
			object->scale(center, factor);
			object->update();
		}
	};
	
	/** Rotates objects by the given angle (in radians). */
	struct Rotate
	{
		double angle;
		MapCoordF center;
		
		void operator()(Object* object) const
		{
			object->rotateAround(center, angle);
			object->update();
		}
	};
	
	/**
	 * Changes the objects' symbols.
	 * NOTE: Make sure to apply this to correctly fitting objects only!
	 */
	struct ChangeSymbol
	{
		const Symbol* new_symbol;
		
		void operator()(Object* object, MapPart* part, int object_index) const
		{
			if (!object->setSymbol(new_symbol, false))
				part->deleteObject(object_index);
			else
				object->update();
		}
	};
	
	/** Delete objects. */
	struct Delete
	{
		void operator()(const Object*, MapPart* part, int object_index) const
		{
			part->deleteObject(object_index);
		}
	};
}


}  // namespace OpenOrienteering

#endif
