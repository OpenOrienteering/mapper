/*
 *    Copyright 2012, 2013 Thomas Schöps
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

#include <cassert>

#if QT_VERSION < 0x050000
#include <QtGui>
#else
#include <QtWidgets>
#endif

#include "map.h"
#include "global.h"
#include "object.h"
#include "symbol.h"
#include "symbol_area.h"
#include "symbol_line.h"

MeasureWidget::MeasureWidget(Map* map, QWidget* parent)
: QWidget(parent),
  map(map)
{
	QSettings settings;
	settings.beginGroup("MeasureWidget");
	preferred_size = settings.value("size", QSize(250, 150)).toSize();
	settings.endGroup();
	
	QGridLayout* layout = new QGridLayout();
	setLayout(layout);
	
	int margin = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
	layout->setContentsMargins(margin, margin, margin, margin);
	
	headline_label = new QLabel();
	layout->addWidget(headline_label, 0, 0, 1, -1);
	
	length_stack = new QStackedWidget();
	length_stack->addWidget(new QLabel(""));
	length_stack->addWidget(new QLabel(tr("Boundary length:"))); // for area
	length_stack->addWidget(new QLabel(tr("Length:")));          // for line
	layout->addWidget(length_stack, 1, 0);
	paper_length_label = new QLabel();
	layout->addWidget(paper_length_label, 1, 1);
	real_length_label = new QLabel();
	layout->addWidget(real_length_label, 1, 2);
	
	area_stack = new QStackedWidget();
	area_stack->addWidget(new QLabel(""));
	area_stack->addWidget(new QLabel(tr("Area:")));
	layout->addWidget(area_stack, 2, 0);
	paper_area_label = new QLabel();
	layout->addWidget(paper_area_label, 2, 1);
	real_area_label = new QLabel();
	layout->addWidget(real_area_label, 2, 2);
	
	warning_label = new QLabel();
	warning_label->setWordWrap(true);
	layout->addWidget(warning_label, 3, 0, 1, -1);
	layout->setRowStretch(3, 1);
	
	layout->setColumnStretch(1, 1);
	layout->setColumnStretch(2, 1);
	
	connect(map, SIGNAL(objectSelectionChanged()), this, SLOT(objectSelectionChanged()));
	connect(map, SIGNAL(selectedObjectEdited()), this, SLOT(objectSelectionChanged()));
	connect(map, SIGNAL(symbolChanged(int,Symbol*,Symbol*)), this, SLOT(objectSelectionChanged()));
	objectSelectionChanged();
}

MeasureWidget::~MeasureWidget()
{
	QSettings settings;
	settings.beginGroup("MeasureWidget");
	settings.setValue("size", size());
	settings.endGroup();
}

QSize MeasureWidget::sizeHint() const
{
	return preferred_size;
}

void MeasureWidget::objectSelectionChanged()
{
	length_stack->setCurrentIndex(0);
	paper_length_label->clear();
	real_length_label->clear();
	area_stack->setCurrentIndex(0);
	paper_area_label->clear();
	real_area_label->clear();
	warning_label->setText(" <br/> <br/> ");
	
	if (map->getNumSelectedObjects() == 0)
	{
		headline_label->setText("<b>" % tr("No object selected.") % "</b>");
	}
	else if (map->getNumSelectedObjects() > 1)
	{
		headline_label->setText("<b>" % tr("%1 objects selected.").arg(locale().toString(map->getNumSelectedObjects())) % "</b>");
	}
	else
	{
		Object* object = *map->selectedObjectsBegin();
		
		const Symbol* symbol = object->getSymbol();
		headline_label->setText(symbol->getNumberAsString() % " <b>" % symbol->getName() % "</b>");
		
		if (object->getType() != Object::Path)
		{
			warning_label->setText(tr("The selected object is not a path."));
		}
		else
		{
			bool contains_area = symbol->getContainedTypes() & Symbol::Area;
			length_stack->setCurrentIndex(contains_area ? 1 : 2);
			
			PathObject* path = reinterpret_cast<PathObject*>(object);
			path->update();
			
			double mm_to_meters = 0.001 * map->getScaleDenominator();
			
			assert(path->getNumParts() > 0);
			double length_mm = path->getPart(0).getLength();
			double length_meters = length_mm * mm_to_meters;
			
			paper_length_label->setText(locale().toString(length_mm, 'f', 2) % " " % tr("mm", "millimeters"));
			real_length_label->setText(locale().toString(length_meters, 'f', 0) % " " % tr("m", "meters"));
			
			if (contains_area)
			{
				area_stack->setCurrentIndex(1);
				
				double area_mm = 0;
				area_mm += path->getPart(0).calculateArea();
				for (int i = 1; i < path->getNumParts(); ++ i)
					area_mm -= path->getPart(i).calculateArea();
				double area_meters = area_mm * mm_to_meters * mm_to_meters;
				
				paper_area_label->setText(locale().toString(area_mm, 'f', 2) % " " % trUtf8("mm²", "square millimeters"));
				real_area_label->setText(locale().toString(area_meters, 'f', 0) % " " % trUtf8("m²", "square meters"));
				
				if (symbol->getType() == Symbol::Area && static_cast<const AreaSymbol*>(symbol)->getMinimumArea() > 1000*area_mm)
					warning_label->setText("<b>" % tr("This object is too small.") % "</b><br/>" %
					                       tr("The minimimum area is %1 %2.").arg(locale().toString(static_cast<const AreaSymbol*>(symbol)->getMinimumArea()/1000.0, 'f', 2), trUtf8("mm²")));
				else
					warning_label->setText(tr("Note: Boundary length and area are correct only if there are no self-intersections and holes are used as such."));
			}
			else if (symbol->getType() == Symbol::Line && static_cast<const LineSymbol*>(symbol)->getMinimumLength() > 1000*length_mm)
				warning_label->setText("<b>" % tr("This line is too short.") % "</b><br/>" %
				                       tr("The minimum length is %1 %2.").arg(locale().toString(static_cast<const LineSymbol*>(symbol)->getMinimumLength()/1000.0, 'f', 2), tr("mm")));
		}
	}
}
