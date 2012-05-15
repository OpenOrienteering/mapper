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


#include "symbol_area.h"

#include <cassert>
#include <QtGui>
#include <QFile>

#include "util.h"
#include "map_color.h"
#include "symbol_point.h"
#include "symbol_line.h"
#include "object.h"
#include "symbol_setting_dialog.h"
#include "symbol_properties_widget.h"
#include "symbol_point_editor.h"
#include "renderable_implementation.h"

// ### FillPattern ###

AreaSymbol::FillPattern::FillPattern()
{
	type = LinePattern;
	angle = 0;
	rotatable = false;
	line_spacing = 5 * 1000;
	line_offset = 0;
	offset_along_line = 0;
	
	line_color = NULL;
	line_width = 0;
	
	point_distance = 0;
	point = NULL;
}
void AreaSymbol::FillPattern::save(QFile* file, Map* map)
{
	qint32 itype = type;
	file->write((const char*)&itype, sizeof(qint32));
	file->write((const char*)&angle, sizeof(float));
	file->write((const char*)&rotatable, sizeof(bool));
	file->write((const char*)&line_spacing, sizeof(int));
	file->write((const char*)&line_offset, sizeof(int));
	file->write((const char*)&offset_along_line, sizeof(int));
	
	if (type == LinePattern)
	{
		int color_index = map->findColorIndex(line_color);
		file->write((const char*)&color_index, sizeof(int));
		file->write((const char*)&line_width, sizeof(int));
	}
	else
	{
		file->write((const char*)&point_distance, sizeof(int));
		bool have_point = point != NULL;
		file->write((const char*)&have_point, sizeof(bool));
		if (have_point)
			point->save(file, map);
	}
}
bool AreaSymbol::FillPattern::load(QFile* file, int version, Map* map)
{
	qint32 itype;
	file->read((char*)&itype, sizeof(qint32));
	type = (Type)itype;
	file->read((char*)&angle, sizeof(float));
	if (version >= 4)
		file->read((char*)&rotatable, sizeof(bool));
	file->read((char*)&line_spacing, sizeof(int));
	if (version >= 3)
	{
		file->read((char*)&line_offset, sizeof(int));
		file->read((char*)&offset_along_line, sizeof(int));
	}
	
	if (type == LinePattern)
	{
		int color_index;
		file->read((char*)&color_index, sizeof(int));
		line_color = (color_index >= 0) ? map->getColor(color_index) : NULL;
		file->read((char*)&line_width, sizeof(int));
	}
	else
	{
		file->read((char*)&point_distance, sizeof(int));
		bool have_point;
		file->read((char*)&have_point, sizeof(bool));
		if (have_point)
		{
			point = new PointSymbol();
			if (!point->load(file, version, map))
				return false;
		}
		else
			point = NULL;
	}
	return true;
}
void AreaSymbol::FillPattern::createRenderables(QRectF extent, RenderableVector& output)
{
	if (line_spacing <= 0)
		return;
	if (type == PointPattern && point_distance <= 0)
		return;
	
	// Make rotation unique
	double rotation = fmod(1.0 * angle, M_PI);
	
	// Helpers
	LineSymbol line;
	PathObject path(NULL);
	PointObject point_object(point);
	MapCoordVectorF coords;
	if (type == LinePattern)
	{
		line.setColor(line_color);
		line.setLineWidth(0.001*line_width);
	}
	path.addCoordinate(0, MapCoord(0, 0));
	path.addCoordinate(1, MapCoord(0, 0));
	coords.push_back(MapCoordF(0, 0));
	coords.push_back(MapCoordF(0, 0));
	
	// Determine real extent to fill
	QRectF fill_extent;
	if (type == LinePattern)
	{
		if (qAbs(rotation - M_PI/2) < 0.0001)
			fill_extent = QRectF(-0.5 * 0.001*line_width, 0, 0.001*line_width, 0);	// horizontal lines
		else if (qAbs(rotation - 0) < 0.0001)
			fill_extent = QRectF(0, -0.5 * 0.001*line_width, 0, 0.001*line_width);	// verticallines
		else
			fill_extent = QRectF(-0.5 * 0.001*line_width, -0.5 * 0.001*line_width, 0.001*line_width, 0.001*line_width);	// not the 100% optimum but the performance gain is surely neglegible
	}
	else
	{
		// TODO: Ugly method to get the point's extent
		point_object.update(true);
		fill_extent = point_object.getExtent();
	}
	extent = QRectF(extent.topLeft() - fill_extent.bottomRight(), extent.bottomRight() - fill_extent.topLeft());
	
	// Fill
	const float offset = 0.001f * line_offset;
	if (qAbs(rotation - M_PI/2) < 0.0001)
	{
		// Special case: vertical lines
		double first = offset + ceil((extent.left() - offset) / (0.001*line_spacing)) * 0.001*line_spacing;
		for (double cur = first; cur < extent.right(); cur += 0.001*line_spacing)
		{
			coords[0] = MapCoordF(cur, extent.top());
			coords[1] = MapCoordF(cur, extent.bottom());
			createLine(coords, &line, &path, &point_object, output);
		}
	}
	else if (qAbs(rotation - 0) < 0.0001)
	{
		// Special case: horizontal lines
		double first = offset + ceil((extent.top() - offset) / (0.001*line_spacing)) * 0.001*line_spacing;
		for (double cur = first; cur < extent.bottom(); cur += 0.001*line_spacing)
		{
			coords[0] = MapCoordF(extent.left(), cur);
			coords[1] = MapCoordF(extent.right(), cur);
			createLine(coords, &line, &path, &point_object, output);
		}
	}
	else
	{
		// General case
		float xfactor = 1.0f / sin(rotation);
		float yfactor = 1.0f / cos(rotation);
		
		const float offset = 0.001f * line_offset;
		float dist_x = xfactor * 0.001*line_spacing;
		float dist_y = yfactor * 0.001*line_spacing;
		float offset_x = xfactor * offset;
		float offset_y = yfactor * offset;
		
		if (rotation < M_PI/2)
		{
			// Start with the upper left corner
			offset_x += (-extent.top()) / tan(rotation);
			offset_y -= extent.left() * tan(rotation);
			float start_x = offset_x + ceil((extent.x() - offset_x) / dist_x) * dist_x;
			float start_y = extent.top();
			float end_x = extent.left();
			float end_y = offset_y + ceil((extent.y() - offset_y) / dist_y) * dist_y;
			
			do
			{
				// Correct coordinates
				if (start_x > extent.right())
				{
					start_y += ((start_x - extent.right()) / dist_x) * dist_y;
					start_x = extent.right();
				}
				if (end_y > extent.bottom())
				{
					end_x += ((end_y - extent.bottom()) / dist_y) * dist_x;
					end_y = extent.bottom();
				}
				
				if (start_y > extent.bottom())
					break;
				
				// Create the renderable(s)
				coords[0] = MapCoordF(start_x, start_y);
				coords[1] = MapCoordF(end_x, end_y);
				createLine(coords, &line, &path, &point_object, output);
				
				// Move to next position
				start_x += dist_x;
				end_y += dist_y;
			} while (true);
		}
		else
		{
			// Start with left lower corner
			offset_x += (-extent.bottom()) / tan(rotation);
			offset_y -= extent.x() * tan(rotation);
			float start_x = offset_x + ceil((extent.x() - offset_x) / dist_x) * dist_x;
			float start_y = extent.bottom();
			float end_x = extent.x();
			float end_y = offset_y + ceil((extent.bottom() - offset_y) / dist_y) * dist_y;
			
			do
			{
				// Correct coordinates
				if (start_x > extent.right())
				{
					start_y += ((start_x - extent.right()) / dist_x) * dist_y;
					start_x = extent.right();
				}
				if (end_y < extent.y())
				{
					end_x += ((end_y - extent.y()) / dist_y) * dist_x;
					end_y = extent.y();
				}
				
				if (start_y < extent.y())
					break;
				
				// Create the renderable(s)
				coords[0] = MapCoordF(start_x, start_y);
				coords[1] = MapCoordF(end_x, end_y);
				createLine(coords, &line, &path, &point_object, output);
				
				// Move to next position
				start_x += dist_x;
				end_y += dist_y;
			} while (true);
		}
	}
}
void AreaSymbol::FillPattern::createLine(MapCoordVectorF& coords, LineSymbol* line, PathObject* path, PointObject* point_object, RenderableVector& output)
{
	if (type == LinePattern)
		line->createRenderables(path, path->getRawCoordinateVector(), coords, output);
	else
	{
		MapCoordF to_end = MapCoordF(coords[1].getX() - coords[0].getX(), coords[1].getY() - coords[0].getY());
		float length = to_end.length();
		float dir_x = to_end.getX() / length;
		float dir_y = to_end.getY() / length;
		
		float offset = 0.001f * offset_along_line;
		float base_dist = dir_x * coords[0].getX() + dir_y * coords[0].getY();	// distance of coords[0] from the zero line
		float first = (offset + ceil((base_dist - offset) / (0.001*point_distance)) * 0.001*point_distance) - base_dist;
		
		coords[0].setX(coords[0].getX() + dir_x * first);
		coords[0].setY(coords[0].getY() + dir_y * first);
		float step_x = dir_x * 0.001*point_distance;
		float step_y = dir_y * 0.001*point_distance;
		
		for (float cur = first; cur < length; cur += 0.001*point_distance)
		{
			// TODO: hack-ish misuse of the point symbol here (with a different point object and with coords.size() == 2 instead of 1)
			point->createRenderables(point_object, point_object->getRawCoordinateVector(), coords, output);
			coords[0].setX(coords[0].getX() + step_x);
			coords[0].setY(coords[0].getY() + step_y);
		}
	}
}

void AreaSymbol::FillPattern::scale(double factor)
{
	line_spacing = qRound(factor * line_spacing);
	line_width = qRound(factor * line_width);
	line_offset = qRound(factor * line_offset);
	offset_along_line = qRound(factor * offset_along_line);
	point_distance = qRound(factor * point_distance);
	
	if (point)
		point->scale(factor);
}

// ### AreaSymbol ###

AreaSymbol::AreaSymbol() : Symbol(Symbol::Area)
{
	color = NULL;
	minimum_area = 0;
}

AreaSymbol::~AreaSymbol()
{
	for (int i = 0; i < (int)patterns.size(); ++i)
	{
		if (patterns[i].type == FillPattern::PointPattern)
			delete patterns[i].point;
	}
}

Symbol* AreaSymbol::duplicate() const
{
	AreaSymbol* new_area = new AreaSymbol();
	new_area->duplicateImplCommon(this);
	new_area->color = color;
	new_area->minimum_area = minimum_area;
	new_area->patterns = patterns;
	for (int i = 0; i < (int)new_area->patterns.size(); ++i)
	{
		if (new_area->patterns[i].type == FillPattern::PointPattern)
			new_area->patterns[i].point = reinterpret_cast<PointSymbol*>(new_area->patterns[i].point->duplicate());
	}
	return new_area;
}

void AreaSymbol::createRenderables(Object* object, const MapCoordVector& flags, const MapCoordVectorF& coords, RenderableVector& output)
{
	if (coords.size() >= 3)
	{
		// The shape output is even created if the area is not filled with a color
		// because the QPainterPath created by it is needed as clip path for the fill objects
		PathObject* path = reinterpret_cast<PathObject*>(object);
		AreaRenderable* clip_path = new AreaRenderable(this, coords, flags, &path->getPathCoordinateVector());
		output.push_back(clip_path);
		
		int size = (int)patterns.size();
		for (int i = 0; i < size; ++i)
		{
			size_t o = output.size();
			patterns[i].createRenderables(clip_path->getExtent(), output);	
			for (; o < output.size(); ++o)
				output[o]->setClipPath(clip_path->getPainterPath());
		}
	}
}
void AreaSymbol::colorDeleted(Map* map, int pos, MapColor* color)
{
	bool change = false;
    if (color == this->color)
	{
		this->color = NULL;
		change = true;
	}
	
	for (int i = 0; i < (int)patterns.size(); ++i)
	{
		if (patterns[i].type == FillPattern::PointPattern)
			patterns[i].point->colorDeleted(map, pos, color);
		else if (patterns[i].line_color == color)
		{
			patterns[i].line_color = NULL;
			change = true;
		}
	}
	
	if (change)
		getIcon(map, true);
}
bool AreaSymbol::containsColor(MapColor* color)
{
	if (color == this->color)
		return true;
	
	for (int i = 0; i < (int)patterns.size(); ++i)
	{
		if (patterns[i].type == FillPattern::PointPattern && patterns[i].point->containsColor(color))
			return true;
		else if (patterns[i].type == FillPattern::LinePattern && patterns[i].line_color == color)
			return true;
	}
	
	return false;
}

void AreaSymbol::scale(double factor)
{
	minimum_area = qRound(factor*factor * minimum_area);
	
	int size = (int)patterns.size();
	for (int i = 0; i < size; ++i)
		patterns[i].scale(factor);
}

void AreaSymbol::saveImpl(QFile* file, Map* map)
{
	int temp = map->findColorIndex(color);
	file->write((const char*)&temp, sizeof(int));
	file->write((const char*)&minimum_area, sizeof(int));
	
	int size = (int)patterns.size();
	file->write((const char*)&size, sizeof(int));
	for (int i = 0; i < size; ++i)
		patterns[i].save(file, map);
}
bool AreaSymbol::loadImpl(QFile* file, int version, Map* map)
{
	int temp;
	file->read((char*)&temp, sizeof(int));
	color = (temp >= 0) ? map->getColor(temp) : NULL;
	if (version >= 2)
		file->read((char*)&minimum_area, sizeof(int));
	
	int size;
	file->read((char*)&size, sizeof(int));
	patterns.resize(size);
	for (int i = 0; i < size; ++i)
		if (!patterns[i].load(file, version, map))
			return false;
	
	return true;
}

SymbolPropertiesWidget* AreaSymbol::createPropertiesWidget(SymbolSettingDialog* dialog)
{
	return new AreaSymbolSettings(this, dialog);
}


// ### AreaSymbolSettings ###

AreaSymbolSettings::AreaSymbolSettings(AreaSymbol* symbol, SymbolSettingDialog* dialog)
 : SymbolPropertiesWidget(symbol, dialog), symbol(symbol)
{
	map = dialog->getPreviewMap();
	controller = dialog->getPreviewController();
	
	updatePatternNames(false);
	
	react_to_changes = true;
	
	QLabel* color_label = new QLabel(tr("Area color:"));
	color_edit = new ColorDropDown(map, symbol->getColor());
	
	QLabel* minimum_area_label = new QLabel(tr("Minimum area:"));
	minimum_area_edit = new QLineEdit(QString::number(0.001f * symbol->minimum_area));
	minimum_area_edit->setValidator(new DoubleValidator(0, 999999, minimum_area_edit));
	
	QLabel* fill_pattern_label = new QLabel("<b>" + tr("Fill patterns") + "</b>");
	add_fill_button = new QPushButton(QIcon(":/images/plus.png"), "");
	delete_fill_button = new QPushButton(QIcon(":/images/minus.png"), "");
	
	fill_pattern_widget = new QWidget();
	QLabel* fill_number_label = new QLabel(tr("Pattern:"));
	fill_number_combo = new QComboBox();
	for (int i = 0; i < (int)symbol->patterns.size(); ++i)
		fill_number_combo->addItem(symbol->patterns[i].name, QVariant(i));
	if (fill_number_combo->count() > 0)
		fill_number_combo->setCurrentIndex(0);
	QLabel* fill_type_label = new QLabel(tr("Type:"));
	fill_type_combo = new QComboBox();
	fill_type_combo->addItem(tr("Line pattern"), QVariant(AreaSymbol::FillPattern::LinePattern));
	fill_type_combo->addItem(tr("Point pattern"), QVariant(AreaSymbol::FillPattern::PointPattern));
	QLabel* fill_angle_label = new QLabel(tr("Angle [degrees]:"));
	fill_angle_edit = new QLineEdit();	// TODO: Use special angle edit widget
	fill_angle_edit->setValidator(new DoubleValidator(0, 360, fill_angle_edit));
	fill_rotatable_check = new QCheckBox(tr("Filling is rotatable by mapper"));
	QLabel* fill_spacing_label = new QLabel(tr("Distance between center lines:"));
	fill_spacing_edit = new QLineEdit();
	fill_spacing_edit->setValidator(new DoubleValidator(0, 999999, fill_spacing_edit));
	
	QLabel* fill_line_offset_label = new QLabel(tr("Line offset:"));
	fill_line_offset_edit = new QLineEdit();
	fill_line_offset_edit->setValidator(new DoubleValidator(-999999, 999999, fill_line_offset_edit));
	
	fill_offset_along_line_label = new QLabel(tr("Offset along line:"));
	fill_offset_along_line_edit = new QLineEdit();
	fill_offset_along_line_edit->setValidator(new DoubleValidator(-999999, 999999, fill_offset_along_line_edit));
	
	fill_color_label = new QLabel(tr("Line color:"));
	fill_color_edit = new ColorDropDown(map);
	fill_linewidth_label = new QLabel(tr("Line width:"));
	fill_linewidth_edit = new QLineEdit();
	fill_linewidth_edit->setValidator(new DoubleValidator(0, 999999, fill_linewidth_edit));
	
	fill_pointdist_label = new QLabel(tr("Point distance:"));
	fill_pointdist_edit = new QLineEdit();
	fill_pointdist_edit->setValidator(new DoubleValidator(0, 999999, fill_pointdist_edit));
	
	
	QGridLayout* top_layout = new QGridLayout();
	top_layout->addWidget(color_label, 0, 0);
	top_layout->addWidget(color_edit, 0, 1);
	top_layout->addWidget(minimum_area_label, 1, 0);
	top_layout->addWidget(minimum_area_edit, 1, 1);
	top_layout->addWidget(fill_pattern_label, 2, 0, 1, 2);
	
	QHBoxLayout* buttons_layout = new QHBoxLayout();
	buttons_layout->addStretch(1);
	buttons_layout->addWidget(add_fill_button);
	buttons_layout->addWidget(delete_fill_button);
	buttons_layout->addStretch(1);
	
	QGridLayout* fill_pattern_layout = new QGridLayout();
	fill_pattern_layout->setMargin(0);
	fill_pattern_layout->addWidget(fill_number_label, 0, 0);
	fill_pattern_layout->addWidget(fill_number_combo, 0, 1);
	fill_pattern_layout->addWidget(fill_type_label, 1, 0);
	fill_pattern_layout->addWidget(fill_type_combo, 1, 1);
	fill_pattern_layout->addWidget(fill_angle_label, 2, 0);
	fill_pattern_layout->addWidget(fill_angle_edit, 2, 1);
	fill_pattern_layout->addWidget(fill_rotatable_check, 3, 0, 1, 2);
	fill_pattern_layout->addWidget(fill_spacing_label, 4, 0);
	fill_pattern_layout->addWidget(fill_spacing_edit, 4, 1);
	fill_pattern_layout->addWidget(fill_line_offset_label, 5, 0);
	fill_pattern_layout->addWidget(fill_line_offset_edit, 5, 1);
	fill_pattern_layout->addWidget(fill_offset_along_line_label, 6, 0);
	fill_pattern_layout->addWidget(fill_offset_along_line_edit, 6, 1);
	
	fill_pattern_layout->addWidget(fill_color_label, 7, 0);
	fill_pattern_layout->addWidget(fill_color_edit, 7, 1);
	fill_pattern_layout->addWidget(fill_linewidth_label, 8, 0);
	fill_pattern_layout->addWidget(fill_linewidth_edit, 8, 1);
	
	fill_pattern_layout->addWidget(fill_pointdist_label, 9, 0);
	fill_pattern_layout->addWidget(fill_pointdist_edit, 9, 1);
	fill_pattern_widget->setLayout(fill_pattern_layout);
	
	QVBoxLayout* layout = new QVBoxLayout();
	layout->addLayout(top_layout);
	layout->addLayout(buttons_layout);
	layout->addWidget(fill_pattern_widget);
	
	QWidget* area_tab = new QWidget();
	area_tab->setLayout(layout);
	addPropertiesGroup(tr("Area settings"), area_tab);
	
	updateFillWidgets(false);
	
	for (int i = 0; i < symbol->getNumFillPatterns(); ++i)
	{
		if (symbol->getFillPattern(i).type == AreaSymbol::FillPattern::PointPattern)
		{
			PointSymbolEditorWidget* editor = new PointSymbolEditorWidget(controller, symbol->getFillPattern(i).point, 16);
			connect(editor, SIGNAL(symbolEdited()), dialog, SLOT(updatePreview()) );
			addPropertiesGroup(symbol->getFillPattern(i).name, editor); 	
		}
	}
	
	connect(color_edit, SIGNAL(currentIndexChanged(int)), this, SLOT(colorChanged()));
	connect(minimum_area_edit, SIGNAL(textEdited(QString)), this, SLOT(minimumDimensionsChanged(QString)));
	connect(add_fill_button, SIGNAL(clicked(bool)), this, SLOT(addFillClicked()));
	connect(delete_fill_button, SIGNAL(clicked(bool)), this, SLOT(deleteFillClicked()));
	connect(fill_number_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(fillNumberChanged(int)));
	connect(fill_type_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(fillTypeChanged(int)));
	connect(fill_angle_edit, SIGNAL(textEdited(QString)), this, SLOT(fillAngleChanged(QString)));
	connect(fill_rotatable_check, SIGNAL(clicked(bool)), this, SLOT(fillRotatableClicked(bool)));
	connect(fill_spacing_edit, SIGNAL(textEdited(QString)), this, SLOT(fillSpacingChanged(QString)));
	connect(fill_line_offset_edit, SIGNAL(textEdited(QString)), this, SLOT(fillLineOffsetChanged(QString)));
	connect(fill_offset_along_line_edit, SIGNAL(textEdited(QString)), this, SLOT(fillOffsetAlongLineChanged(QString)));
	
	connect(fill_color_edit, SIGNAL(currentIndexChanged(int)), this, SLOT(fillColorChanged()));
	connect(fill_linewidth_edit, SIGNAL(textEdited(QString)), this, SLOT(fillLinewidthChanged(QString)));
	connect(fill_pointdist_edit, SIGNAL(textEdited(QString)), this, SLOT(fillPointdistChanged(QString)));
}

void AreaSymbolSettings::updatePatternNames(bool update_ui)
{
	for (int i = 0; i < (int)symbol->patterns.size(); ++i)
	{
		if (symbol->patterns[i].type == AreaSymbol::FillPattern::PointPattern)
		{
			QString name = tr("Point pattern %1").arg(i+1);
			if (update_ui)
			{
				renamePropertiesGroup(symbol->patterns[i].name, name);
				fill_number_combo->setItemText(i, name);
			}
			symbol->patterns[i].name = name;
			symbol->patterns[i].point->setName(name);
		}
		else
		{
			QString name = tr("Line pattern %1").arg(i+1);
			symbol->patterns[i].name = name;
			if (update_ui)
			{
				fill_number_combo->setItemText(i, name);
			}
		}
	}
}

void AreaSymbolSettings::updateFillWidgets(bool show)
{
	int index = fill_number_combo->currentIndex();
	
	if (index < 0)
	{
		fill_pattern_widget->hide();
		delete_fill_button->setEnabled(false);
		return;
	}
	
	if (show)
		fill_pattern_widget->show();
	delete_fill_button->setEnabled(true);
	
	AreaSymbol::FillPattern* fill = &symbol->patterns[index];
	
	react_to_changes = false;
	fill_type_combo->setCurrentIndex(fill_type_combo->findData(fill->type));
	fill_angle_edit->setText(QString::number(fill->angle * 360 / (2*M_PI)));
	fill_rotatable_check->setChecked(fill->rotatable);
	fill_spacing_edit->setText(QString::number(0.001 * fill->line_spacing));
	fill_line_offset_edit->setText(QString::number(0.001 * fill->line_offset));
	
	if (fill->type == AreaSymbol::FillPattern::LinePattern)
	{
		if (show)
		{
			fill_color_label->show();
			fill_color_edit->show();
			fill_linewidth_label->show();
			fill_linewidth_edit->show();
		}
		fill_offset_along_line_label->hide();
		fill_offset_along_line_edit->hide();
		fill_pointdist_label->hide();
		fill_pointdist_edit->hide();
		
		fill_color_edit->setColor(fill->line_color);
		fill_linewidth_edit->setText(QString::number(0.001 * fill->line_width));
	}
	else
	{
		if (show)
		{
			fill_offset_along_line_label->show();
			fill_offset_along_line_edit->show();
			fill_pointdist_label->show();
			fill_pointdist_edit->show();
		}
		fill_color_label->hide();
		fill_color_edit->hide();
		fill_linewidth_label->hide();
		fill_linewidth_edit->hide();

		fill_offset_along_line_edit->setText(QString::number(0.001 * fill->offset_along_line));
		fill_pointdist_edit->setText(QString::number(0.001 * fill->point_distance));
	}
	react_to_changes = true;
}

void AreaSymbolSettings::colorChanged()
{
	symbol->color = color_edit->color();
	dialog->updatePreview();
}

void AreaSymbolSettings::minimumDimensionsChanged(QString text)
{
	symbol->minimum_area = qRound(1000 * minimum_area_edit->text().toFloat());
}

void AreaSymbolSettings::addFillClicked()
{
	int index = fill_number_combo->currentIndex() + 1;
	if (index < 0)
		index = symbol->patterns.size();
	symbol->patterns.insert(symbol->patterns.begin() + index, AreaSymbol::FillPattern());
	updatePatternNames();
	
	fill_number_combo->insertItem(index, symbol->patterns[index].name, QVariant(index));
	assert(int(symbol->patterns.size()) == fill_number_combo->count());
	for (int i = index + 1; i < fill_number_combo->count(); ++i)
		fill_number_combo->setItemText(i, symbol->patterns[i].name);

	dialog->updatePreview();
	
	fill_number_combo->setCurrentIndex(index);
}

void AreaSymbolSettings::deleteFillClicked()
{
	int index = fill_number_combo->currentIndex();
	assert(index >= 0);
	
	AreaSymbol::FillPattern& fill = symbol->patterns[index];
	if (fill.type == AreaSymbol::FillPattern::PointPattern)
	{
		removePropertiesGroup(fill.name);
		delete fill.point;
		fill.point = NULL;
	}
	symbol->patterns.erase(symbol->patterns.begin() + index);
	
	fill_number_combo->removeItem(index);
	updatePatternNames();
	updateFillWidgets(true);
	dialog->updatePreview();
}

void AreaSymbolSettings::fillNumberChanged(int index)
{
	if (!react_to_changes) return;
	updateFillWidgets(true);
}

void AreaSymbolSettings::fillTypeChanged(int index)
{
	if (!react_to_changes) return;
	
	AreaSymbol::FillPattern* fill = &symbol->patterns[fill_number_combo->currentIndex()];
	fill->type = static_cast<AreaSymbol::FillPattern::Type>(fill_type_combo->itemData(index).toInt());
	if (fill->type == AreaSymbol::FillPattern::LinePattern)
	{
		// Changing to line, clear point settings
		removePropertiesGroup(fill->name);
		delete fill->point;
		fill->point = NULL;
		fill->point_distance = 0;
		updatePatternNames();
	}
	else
	{
		// Changing to point, clear line settings and create point
		fill->line_color = NULL;
		fill->line_width = 0;
		fill->point = new PointSymbol();
		updatePatternNames();
		fill_number_combo->setItemText(fill_number_combo->currentIndex(), fill->name); // FIXME: renumber only following items
		PointSymbolEditorWidget* editor = new PointSymbolEditorWidget(controller, fill->point, 16);
		connect(editor, SIGNAL(symbolEdited()), dialog, SLOT(updatePreview()) );
		if (fill_number_combo->currentIndex() == int(symbol->patterns.size()) - 1)
		{
			addPropertiesGroup(fill->name, editor); 	
		}
		else
		{
			int group_index = indexOfPropertiesGroup(symbol->patterns[fill_number_combo->currentIndex()+1].name);
			insertPropertiesGroup(group_index, fill->name, editor);
		}
	}
	updateFillWidgets(true);
	dialog->updatePreview();
}

void AreaSymbolSettings::fillAngleChanged(QString text)
{
	if (!react_to_changes) return;
	AreaSymbol::FillPattern* fill = &symbol->patterns[fill_number_combo->currentIndex()];
	fill->angle = text.toFloat() * (2*M_PI) / 360.0;
	dialog->updatePreview();
}

void AreaSymbolSettings::fillRotatableClicked(bool checked)
{
	if (!react_to_changes) return;
	AreaSymbol::FillPattern* fill = &symbol->patterns[fill_number_combo->currentIndex()];
	fill->rotatable = checked;
}

void AreaSymbolSettings::fillSpacingChanged(QString text)
{
	if (!react_to_changes) return;
	AreaSymbol::FillPattern* fill = &symbol->patterns[fill_number_combo->currentIndex()];
	fill->line_spacing = qRound(1000 * text.toFloat());
	dialog->updatePreview();
}

void AreaSymbolSettings::fillLineOffsetChanged(QString text)
{
	if (!react_to_changes) return;
	AreaSymbol::FillPattern* fill = &symbol->patterns[fill_number_combo->currentIndex()];
	fill->line_offset = qRound(1000 * text.toFloat());
	dialog->updatePreview();
}

void AreaSymbolSettings::fillOffsetAlongLineChanged(QString text)
{
	if (!react_to_changes) return;
	AreaSymbol::FillPattern* fill = &symbol->patterns[fill_number_combo->currentIndex()];
	assert(fill->type == AreaSymbol::FillPattern::PointPattern);
	fill->offset_along_line = qRound(1000 * text.toFloat());
	dialog->updatePreview();
}

void AreaSymbolSettings::fillColorChanged()
{
	if (!react_to_changes) return;
	AreaSymbol::FillPattern* fill = &symbol->patterns[fill_number_combo->currentIndex()];
	assert(fill->type == AreaSymbol::FillPattern::LinePattern);
	fill->line_color = fill_color_edit->color();
	dialog->updatePreview();
}

void AreaSymbolSettings::fillLinewidthChanged(QString text)
{
	if (!react_to_changes) return;
	AreaSymbol::FillPattern* fill = &symbol->patterns[fill_number_combo->currentIndex()];
	assert(fill->type == AreaSymbol::FillPattern::LinePattern);
	fill->line_width = qRound(1000 * text.toFloat());
	dialog->updatePreview();
}

void AreaSymbolSettings::fillPointdistChanged(QString text)
{
	if (!react_to_changes) return;
	AreaSymbol::FillPattern* fill = &symbol->patterns[fill_number_combo->currentIndex()];
	assert(fill->type == AreaSymbol::FillPattern::PointPattern);
	fill->point_distance = qRound(1000 * text.toFloat());
	dialog->updatePreview();
}

#include "symbol_area.moc"
