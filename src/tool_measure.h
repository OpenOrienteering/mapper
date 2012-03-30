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


#ifndef _OPENORIENTEERING_TOOL_MEASURE_H_
#define _OPENORIENTEERING_TOOL_MEASURE_H_

#include <QWidget>

#include "map_editor.h"

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE

class Map;

class MeasureWidget : public EditorDockWidgetChild
{
Q_OBJECT
public:
	MeasureWidget(Map* map, QWidget* parent = NULL);
	virtual ~MeasureWidget();
	
	//virtual QSize sizeHint() const;
	
protected slots:
	void objectSelectionChanged();
	
private:
	QLabel* label;
	Map* map;
};

#endif
