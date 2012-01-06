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


#include "symbol_point.h"

#include <QtGui>

#include "map.h"
#include "util.h"
#include "map_color.h"
#include "symbol_setting_dialog.h"

PointSymbol::PointSymbol(Map* map) : Symbol()
{
	rotatable = false;
	inner_radius = 1000;
	inner_color = NULL;
	outer_width = 250;
	outer_color = NULL;
	
	connect(map, SIGNAL(colorDeleted(int,MapColor*)), this, SLOT(colorDeleted(int,MapColor*)));
}

void PointSymbol::createRenderables(Object* object, const MapCoordVectorF& coords, RenderableVector& output)
{
	if (inner_color && inner_radius > 0)
		output.push_back(new DotRenderable(this, coords[0]));
	if (outer_color && outer_width > 0)
		output.push_back(new CircleRenderable(this, coords[0]));
	
	// TODO: other geometry - possibly needs to be moved and rotated
}

void PointSymbol::colorDeleted(int pos, MapColor* color)
{
	if (color == inner_color)
		inner_color = NULL;
	if (color == outer_color)
		outer_color = NULL;
}

void PointSymbol::saveImpl(QFile* file, Map* map)
{
	file->write((const char*)&rotatable, sizeof(bool));
	
	file->write((const char*)&inner_radius, sizeof(int));
	int temp = map->findColorIndex(inner_color);
	file->write((const char*)&temp, sizeof(int));
	
	file->write((const char*)&outer_width, sizeof(int));
	temp = map->findColorIndex(outer_color);
	file->write((const char*)&temp, sizeof(int));
}
void PointSymbol::loadImpl(QFile* file, Map* map)
{
	file->read((char*)&rotatable, sizeof(bool));
	
	file->read((char*)&inner_radius, sizeof(int));
	int temp;
	file->read((char*)&temp, sizeof(int));
	inner_color = map->getColor(temp);
	
	file->read((char*)&outer_width, sizeof(int));
	file->read((char*)&temp, sizeof(int));
	outer_color = map->getColor(temp);
}

// ### PointSymbolSettings ###

PointSymbolSettings::PointSymbolSettings(PointSymbol* symbol, Map* map, SymbolSettingDialog* parent) : QGroupBox(tr("Point settings"), parent), dialog(parent)
{
	this->symbol = symbol;
	
	oriented_to_north_check = new QCheckBox(tr("Always oriented to north (not rotatable)"));
	oriented_to_north_check->setChecked(!symbol->rotatable);
	
	QLabel* inner_radius_label = new QLabel(tr("Inner radius <b>a</b> [mm]:"));
	inner_radius_edit = new QLineEdit(QString::number(0.001 * symbol->inner_radius));
	inner_radius_edit->setValidator(new DoubleValidator(0, 99999, inner_radius_edit));
	
	QLabel* inner_color_label = new QLabel(tr("Inner color:"));
	inner_color_edit = new ColorDropDown(map, symbol->inner_color);
	
	QLabel* outer_width_label = new QLabel(tr("Outer width <b>b</b> [mm]:"));
	outer_width_edit = new QLineEdit(QString::number(0.001 * symbol->outer_width));
	outer_width_edit->setValidator(new DoubleValidator(0, 99999, outer_width_edit));
	
	QLabel* outer_color_label = new QLabel(tr("Outer color:"));
	outer_color_edit = new ColorDropDown(map, symbol->outer_color);
	
	QLabel* explanation_label = new QLabel();
	explanation_label->setPixmap(QPixmap("images/symbol_point_explanation.png"));
	
	QHBoxLayout* inner_radius_layout = new QHBoxLayout();
	inner_radius_layout->addWidget(inner_radius_label);
	inner_radius_layout->addWidget(inner_radius_edit);
	inner_radius_layout->addStretch(1);
	QHBoxLayout* inner_color_layout = new QHBoxLayout();
	inner_color_layout->addWidget(inner_color_label);
	inner_color_layout->addWidget(inner_color_edit);
	inner_color_layout->addStretch(1);
	QHBoxLayout* outer_width_layout = new QHBoxLayout();
	outer_width_layout->addWidget(outer_width_label);
	outer_width_layout->addWidget(outer_width_edit);
	outer_width_layout->addStretch(1);
	QHBoxLayout* outer_color_layout = new QHBoxLayout();
	outer_color_layout->addWidget(outer_color_label);
	outer_color_layout->addWidget(outer_color_edit);
	outer_color_layout->addStretch(1);
	
	QVBoxLayout* edits_layout = new QVBoxLayout();
	edits_layout->addWidget(oriented_to_north_check);
	edits_layout->setAlignment(oriented_to_north_check, Qt::AlignLeft);
	edits_layout->addLayout(inner_radius_layout);
	edits_layout->addLayout(inner_color_layout);
	edits_layout->addLayout(outer_width_layout);
	edits_layout->addLayout(outer_color_layout);
	
	QHBoxLayout* layout = new QHBoxLayout();
	layout->addLayout(edits_layout);
	layout->addWidget(explanation_label);
	setLayout(layout);
	
	connect(oriented_to_north_check, SIGNAL(clicked(bool)), this, SLOT(orientedToNorthClicked(bool)));
	connect(inner_radius_edit, SIGNAL(textEdited(QString)), this, SLOT(innerRadiusChanged(QString)));
	connect(inner_color_edit, SIGNAL(currentIndexChanged(int)), this, SLOT(innerColorChanged()));
	connect(outer_width_edit, SIGNAL(textEdited(QString)), this, SLOT(outerWidthChanged(QString)));
	connect(outer_color_edit, SIGNAL(currentIndexChanged(int)), this, SLOT(outerColorChanged()));
}
void PointSymbolSettings::orientedToNorthClicked(bool checked)
{
	symbol->rotatable = !checked;
}
void PointSymbolSettings::innerRadiusChanged(QString text)
{
	symbol->inner_radius = qRound(1000 * text.toDouble());
	dialog->updatePreview();
}
void PointSymbolSettings::innerColorChanged()
{
	symbol->inner_color = inner_color_edit->color();
	dialog->updatePreview();
}
void PointSymbolSettings::outerWidthChanged(QString text)
{
	symbol->outer_width = qRound(1000 * text.toDouble());
	dialog->updatePreview();
}
void PointSymbolSettings::outerColorChanged()
{
	symbol->outer_color = outer_color_edit->color();
	dialog->updatePreview();
}

#include "symbol_point.moc"
