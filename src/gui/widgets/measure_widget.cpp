/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2018, 2024 Kai Pastor
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

#include <limits>

#include <QBuffer>
#include <QIcon>
#include <QIODevice>
#include <QLatin1String>
#include <QLocale>
#include <QPixmap>
#include <QScroller>
#include <QSize>
#include <QStyle>

#include "core/map.h"
#include "core/objects/object.h"
#include "core/symbols/symbol.h"


namespace OpenOrienteering {

MeasureWidget::MeasureWidget(Map* map, QWidget* parent)
: QTextBrowser(parent)
, map(map)
{
	auto const std_icon = style()->standardIcon(QStyle::SP_MessageBoxWarning);
	auto const pixmap = std_icon.pixmap(QSize {22, 22});
	QBuffer buffer;
	buffer.open(QIODevice::WriteOnly);
	pixmap.save(&buffer, "PNG");
	warning_icon = QLatin1String("<img align=\"right\" src=\"data:image/png;base64,")
	               + QString::fromLatin1(buffer.data().toBase64())
	               + QLatin1String("\"/>");
	
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
	bool show_warning = false;
	
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
		const auto* object = *begin(selected_objects);
		const auto* symbol = object->getSymbol();
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
				
				auto minimum_area = 0.001 * symbol->getMinimumArea();
				auto minimum_area_text = locale().toString(minimum_area, 'f', 2);
				
				if (paper_area < minimum_area && paper_area_text != minimum_area_text)
				{
					extra_text = QLatin1String("<b>") + tr("This object is too small.") + QLatin1String("</b><br/>")
					             + tr("The minimimum area is %1 %2.").arg(minimum_area_text, tr("mm²"))
					             + QLatin1String("<br/>");
					show_warning = true;
				}
				extra_text.append(tr("Note: Boundary length and area are correct only if there are no self-intersections and holes are used as such."));
			}
			else
			{
				body.append(table_row.arg(tr("Length:"),
				                          paper_length_text, tr("mm", "millimeters"),
				                          real_length_text, tr("m", "meters")));
				
				auto minimum_length = 0.001 * symbol->getMinimumLength();
				auto minimum_length_text = locale().toString(minimum_length, 'f', 2);
				
				if (paper_length < minimum_length && paper_length_text != minimum_length_text)
				{
					extra_text = QLatin1String("<b>") + tr("This line is too short.") + QLatin1String("</b><br/>")
					             + tr("The minimum length is %1 %2.").arg(minimum_length_text, tr("mm"));
					show_warning = true;
				}
			}
			
			body.append(QLatin1String("</table>"));
		}
	}
	
	if (!extra_text.isEmpty())
		body.append(QLatin1String("<p>") + extra_text + QLatin1String("</p>"));
	setHtml(QLatin1String("<p><b>") + (show_warning ? warning_icon : QLatin1String()) + headline + QLatin1String("</b></p>") + body);
}


}  // namespace OpenOrienteering
