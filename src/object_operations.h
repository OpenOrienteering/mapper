/*
 *    Copyright 2012, 2013 Thomas Schoeps
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


#ifndef _OPENORIENTEERING_OBJECT_OPERATIONS_H_
#define _OPENORIENTEERING_OBJECT_OPERATIONS_H_

#include "symbol.h"
#include "object.h"
#include "map.h"

/**
 * Object conditions and processors,
 * see methods Map::applyOnAllObjects() and MapPart::applyOnAllObjects()
 */
namespace ObjectOp
{
	// Conditions
	
	/** Returns true for all objects. */
	struct NoCondition
	{
		inline NoCondition() {}
		inline bool operator()(Object* object) const
		{
			Q_UNUSED(object);
			return true;
		}
	};
	
	/** Returns true for objects with the given symbol. */
	struct HasSymbol
	{
		inline HasSymbol(const Symbol* symbol) : symbol(symbol) {}
		inline bool operator()(Object* object) const
		{
			return object->getSymbol() == symbol;
		}
	private:
		const Symbol* symbol;
	};
	
	/** Returns true for objects with the given symbol type. */
	struct HasSymbolType
	{
		inline HasSymbolType(Symbol::Type type) : type(type) {}
		inline bool operator()(Object* object) const
		{
			return object->getSymbol()->getType() == type;
		}
	private:
		Symbol::Type type;
	};
	
	/** Returns true for objects where the symbol type contains the given type. */
	struct ContainsSymbolType
	{
		inline ContainsSymbolType(Symbol::Type type) : type(type) {}
		inline bool operator()(Object* object) const
		{
			return object->getSymbol()->getContainedTypes() & type;
		}
	private:
		Symbol::Type type;
	};
	
	
	// Operations
	
	/** Scales objects by the given factor. */
	struct Scale
	{
		inline Scale(double factor, const MapCoord& scaling_center) : factor(factor), center(scaling_center) {}
		inline bool operator()(Object* object, MapPart* part, int object_index) const
		{
			Q_UNUSED(part);
			Q_UNUSED(object_index);
			object->scale(center, factor);
			object->update(true, true);
			return true;
		}
	private:
		double factor;
		MapCoordF center;
	};
	
	/** Rotates objects by the given angle (in radians). */
	struct Rotate
	{
		inline Rotate(double angle, const MapCoord& center) : angle(angle), center(center) {}
		inline bool operator()(Object* object, MapPart* part, int object_index) const
		{
			Q_UNUSED(part);
			Q_UNUSED(object_index);
			object->rotateAround(center, angle);
			object->update(true, true);
			return true;
		}
	private:
		double angle;
		MapCoordF center;
	};
	
	/** Calls update() on the objects. */
	struct Update
	{
		inline Update(bool force = true) : force(force) {}
		inline bool operator()(Object* object, MapPart* part, int object_index) const
		{
			Q_UNUSED(part);
			Q_UNUSED(object_index);
			object->update(force, true);
			return true;
		}
	private:
		bool force;
	};
	
	/**
	 * Changes the objects' symbols.
	 * NOTE: Make sure to apply this to correctly fitting objects only!
	 */
	struct ChangeSymbol
	{
		inline ChangeSymbol(const Symbol* new_symbol) : new_symbol(new_symbol) {}
		inline bool operator()(Object* object, MapPart* part, int object_index) const
		{
			if (!object->setSymbol(new_symbol, false))
				part->deleteObject(object_index, false);
			else
				object->update(true, true);
			return true;
		}
	private:
		const Symbol* new_symbol;
	};
	
	/** Delete objects. */
	struct Delete
	{
		inline Delete() {}
		inline bool operator()(Object* object, MapPart* part, int object_index) const
		{
			Q_UNUSED(object);
			part->deleteObject(object_index, false);
			return true;
		}
	};
	
	/**
	 * Can be used to check for the existence of certain types of objects
	 * by checking if this operation would be applied to any object
	 * under a given condition.
	 */
	struct NoOp
	{
		inline NoOp() {}
		inline bool operator()(Object* object, MapPart* part, int object_index) const
		{
			Q_UNUSED(object);
			Q_UNUSED(part);
			Q_UNUSED(object_index);
			// Abort
			return false;
		}
	};
}

#endif
