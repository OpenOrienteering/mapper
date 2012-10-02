/*
 *    Copyright 2012 Thomas Schoeps
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

// Object conditions and processors, see methods Map::operationOnAllObjects() and MapPart::operationOnAllObjects()
namespace ObjectOp
{
	// Conditions
	
	struct NoCondition
	{
		inline NoCondition() {}
		inline bool operator()(Object* object) const
		{
			return true;
		}
	};
	
	struct HasSymbol
	{
		inline HasSymbol(Symbol* symbol) : symbol(symbol) {}
		inline bool operator()(Object* object) const
		{
			return object->getSymbol() == symbol;
		}
	private:
		Symbol* symbol;
	};
	
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
	
	struct Scale
	{
		inline Scale(double factor) : factor(factor) {}
		inline bool operator()(Object* object, MapPart* part, int object_index) const
		{
			object->scale(factor);
			object->update(true, true);
			return true;
		}
	private:
		double factor;
	};
	
	struct Rotate
	{
		inline Rotate(double angle) : angle(angle) {}
		inline bool operator()(Object* object, MapPart* part, int object_index) const
		{
			object->rotateAround(MapCoordF(0, 0), angle);
			object->update(true, true);
			return true;
		}
	private:
		double angle;
	};
	
	struct Update
	{
		inline Update(bool force = true) : force(force) {}
		inline bool operator()(Object* object, MapPart* part, int object_index) const
		{
			object->update(force, true);
			return true;
		}
	private:
		bool force;
	};
	
	// NOTE: Make sure to apply this to correctly fitting objects only!
	struct ChangeSymbol
	{
		inline ChangeSymbol(Symbol* new_symbol) : new_symbol(new_symbol) {}
		inline bool operator()(Object* object, MapPart* part, int object_index) const
		{
			if (!object->setSymbol(new_symbol, false))
				part->deleteObject(object_index, false);
			else
				object->update(true, true);
			return true;
		}
	private:
		Symbol* new_symbol;
	};
	
	struct Delete
	{
		inline Delete() {}
		inline bool operator()(Object* object, MapPart* part, int object_index) const
		{
			part->deleteObject(object_index, false);
			return true;
		}
	};
	
	// Can be used to check for the existence of certain types of objects by checking if this operation would be applied to any object under a given condition
	struct NoOp
	{
		inline NoOp() {}
		inline bool operator()(Object* object, MapPart* part, int object_index) const
		{
			// Abort
			return false;
		}
	};
}

#endif
