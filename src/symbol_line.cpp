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


#include "symbol_line.h"

#include <QtGui>
#include <QFile>

#include "object.h"
#include "map_color.h"
#include "util.h"
#include "symbol_setting_dialog.h"

LineSymbol::LineSymbol() : Symbol(Symbol::Line)
{
	line_width = 0;
	color = NULL;
	cap_style = FlatCap;
	join_style = MiterJoin;
}
LineSymbol::~LineSymbol()
{
}
Symbol* LineSymbol::duplicate()
{
	LineSymbol* new_line = new LineSymbol();
	new_line->duplicateImplCommon(this);
	new_line->line_width = line_width;
	new_line->color = color;
	new_line->cap_style = cap_style;
	new_line->join_style = join_style;
	return new_line;
}

void LineSymbol::createRenderables(Object* object, const MapCoordVectorF& coords, RenderableVector& output)
{
	if (coords.size() < 2)
		return;
	
	if (cap_style != PointedCap)	// TODO: more conditions to check if line is "simple" (no dashes)
	{
		if (color && line_width > 0)
			output.push_back(new LineRenderable(this, coords, object->getCoordinateVector(), object->isPathClosed()));
	}
}
void LineSymbol::colorDeleted(Map* map, int pos, MapColor* color)
{
    if (color == this->color)
	{
		this->color = NULL;
		getIcon(map, true);
	}
}
bool LineSymbol::containsColor(MapColor* color)
{
	return color == this->color;
}

void LineSymbol::saveImpl(QFile* file, Map* map)
{
	file->write((const char*)&line_width, sizeof(int));
	int temp = map->findColorIndex(color);
	file->write((const char*)&temp, sizeof(int));
}
bool LineSymbol::loadImpl(QFile* file, Map* map)
{
	file->read((char*)&line_width, sizeof(int));
	int temp;
	file->read((char*)&temp, sizeof(int));
	color = (temp >= 0) ? map->getColor(temp) : NULL;
	return true;
}

// ### LineSymbolSettings ###

LineSymbolSettings::LineSymbolSettings(LineSymbol* symbol, Map* map, SymbolSettingDialog* parent) : QGroupBox(tr("Line settings"), parent), symbol(symbol), dialog(parent)
{
	QLabel* width_label = new QLabel(tr("Line width:"));
	width_edit = new QLineEdit(QString::number(symbol->getLineWidth()));
	width_edit->setValidator(new DoubleValidator(0, 999999, width_edit));
	
	QLabel* color_label = new QLabel(tr("Line color:"));
	color_edit = new ColorDropDown(map, symbol->getColor());
	
	QGridLayout* layout = new QGridLayout();
	layout->addWidget(width_label, 0, 0);
	layout->addWidget(width_edit, 0, 1);
	layout->addWidget(color_label, 1, 0);
	layout->addWidget(color_edit, 1, 1);
	
	setLayout(layout);
	
	connect(width_edit, SIGNAL(textEdited(QString)), this, SLOT(widthChanged(QString)));
	connect(color_edit, SIGNAL(currentIndexChanged(int)), this, SLOT(colorChanged()));
}

void LineSymbolSettings::widthChanged(QString text)
{
	symbol->line_width = qRound64(1000 * text.toFloat());
	dialog->updatePreview();
}
void LineSymbolSettings::colorChanged()
{
	symbol->color = color_edit->color();
	dialog->updatePreview();
}

#include "symbol_line.moc"
