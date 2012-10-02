/*
 *    Copyright 2012 Thomas Schöps
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


#ifndef _OPENORIENTEERING_MAP_PART_H_
#define _OPENORIENTEERING_MAP_PART_H_

#include <vector>

#include <QString>
#include <QRect>
#include <QHash>
#include <QSet>
#include <QScopedPointer>

#include "global.h"
#include "undo.h"
#include "matrix.h"
#include "map_coord.h"

QT_BEGIN_NAMESPACE
class QIODevice;
class QPainter;
class QXmlStreamReader;
class QXmlStreamWriter;
QT_END_NAMESPACE

class Map;
struct MapColor;
class MapWidget;
class MapView;
class MapEditorController;
class Symbol;
class CombinedSymbol;
class LineSymbol;
class PointSymbol;
class Object;
class Renderable;
class MapRenderables;
class Template;
class OCAD8FileImport;
class Georeferencing;
class MapGrid;

typedef std::vector< std::pair< int, Object* > > SelectionInfoVector;

struct ObjectOperationResult
{
	enum Enum
	{
		NoResult = 0,
		Success = 1 << 0,
		Abort = 1 << 1
	};
};

class MapPart
{
friend class OCAD8FileImport;
public:
	MapPart(const QString& name, Map* map);
	~MapPart();
	
	void save(QIODevice* file, Map* map);
	bool load(QIODevice* file, int version, Map* map);
	void save(QXmlStreamWriter& xml, const Map& map) const;
	static MapPart* load(QXmlStreamReader& xml, Map& map);
	
	inline const QString& getName() const {return name;}
	inline void setName(const QString new_name) {name = new_name;}
	
	inline int getNumObjects() const {return (int)objects.size();}
	inline Object* getObject(int i) {return objects[i];}
    inline const Object* getObject(int i) const {return objects[i];}
	int findObjectIndex(Object* object);					// asserts that the object is contained in the part
	void setObject(Object* object, int pos, bool delete_old);
	void addObject(Object* object, int pos);
	void deleteObject(int pos, bool remove_only);
	bool deleteObject(Object* object, bool remove_only);	// returns if the object was found
	
	/// Imports the contents of the other part (which can be from another map) into this part.
	/// Uses symbol_map to replace all symbols contained there. No replacement is done for symbols which are not in the map.
	void importPart(MapPart* other, QHash<Symbol*, Symbol*>& symbol_map, bool select_new_objects);
	
	void findObjectsAt(MapCoordF coord, float tolerance, bool treat_areas_as_paths, bool extended_selection, bool include_hidden_objects, bool include_protected_objects, SelectionInfoVector& out);
	void findObjectsAtBox(MapCoordF corner1, MapCoordF corner2, bool include_hidden_objects, bool include_protected_objects, std::vector<Object*>& out);
	int countObjectsInRect(QRectF map_coord_rect, bool include_hidden_objects);
	
	QRectF calculateExtent(bool include_helper_symbols);
	
	/// See Map::operationOnAllObjects()
	template<typename Processor, typename Condition> ObjectOperationResult::Enum operationOnAllObjects(const Processor& processor, const Condition& condition)
	{
		int result = ObjectOperationResult::NoResult;
		int size = (int)objects.size();
		for (int i = size - 1; i >= 0; --i)
		{
			if (!condition(objects[i]))
				continue;
			result |= ObjectOperationResult::Success;
			
			if (!processor(objects[i], this, i))
			{
				result |= ObjectOperationResult::Abort;
				return (ObjectOperationResult::Enum)result;
			}
		}
		return (ObjectOperationResult::Enum)result;
	}
	/// See Map::operationOnAllObjects()
	template<typename Processor> ObjectOperationResult::Enum operationOnAllObjects(const Processor& processor)
	{
		int size = (int)objects.size();
		int result = size >= 1 ? ObjectOperationResult::Success : ObjectOperationResult::NoResult;
		for (int i = size - 1; i >= 0; --i)
		{
			if (!processor(objects[i], this, i))
			{
				result |= ObjectOperationResult::Abort;
				return (ObjectOperationResult::Enum)result;
			}
		}
		return (ObjectOperationResult::Enum)result;
	}
	void scaleAllObjects(double factor);
	void rotateAllObjects(double rotation);
	void updateAllObjects();
	void updateAllObjectsWithSymbol(Symbol* symbol);
	void changeSymbolForAllObjects(Symbol* old_symbol, Symbol* new_symbol);
	bool deleteAllObjectsWithSymbol(Symbol* symbol);		// returns if there was an object that was deleted
	bool doObjectsExistWithSymbol(Symbol* symbol);
	
private:
	QString name;
	std::vector<Object*> objects;	// TODO: this could / should be a spatial representation optimized for quick access
	Map* map;
};

#endif
