/*
 *    Copyright 2025 Matthias Kühlewein
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

#ifndef DYNAMIC_OBJECT_QUERY_H
#define DYNAMIC_OBJECT_QUERY_H

#include <QLatin1String>
#include <QString>
#include <QStringList>
#include <QStringRef>

namespace OpenOrienteering {

class Map;
class Object;
class PathObject;
class Symbol;

class DynamicObjectQuery
{
public:
	/** 
	 * Enumeration of all possible dynamic query types.
	 */
	enum Type
	{
		LineObjectQuery = 0,
		AreaObjectQuery = 1,
		SymbolQuery     = 2,
		ObjectQuery     = 3
	};
	
	DynamicObjectQuery(Type type) noexcept;
	~DynamicObjectQuery();
	
	Type getType() const { return type; }
	bool IsValid() const { return valid; }
	
protected:
	void parseTokenAttributes(QStringRef token_attributes_text);
	
	QStringList attributes;
	bool valid;
	
private:
	Type type;
};


class AreaObjectQuery : public DynamicObjectQuery
{
public:
	AreaObjectQuery(QStringRef token_attributes_text);
	
	static QString getKeyword() { return QLatin1String{"AREA"}; };
	
	bool performQuery(const PathObject* path_object) const;
};


class LineObjectQuery : public DynamicObjectQuery
{
public:
	LineObjectQuery(QStringRef token_attributes_text);
	
	static QString getKeyword() { return QLatin1String{"LINE"}; };
	
	bool performQuery(const PathObject* path_object) const;
};


class SymbolQuery : public DynamicObjectQuery
{
public:
	SymbolQuery(QStringRef token_attributes_text);
	
	static QString getKeyword() { return QLatin1String{"SYMBOL"}; };
	
	bool performQuery(const Map* map, const Symbol* symbol) const;
};

/*
class ObjectQuery : public DynamicObjectQuery
{
public:
	ObjectQuery(QStringRef token_attributes_text);
	
	static QString getKeyword() { return QLatin1String{"OBJECT"}; };
	
	bool performQuery(const Object* object) const;
};
*/

class DynamicObjectQueryManager
{
public:
	DynamicObjectQueryManager();
	
	static bool performDynamicQuery(const Object* object, const DynamicObjectQuery* dynamic_query);
	
	DynamicObjectQuery* parse(QStringRef token_text, QStringRef token_attributes_text);
	
};


}  // namespace OpenOrienteering

#endif // DYNAMIC_OBJECT_QUERY_H
