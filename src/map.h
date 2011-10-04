/*
 *    Copyright 2011 Thomas Schöps
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


#ifndef _OPENORIENTEERING_MAP_H_
#define _OPENORIENTEERING_MAP_H_

#include <vector>

#include <QString>
#include <QColor>

QT_BEGIN_NAMESPACE
class QFile;
QT_END_NAMESPACE

class MapWidget;

/// Central class for an OpenOrienteering map
class Map : public QObject
{
Q_OBJECT
public:
	struct Color
	{
		void updateFromCMYK();
		void updateFromRGB();
		
		QString name;
		int priority;
		
		// values are in range [0; 1]
		float c, m, y, k;
		float r, g, b;
		float opacity;
		
		QColor color;
	};
	
	/// Creates a new, empty map
	Map();
	~Map();
	
	/// Attempts to save the map to the given file.
	bool saveTo(const QString& path);
	/// Attempts to load the map from the specified path. Returns true on success.
	bool loadFrom(const QString& path);
	
	/// Must be called to notify the map of new widgets displaying it. Useful to notify the widgets about which parts of the map have changed and need to be redrawn
	void addMapWidget(MapWidget* widget);
	void removeMapWidget(MapWidget* widget);
	/// Redraws all map widgets completely - that can be slow!
	void updateAllMapWidgets();
	
	// Colors
	
	inline int getNumColors() const {return (int)colors.size();}
	inline Color* getColor(int i) {return colors[i];}
	void setColor(Color* color, int pos);
	Color* addColor(int pos);
	void addColor(Color* color, int pos);
	void deleteColor(int pos);
	void setColorsDirty();
	
	inline void setScaleDenominator(int value) {scale_denominator = value;}
	inline int getScaleDenominator() const {return scale_denominator;}
	
signals:
	void gotUnsavedChanges();
	
private:
	typedef std::vector<Color*> ColorVector;
	typedef std::vector<MapWidget*> WidgetVector;
	
	void checkIfFirstColorAdded();
	void saveString(QFile* file, const QString& str);
	void loadString(QFile* file, QString& str);
	
	ColorVector colors;
	WidgetVector widgets;
	
	bool colors_dirty;		// are there unsaved changes for the colors?
	bool unsaved_changes;	// are there unsaved changes for any component?
	
	int scale_denominator;	// this is the number x if the scale is written as 1:x
};

/// Stores view position, zoom and template visibilities to define a view onto a map
class MapView
{
public:
	/// Creates a default view looking at the origin
	MapView(Map* map);
	
	inline Map* getMap() const {return map;}
	
private:
	Map* map;
};

#endif
