/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2015 Kai Pastor
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


#include "measure_widget.h"

#include <QLocale>
#include <QScroller>

#include "core/map.h"
#include "core/objects/object.h"
#include "core/symbols/symbol.h"
#include "core/symbols/area_symbol.h"
#include "core/symbols/line_symbol.h"


namespace OpenOrienteering {

MeasureWidget::MeasureWidget(Map* map, QWidget* parent)
: QTextBrowser(parent)
, map(map)
{
	QScroller::grabGesture(viewport(), QScroller::TouchGesture);
	
	connect(map, &Map::objectSelectionChanged, this, &MeasureWidget::objectSelectionChanged);
	connect(map, &Map::selectedObjectEdited, this, &MeasureWidget::objectSelectionChanged);
	connect(map, &Map::symbolChanged, this, &MeasureWidget::objectSelectionChanged);
	
	objectSelectionChanged();
}

MeasureWidget::~MeasureWidget() = default;


void MeasureWidget::objectSelectionChanged()
{
	QString headline;   // inline HTML
	QString body;       // HTML blocks
	QString extra_text; // inline HTML
	
	auto& selected_objects = map->selectedObjects();
	if (selected_objects.empty())
	{
		extra_text = tr("No object selected.");
	}
	else if (selected_objects.size() > 1)
	{
		extra_text = tr("%1 objects selected.").arg(locale().toString(map->getNumSelectedObjects()));
	}
	else
	{
		const Object* object = *begin(selected_objects);
		const Symbol* symbol = object->getSymbol();
		headline = symbol->getNumberAsString() + QLatin1Char(' ') + symbol->getName();
		
		if (object->getType() != Object::Path)
		{
			extra_text = tr("The selected object is not a path.");
		}
		else
		{
			body = QLatin1String{ "<table>" };
			static const QString table_row{ QLatin1String{
			  "<tr><td>%1</td><td align=\"center\">%2 %3</td><td align=\"center\">(%4 %5)</td></tr>" 
			} };
			
			double paper_to_real = 0.001 * map->getScaleDenominator();
			
			object->update();
			const PathPartVector& parts = static_cast<const PathObject*>(object)->parts();
			Q_ASSERT(!parts.empty());
			
			auto paper_length = parts.front().length();
			auto real_length  = paper_length * paper_to_real;
			
			auto paper_length_text = locale().toString(paper_length, 'f', 2);
			auto real_length_text  = locale().toString(real_length, 'f', 0);
			
			if (symbol->getContainedTypes() & Symbol::Area)
			{
				body.append(table_row.arg(tr("Boundary length:"),
				                          paper_length_text, tr("mm", "millimeters"),
				                          real_length_text, tr("m", "meters")));
				
				auto paper_area = parts.front().calculateArea();
				if (parts.size() > 1)
				{
					paper_area *= 2;
					for (const auto& part : parts)
						paper_area -= part.calculateArea();
				}
				double real_area = paper_area * paper_to_real * paper_to_real;
				
				auto paper_area_text = locale().toString(paper_area, 'f', 2);
				auto real_area_text   = locale().toString(real_area, 'f', 0);
				
				body.append(table_row.arg(tr("Area:"),
				                          paper_area_text, tr("mm²", "square millimeters"),
				                          real_area_text , tr("m²", "square meters")));
				
				auto minimum_area = 0.0;
				auto minimum_area_text = QString{ };
				if (symbol->getType() == Symbol::Area)
				{
					minimum_area      = 0.001 * static_cast<const AreaSymbol*>(symbol)->getMinimumArea();
					minimum_area_text = locale().toString(minimum_area, 'f', 2);
				}
				
				if (paper_area < minimum_area && paper_area_text != minimum_area_text)
				{
					extra_text = QLatin1String("<b>") + tr("This object is too small.") + QLatin1String("</b><br/>")
					             + tr("The minimimum area is %1 %2.").arg(minimum_area_text, tr("mm²"))
					             + QLatin1String("<br/>");
				}
				extra_text.append(tr("Note: Boundary length and area are correct only if there are no self-intersections and holes are used as such."));
			}
			else
			{
				body.append(table_row.arg(tr("Length:"),
				                          paper_length_text, tr("mm", "millimeters"),
				                          real_length_text, tr("m", "meters")));
				
				auto minimum_length  = 0.0;
				auto minimum_length_text = QString{ };
				if (symbol->getType() == Symbol::Line)
				{
					minimum_length      = 0.001 * static_cast<const LineSymbol*>(symbol)->getMinimumLength();
					minimum_length_text = locale().toString(minimum_length, 'f', 2);
				}
				
				if (paper_length < minimum_length && paper_length_text != minimum_length_text)
				{
					extra_text = QLatin1String("<b>") + tr("This line is too short.") + QLatin1String("</b><br/>")
					             + tr("The minimum length is %1 %2.").arg(minimum_length_text, tr("mm"));
				}
			}
			
			body.append(QLatin1String("</table>"));
		}
	}
	
	if (!extra_text.isEmpty())
		body.append(QLatin1String("<p>") + extra_text + QLatin1String("</p>"));
	setHtml(QLatin1String("<p><b>") + headline + QLatin1String("</b></p>") + body);
}


}  // namespace OpenOrienteering
