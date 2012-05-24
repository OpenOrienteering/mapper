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

#define HEADLINE(text) QLabel(QString("<b>") % text % "</b>")

AreaSymbolSettings::AreaSymbolSettings(AreaSymbol* symbol, SymbolSettingDialog* dialog)
 : SymbolPropertiesWidget(symbol, dialog), symbol(symbol)
{
	map = dialog->getPreviewMap();
	controller = dialog->getPreviewController();
	
	color_edit = new ColorDropDown(map);
	minimum_size_edit = new QDoubleSpinBox();
	minimum_size_edit->setDecimals(1);
	minimum_size_edit->setRange(0.0, 999999.9);
	minimum_size_edit->setSuffix(QString::fromUtf8(" mm²"));
	
	pattern_list = new QListWidget();
	
	del_pattern_button = new QPushButton(QIcon(":/images/minus.png"), "");
	add_pattern_button = new QToolButton();
	add_pattern_button->setIcon(QIcon(":/images/plus.png"));
	add_pattern_button->setToolButtonStyle(Qt::ToolButtonIconOnly);
	add_pattern_button->setPopupMode(QToolButton::InstantPopup);
	add_pattern_button->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
	add_pattern_button->setMinimumWidth(del_pattern_button->sizeHint().width());
	QMenu* add_fill_button_menu = new QMenu(add_pattern_button);
	add_fill_button_menu->addAction(tr("Line pattern"), this, SLOT(addLinePattern()));
	add_fill_button_menu->addAction(tr("Point pattern"), this, SLOT(addPointPattern()));
	add_pattern_button->setMenu(add_fill_button_menu);
	
	QLabel* fill_angle_label = new QLabel(tr("Angle:"));
	pattern_angle_edit = new QDoubleSpinBox();
	pattern_angle_edit->setDecimals(1);
	pattern_angle_edit->setRange(0.0, 360.0);
	pattern_angle_edit->setSuffix(QString::fromUtf8(" °"));
	pattern_rotatable_check = new QCheckBox(tr("Filling is rotatable by mapper"));
	QLabel* fill_spacing_label = new QLabel(tr("Distance between center lines:"));
	pattern_spacing_edit = new QDoubleSpinBox();
	pattern_spacing_edit->setDecimals(2);
	pattern_spacing_edit->setRange(0.0, 999999.0);
	pattern_spacing_edit->setSuffix(" mm");
	
	QLabel* fill_line_offset_label = new QLabel(tr("Line offset:"));
	pattern_line_offset_edit = new QDoubleSpinBox();
	pattern_line_offset_edit->setDecimals(2);
	pattern_line_offset_edit->setRange(0.0, 999999.0);
	pattern_line_offset_edit->setSuffix(" mm");
	
	pattern_offset_along_line_label = new QLabel(tr("Offset along line:"));
	pattern_offset_along_line_edit = new QDoubleSpinBox();
	pattern_offset_along_line_edit->setDecimals(2);
	pattern_offset_along_line_edit->setRange(0.0, 999999.0);
	pattern_offset_along_line_edit->setSuffix(" mm");
	
	pattern_color_label = new QLabel(tr("Line color:"));
	pattern_color_edit = new ColorDropDown(map);
	pattern_linewidth_label = new QLabel(tr("Line width:"));
	pattern_linewidth_edit = new QDoubleSpinBox();
	pattern_linewidth_edit->setDecimals(2);
	pattern_linewidth_edit->setRange(0.0, 999999.0);
	pattern_linewidth_edit->setSuffix(" mm");
	
	pattern_pointdist_label = new QLabel(tr("Point distance:"));
	pattern_pointdist_edit = new QDoubleSpinBox();
	pattern_pointdist_edit->setDecimals(2);
	pattern_pointdist_edit->setRange(0.0, 999999.0);
	pattern_pointdist_edit->setSuffix(" mm");
	
	
	QHBoxLayout* buttons_layout = new QHBoxLayout();
	buttons_layout->addStretch(1);
	buttons_layout->addWidget(add_pattern_button);
	buttons_layout->addWidget(del_pattern_button);
	buttons_layout->addStretch(1);
	
	QFormLayout* fill_pattern_layout = new QFormLayout();
//	fill_pattern_layout->setMargin(0);
	fill_pattern_layout->addRow(fill_angle_label, pattern_angle_edit);
	fill_pattern_layout->addRow(pattern_rotatable_check);
	fill_pattern_layout->addRow(fill_spacing_label, pattern_spacing_edit);
	fill_pattern_layout->addRow(fill_line_offset_label, pattern_line_offset_edit);
	fill_pattern_layout->addRow(pattern_offset_along_line_label, pattern_offset_along_line_edit);
	fill_pattern_layout->addRow(pattern_pointdist_label, pattern_pointdist_edit);
	fill_pattern_layout->addRow(pattern_linewidth_label, pattern_linewidth_edit);
	fill_pattern_layout->addRow(pattern_color_label, pattern_color_edit);
	
	QVBoxLayout* fill_patterns_list_layout = new QVBoxLayout();
	fill_patterns_list_layout->addWidget(pattern_list, 1);
	fill_patterns_list_layout->addLayout(buttons_layout, 0);
	
	QWidget* fill_patterns_widget = new QWidget();
	QHBoxLayout* fill_patterns_layout = new QHBoxLayout();
	fill_patterns_layout->addLayout(fill_patterns_list_layout, 1);
	fill_patterns_layout->addSpacing(16);
	fill_patterns_layout->addLayout(fill_pattern_layout, 3);
	fill_patterns_widget->setLayout(fill_patterns_layout);
	fill_patterns_layout->setContentsMargins(0,0,0,0);
	
	QFormLayout* layout = new QFormLayout();
	layout->addRow(tr("Area color:"), color_edit);
	layout->addRow(tr("Minimum size:"), minimum_size_edit);
	layout->addRow(new QLabel());
	layout->addRow(new HEADLINE(tr("Fill patterns")));
	layout->addRow(fill_patterns_widget);
	
	QWidget* area_tab = new QWidget();
	area_tab->setLayout(layout);
	addPropertiesGroup(tr("Area settings"), area_tab);
	
	connect(color_edit, SIGNAL(currentIndexChanged(int)), this, SLOT(colorChanged()));
	connect(minimum_size_edit, SIGNAL(valueChanged(double)), this, SLOT(minimumSizeChanged(double)));
	connect(del_pattern_button, SIGNAL(clicked(bool)), this, SLOT(deleteActivePattern()));
	connect(pattern_list, SIGNAL(currentRowChanged(int)), this, SLOT(selectPattern(int)));
	connect(pattern_angle_edit, SIGNAL(valueChanged(double)), this, SLOT(patternAngleChanged(double)));
	connect(pattern_rotatable_check, SIGNAL(clicked(bool)), this, SLOT(patternRotatableClicked(bool)));
	connect(pattern_spacing_edit, SIGNAL(valueChanged(double)), this, SLOT(patternSpacingChanged(double)));
	connect(pattern_line_offset_edit, SIGNAL(valueChanged(double)), this, SLOT(patternLineOffsetChanged(double)));
	connect(pattern_offset_along_line_edit, SIGNAL(valueChanged(double)), this, SLOT(patternOffsetAlongLineChanged(double)));
	
	connect(pattern_color_edit, SIGNAL(currentIndexChanged(int)), this, SLOT(patternColorChanged()));
	connect(pattern_linewidth_edit, SIGNAL(valueChanged(double)), this, SLOT(patternLineWidthChanged(double)));
	connect(pattern_pointdist_edit, SIGNAL(valueChanged(double)), this, SLOT(patternPointDistChanged(double)));
	
	updateAreaGeneral();
	loadPatterns();
}

void AreaSymbolSettings::reset(Symbol* symbol)
{
	assert(symbol->getType() == Symbol::Area);
	
	clearPatterns();
	
	SymbolPropertiesWidget::reset(symbol);
	this->symbol = reinterpret_cast<AreaSymbol*>(symbol);
	
	updateAreaGeneral();
	loadPatterns();
}

void AreaSymbolSettings::updateAreaGeneral()
{
	color_edit->blockSignals(true);
	color_edit->setColor(symbol->getColor());
	color_edit->blockSignals(false);
	minimum_size_edit->blockSignals(true);
	minimum_size_edit->setValue(0.001 * symbol->minimum_area);
	minimum_size_edit->blockSignals(false);
}

void AreaSymbolSettings::clearPatterns()
{
	pattern_list->clear();
	for (int i = 0; i < (int)this->symbol->patterns.size(); ++i)
	{
		if (this->symbol->patterns[i].type == AreaSymbol::FillPattern::PointPattern)
			removePropertiesGroup(2);
	}
}

void AreaSymbolSettings::loadPatterns()
{
	assert(pattern_list->count() == 0);
	assert(count() == 2); // General + Area tab
	
	for (int i = 0; i < symbol->getNumFillPatterns(); ++i)
	{
		pattern_list->addItem(symbol->patterns[i].name);
		if (symbol->getFillPattern(i).type == AreaSymbol::FillPattern::PointPattern)
		{
			PointSymbolEditorWidget* editor = new PointSymbolEditorWidget(controller, symbol->getFillPattern(i).point, 16);
			connect(editor, SIGNAL(symbolEdited()), this, SIGNAL(propertiesModified()) );
			addPropertiesGroup(symbol->patterns[i].name, editor);
		}
	}
	updatePatternNames();
	pattern_list->setCurrentRow(0);
}

void AreaSymbolSettings::updatePatternNames()
{
	int line_pattern_num = 0, point_pattern_num = 0;
	for (int i = 0; i < (int)symbol->patterns.size(); ++i)
	{
		if (symbol->patterns[i].type == AreaSymbol::FillPattern::PointPattern)
		{
			point_pattern_num++;
			QString name = tr("Point pattern %1").arg(point_pattern_num);
			symbol->patterns[i].name = name;
			symbol->patterns[i].point->setName(name);
			pattern_list->item(i)->setText(name);
			renamePropertiesGroup(point_pattern_num + 1, name);
		}
		else
		{
			line_pattern_num++;
			QString name = tr("Line pattern %1").arg(line_pattern_num);
			symbol->patterns[i].name = name;
			pattern_list->item(i)->setText(symbol->patterns[i].name);
		}
	}
}

void AreaSymbolSettings::updatePatternWidgets()
{
	int index = pattern_list->currentRow();
	bool pattern_active = (index >= 0);
	if (pattern_active)
		active_pattern = symbol->patterns.begin() + index;
	
	del_pattern_button->setEnabled(pattern_active);
	
	AreaSymbol::FillPattern* pattern = pattern_active ? &*active_pattern : new AreaSymbol::FillPattern();
	
	pattern_angle_edit->blockSignals(true);
	pattern_rotatable_check->blockSignals(true);
	pattern_spacing_edit->blockSignals(true);
	pattern_line_offset_edit->blockSignals(true);
	
	pattern_angle_edit->setValue(pattern->angle * 180.0 / M_PI);
	pattern_angle_edit->setEnabled(pattern_active);
	pattern_rotatable_check->setChecked(pattern->rotatable);
	pattern_rotatable_check->setEnabled(pattern_active);
	pattern_spacing_edit->setValue(0.001 * pattern->line_spacing);
	pattern_spacing_edit->setEnabled(pattern_active);
	pattern_line_offset_edit->setValue(0.001 * pattern->line_offset);
	pattern_line_offset_edit->setEnabled(pattern_active);
	
	pattern_angle_edit->blockSignals(false);
	pattern_rotatable_check->blockSignals(false);
	pattern_spacing_edit->blockSignals(false);
	pattern_line_offset_edit->blockSignals(false);
	
	if (pattern->type == AreaSymbol::FillPattern::LinePattern)
	{
		pattern_offset_along_line_label->hide();
		pattern_offset_along_line_edit->hide();
		pattern_pointdist_label->hide();
		pattern_pointdist_edit->hide();
		
		pattern_linewidth_edit->blockSignals(true);
		pattern_color_edit->blockSignals(true);
		
		pattern_linewidth_label->show();
		pattern_linewidth_edit->show();
		pattern_linewidth_edit->setValue(0.001 * pattern->line_width);
		pattern_linewidth_edit->setEnabled(pattern_active);
		pattern_color_label->show();
		pattern_color_edit->show();
		pattern_color_edit->setColor(pattern->line_color);
		pattern_color_edit->setEnabled(pattern_active);
		
		pattern_linewidth_edit->blockSignals(false);
		pattern_color_edit->blockSignals(false);
	}
	else
	{
		pattern_color_label->hide();
		pattern_color_edit->hide();
		pattern_linewidth_label->hide();
		pattern_linewidth_edit->hide();
		
		pattern_offset_along_line_edit->blockSignals(true);
		pattern_pointdist_edit->blockSignals(true);
		
		pattern_offset_along_line_label->show();
		pattern_offset_along_line_edit->show();
		pattern_offset_along_line_edit->setValue(0.001 * pattern->offset_along_line);
		pattern_offset_along_line_edit->setEnabled(pattern_active);
		pattern_pointdist_label->show();
		pattern_pointdist_edit->show();
		pattern_pointdist_edit->setValue(0.001 * pattern->point_distance);
		pattern_pointdist_edit->setEnabled(pattern_active);
		
		pattern_offset_along_line_edit->blockSignals(false);
		pattern_pointdist_edit->blockSignals(false);
	}
	
	if (!pattern_active)
		delete pattern;
}

void AreaSymbolSettings::addLinePattern()
{
	addPattern(AreaSymbol::FillPattern::LinePattern);
}

void AreaSymbolSettings::addPointPattern()
{
	addPattern(AreaSymbol::FillPattern::PointPattern);
}

void AreaSymbolSettings::addPattern(AreaSymbol::FillPattern::Type type)
{
	int index = pattern_list->currentRow() + 1;
	if (index < 0)
		index = symbol->patterns.size();
	
	active_pattern = symbol->patterns.insert(symbol->patterns.begin() + index, AreaSymbol::FillPattern());
	QString temp_name("new");
	pattern_list->insertItem(index, temp_name);
	assert(int(symbol->patterns.size()) == pattern_list->count());
	
	active_pattern->type = type;
	if (type == AreaSymbol::FillPattern::PointPattern)
	{
		active_pattern->point = new PointSymbol();
		PointSymbolEditorWidget* editor = new PointSymbolEditorWidget(controller, active_pattern->point, 16);
		connect(editor, SIGNAL(symbolEdited()), this, SIGNAL(propertiesModified()) );
		if (pattern_list->currentRow() == int(symbol->patterns.size()) - 1)
		{
			addPropertiesGroup(temp_name, editor); 	
		}
		else
		{
			int group_index = indexOfPropertiesGroup(symbol->patterns[pattern_list->currentRow()+1].name);
			insertPropertiesGroup(group_index, temp_name, editor);
		}
	}
	updatePatternNames();
	emit propertiesModified();
	
	pattern_list->setCurrentRow(index);
}

void AreaSymbolSettings::deleteActivePattern()
{
	int index = pattern_list->currentRow();
	if (index < 0)
	{
		qWarning() << "AreaSymbolSettings::deleteActivePattern(): no pattern active.";
		return;
	}
	
	if (active_pattern->type == AreaSymbol::FillPattern::PointPattern)
	{
		removePropertiesGroup(active_pattern->name);
		delete active_pattern->point;
		active_pattern->point = NULL;
	}
	symbol->patterns.erase(active_pattern);
	
	delete pattern_list->takeItem(index);
	updatePatternNames();
	updatePatternWidgets();
	
	emit propertiesModified();
	
	index = pattern_list->currentRow();
	selectPattern(index);
}

void AreaSymbolSettings::selectPattern(int index)
{
	active_pattern = symbol->patterns.begin() + index;
	updatePatternWidgets();
}

void AreaSymbolSettings::colorChanged()
{
	symbol->color = color_edit->color();
	emit propertiesModified();
}

void AreaSymbolSettings::minimumSizeChanged(double value)
{
	symbol->minimum_area = qRound(1000.0 * value);
	emit propertiesModified();
}

void AreaSymbolSettings::patternAngleChanged(double value)
{
	active_pattern->angle = value * (2*M_PI) / 360.0;
	emit propertiesModified();
}

void AreaSymbolSettings::patternRotatableClicked(bool checked)
{
	active_pattern->rotatable = checked;
	emit propertiesModified();
}

void AreaSymbolSettings::patternSpacingChanged(double value)
{
	active_pattern->line_spacing = qRound(1000.0 * value);
	emit propertiesModified();
}

void AreaSymbolSettings::patternLineOffsetChanged(double value)
{
	active_pattern->line_offset = qRound(1000.0 * value);
	emit propertiesModified();
}

void AreaSymbolSettings::patternOffsetAlongLineChanged(double value)
{
	assert(active_pattern->type == AreaSymbol::FillPattern::PointPattern);
	active_pattern->offset_along_line = qRound(1000.0 * value);
	emit propertiesModified();
}

void AreaSymbolSettings::patternColorChanged()
{
	assert(active_pattern->type == AreaSymbol::FillPattern::LinePattern);
	active_pattern->line_color = pattern_color_edit->color();
	emit propertiesModified();
}

void AreaSymbolSettings::patternLineWidthChanged(double value)
{
	assert(active_pattern->type == AreaSymbol::FillPattern::LinePattern);
	active_pattern->line_width = qRound(1000.0 * value);
	emit propertiesModified();
}

void AreaSymbolSettings::patternPointDistChanged(double value)
{
	assert(active_pattern->type == AreaSymbol::FillPattern::PointPattern);
	active_pattern->point_distance = qRound(1000.0 * value);
	emit propertiesModified();
}

#include "symbol_area.moc"
