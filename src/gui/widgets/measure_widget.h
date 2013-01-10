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


#ifndef _OPENORIENTEERING_MEASURE_WIDGET_H_
#define _OPENORIENTEERING_MEASURE_WIDGET_H_

#include <QSize>
#include <QWidget>

class QLabel;
class QStackedWidget;

class Map;

class MeasureWidget : public QWidget
{
Q_OBJECT
public:
	MeasureWidget(Map* map, QWidget* parent = NULL);
	virtual ~MeasureWidget();
	
	virtual QSize sizeHint() const;
	
protected slots:
	void objectSelectionChanged();
	
private:
	Map* map;
	
	QSize preferred_size;
	
	QLabel* headline_label;
	QStackedWidget* length_stack;
	QLabel* paper_length_label;
	QLabel* real_length_label;
	QStackedWidget* area_stack;
	QLabel* paper_area_label;
	QLabel* real_area_label;
	QLabel* warning_label;
};

#endif
