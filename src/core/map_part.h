/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2017 Kai Pastor
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


#ifndef OPENORIENTEERING_MAP_PART_H
#define OPENORIENTEERING_MAP_PART_H

#include <cstddef>
#include <functional>
#include <memory>
#include <vector>
#include <utility>

#include <QHash>
#include <QRectF>
#include <QString>

class QIODevice;
class QTransform;
class QXmlStreamReader;
class QXmlStreamWriter;
// IWYU pragma: no_forward_declare QRectF

namespace OpenOrienteering {

class Map;
class MapCoordF;
class Object;
class Symbol;
using SymbolDictionary = QHash<QString, Symbol*>; // from symbol.h
class UndoStep;


using SelectionInfoVector = std::vector<std::pair<int, Object*>> ;


/**
 * Represents a part of a map by owning a list of map objects.
 * 
 * Dividing maps in parts is e.g. useful to have multiple mappers work on a map:
 * every mapper can do the work in his/her part without getting into conflict
 * with other parts. For display, the objects from all parts are merged.
 * 
 * Another application is in course setting, where it is useful to have
 * a map part for event-specific map objects and parts for course-specific
 * map objects. Then a course can be printed by merging the event-specific part
 * with the part for the course.
 * 
 * Currently, only one map part can be used per map.
 */
class MapPart
{
friend class OCAD8FileImport;
public:
	/**
	 * Creates a new map part with the given name for a map.
	 */
	MapPart(const QString& name, Map* map);
	
	MapPart(const MapPart&) = delete;
	
	/**
	 * Destroys the map part.
	 */
	~MapPart();
	
	
	MapPart& operator=(const MapPart&) = delete;
	
	
	/**
	 * Loads the map part in the old "native" format from the given file.
	 */
	bool load(QIODevice* file, int version, Map* map);
	
	/**
	 * Saves the map part in xml format to the given stream.
	 */
	void save(QXmlStreamWriter& xml) const;
	
	/**
	 * Loads the map part in xml format from the given stream.
	 * 
	 * Needs a dictionary to map symbol ids to symbol pointers.
	 */
	static MapPart* load(QXmlStreamReader& xml, Map& map, SymbolDictionary& symbol_dict);
	
	/**
	 * Returns the part's name.
	 */
	const QString& getName() const;
	
	/**
	 * Sets the part's name.
	 */
	void setName(const QString& new_name);
	
	
	/**
	 * Returns the number of objects in the part.
	 */
	int getNumObjects() const;
	
	/**
	 * Returns the i-th object from the part.
	 */
	const Object* getObject(int i) const;
	
	/**
	 * Returns the i-th object from the part.
	 */
	Object* getObject(int i);
	
	/**
	 * Returns the index of the object.
	 * 
	 * Loops over all objects in the part and looks for the given pointer.
	 * The object must be contained in this part,
	 * otherwise an assert is triggered (in debug builds),
	 * or -1 is returned (release builds).
	 */
	int findObjectIndex(const Object* object) const;
	
	/**
	 * Replaces the object at the given index with another.
	 * 
	 * If delete_old is set, calls "delete old_object".
	 */
	void setObject(Object* object, int pos, bool delete_old);
	
	/**
	 * Adds the object as new object at the end.
	 */
	void addObject(Object* object);
	
	/**
	 * Adds the object as new object at the given index.
	 */
	void addObject(Object* object, int pos);
	
	/**
	 * Deleted the object from the given index.
	 * 
	 * If remove_only is set, does not call "delete object".
	 * 
	 * @todo Make a separate method "removeObject()", this is misleading!
	 */
	void deleteObject(int pos, bool remove_only);
	
	/**
	 * Deleted the object from the given index.
	 * 
	 * If remove_only is set, does not call "delete object".
	 * Returns if the object was found in this part.
	 * 
	 * @todo Make a separate method "removeObject()", this is misleading!
	 */
	bool deleteObject(Object* object, bool remove_only);
	
	
	/**
	 * Imports the contents another part into this part.
	 * 
	 * The other part can be from another map.
	 * Uses symbol_map to replace all symbols contained there.
	 * No replacement is done for symbols which are not in the symbol_map.
	 */
	std::unique_ptr<UndoStep> importPart(const MapPart* other, const QHash<const Symbol*, Symbol*>& symbol_map,
		const QTransform& transform, bool select_new_objects);
	
	
	/**
	 * @see Map::findObjectsAt().
	 */
	void findObjectsAt(MapCoordF coord, float tolerance, bool treat_areas_as_paths,
		bool extended_selection, bool include_hidden_objects,
		bool include_protected_objects, SelectionInfoVector& out) const;
	
	/**
	 * @see Map::findObjectsAtBox().
	 */
	void findObjectsAtBox(MapCoordF corner1, MapCoordF corner2,
		bool include_hidden_objects, bool include_protected_objects,
		std::vector<Object*>& out) const;
	
	/** 
	 * @see Map::countObjectsInRect().
	 */
	int countObjectsInRect(const QRectF& map_coord_rect, bool include_hidden_objects) const;
	
	/**
	 * Calculates and returns the bounding box of all objects in this map part.
	 */
	QRectF calculateExtent(bool include_helper_symbols) const;
	
	
	/**
	 * Applies a condition on all objects (until the first match is found).
	 * 
	 * @return True if there is an object matching the condition, false otherwise.
	 */
	bool existsObject(const std::function<bool (const Object*)>& condition) const;
	
	/**
	 * @copybrief   Map::applyOnMatchingObjects()
	 * @copydetails Map::applyOnMatchingObjects()
	 */
	void applyOnMatchingObjects(const std::function<void (Object*)>& operation, const std::function<bool (const Object*)>& condition);
	
	/**
	 * @copybrief   Map::applyOnMatchingObjects()
	 * @copydetails Map::applyOnMatchingObjects()
	 */
	void applyOnMatchingObjects(const std::function<void (Object*, MapPart*, int)>& operation, const std::function<bool (const Object*)>& condition);
	
	/**
	 * @copybrief   Map::applyOnAllObjects()
	 * @copydetails Map::applyOnAllObjects()
	 */
	void applyOnAllObjects(const std::function<void (Object*)>& operation);
	
	/**
	 * @copybrief   Map::applyOnAllObjects()
	 * @copydetails Map::applyOnAllObjects()
	 */
	void applyOnAllObjects(const std::function<void (Object*, MapPart*, int)>& operation);
	
	
private:
	typedef std::vector<Object*> ObjectList;

	QString name;
	ObjectList objects;  ///< @todo This could be a spatial representation optimized for quick access
	Map* const map;
};



// ## MapPart inline and template code ###

inline
const QString& MapPart::getName() const
{
	return name;
}

inline
int MapPart::getNumObjects() const
{
	return int(objects.size());
}

inline
Object* MapPart::getObject(int i)
{
	return objects[std::size_t(i)];
}

inline
const Object* MapPart::getObject(int i) const
{
	return objects[std::size_t(i)];
}


}  // namespace OpenOrienteering

#endif
