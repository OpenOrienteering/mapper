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


#include "tool_measure.h"

#include <QtGui>

#include "map.h"
#include "global.h"
#include "object.h"
#include "symbol.h"

MeasureWidget::MeasureWidget(Map* map, QWidget* parent) : EditorDockWidgetChild(parent), map(map)
{
	label = new QLabel();
	
	QVBoxLayout* layout = new QVBoxLayout();
	layout->addWidget(label);
	setLayout(layout);
	
	connect(map, SIGNAL(objectSelectionChanged()), this, SLOT(objectSelectionChanged()));
	connect(map, SIGNAL(selectedObjectEdited()), this, SLOT(objectSelectionChanged()));
	objectSelectionChanged();
}
MeasureWidget::~MeasureWidget()
{
}
/*QSize MeasureWidget::sizeHint() const
{
	return QSize(0, 0);
}*/

void MeasureWidget::objectSelectionChanged()
{
	QString text = "";
	
	if (map->getNumSelectedObjects() == 0)
		text = "<b>" + tr("No objects selected.<br/>Draw or select a path<br/>to measure it.") + "</b>";
	else if (map->getNumSelectedObjects() > 1)
		text = tr("%1 objects selected.").arg("<b>" + QString::number(map->getNumSelectedObjects()) + "</b>");
	else
	{
		Object* object = *map->selectedObjectsBegin();
		if (object->getType() != Object::Path)
			text = "<b>" + tr("Selected object is not a path.") + "</b>";
		else
		{
			PathObject* path = reinterpret_cast<PathObject*>(object);
			path->update();
			
			QLocale locale;
			double mm_to_meters = 0.001 * map->getScaleDenominator();
			
			assert(path->getNumParts() > 0);
			double length_mm = path->getPart(0).getLength();
			double length_meters = length_mm * mm_to_meters;
			
			bool contains_area = path->getSymbol()->getContainedTypes() & Symbol::Area;
			QString length_string;
			if (contains_area)
				length_string = tr("Boundary length");
			else
				length_string = tr("Length");
			
			text += length_string + " [" + tr("mm") + "]: <b>" + locale.toString(length_mm) + "</b><br/>" +
			        length_string + " [" + tr("meters") + "]: <b>" + locale.toString(length_meters) + "</b>";
			
			if (contains_area)
			{
				double area_mm = 0;
				area_mm += path->getPart(0).calculateArea();
				for (int i = 1; i < path->getNumParts(); ++ i)
					area_mm -= path->getPart(i).calculateArea();
				double area_meters = area_mm * mm_to_meters * mm_to_meters;
				
				QString area_string = tr("Area");
				text += "<br/>" + area_string + " [" + tr("mm") + "]: <b>" + locale.toString(area_mm) + "</b><br/>" +
				        area_string + " [" + tr("meters") + "]: <b>" + locale.toString(area_meters) + "</b><br/>";
				text += tr("<br/><u>Warning</u>: area is only correct if<br/>there are no self-intersections and<br/>holes are used as such.");
			}
		}
	}
	
	label->setText(text);
}
