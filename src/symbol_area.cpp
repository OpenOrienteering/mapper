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


#include "symbol_area.h"

#include <cassert>

#if QT_VERSION < 0x050000
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QIODevice>
#include <QXmlStreamWriter>

#include "core/map_color.h"
#include "map.h"
#include "object.h"
#include "renderable_implementation.h"
#include "symbol_line.h"
#include "symbol_point.h"
#include "symbol_point_editor.h"
#include "symbol_properties_widget.h"
#include "symbol_setting_dialog.h"
#include "gui/widgets/color_dropdown.h"
#include "util_gui.h"
#include "util.h"

// ### FillPattern ###

AreaSymbol::FillPattern::FillPattern()
{
	type = LinePattern;
	angle = 0;
	rotatable = false;
	line_spacing = 5000; // 5 mm
	line_offset = 0;
	offset_along_line = 0;
	
	line_color = NULL;
	line_width = 200; // 0.2 mm
	
	point_distance = 5000; // 5 mm
	point = NULL;
}

bool AreaSymbol::FillPattern::load(QIODevice* file, int version, Map* map)
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
			if (version < 21)
				point->setRotatable(true);
		}
		else
			point = NULL;
	}
	return true;
}

void AreaSymbol::FillPattern::save(QXmlStreamWriter& xml, const Map& map) const
{
	xml.writeStartElement("pattern");
	xml.writeAttribute("type", QString::number(type));
	xml.writeAttribute("angle", QString::number(angle));
	if (rotatable)
		xml.writeAttribute("rotatable", "true");
	xml.writeAttribute("line_spacing", QString::number(line_spacing));
	xml.writeAttribute("line_offset", QString::number(line_offset));
	xml.writeAttribute("offset_along_line", QString::number(offset_along_line));
	
	if (type == LinePattern)
	{
		xml.writeAttribute("color", QString::number(map.findColorIndex(line_color)));
		xml.writeAttribute("line_width", QString::number(line_width));
	}
	else
	{
		xml.writeAttribute("point_distance", QString::number(point_distance));
		if (point != NULL)
			point->save(xml, map);
	}
	
	xml.writeEndElement(/*pattern*/);
}

void AreaSymbol::FillPattern::load(QXmlStreamReader& xml, const Map& map, SymbolDictionary& symbol_dict)
{
	Q_ASSERT (xml.name() == "pattern");
	
	QXmlStreamAttributes attributes = xml.attributes();
	type = static_cast<Type>(attributes.value("type").toString().toInt());
	angle = attributes.value("angle").toString().toFloat();
	rotatable = (attributes.value("rotatable") == "true");
	line_spacing = attributes.value("line_spacing").toString().toInt();
	line_offset = attributes.value("line_offset").toString().toInt();
	offset_along_line = attributes.value("offset_along_line").toString().toInt();
	
	if (type == LinePattern)
	{
		int temp = attributes.value("color").toString().toInt();
		line_color = map.getColor(temp);
		line_width = attributes.value("line_width").toString().toInt();
		xml.skipCurrentElement();
	}
	else
	{
		point_distance = attributes.value("point_distance").toString().toInt();
		while (xml.readNextStartElement())
		{
			if (xml.name() == "symbol")
				point = static_cast<PointSymbol*>(Symbol::load(xml, map, symbol_dict));
			else
				xml.skipCurrentElement();
		}
	}
}

bool AreaSymbol::FillPattern::equals(AreaSymbol::FillPattern& other, Qt::CaseSensitivity case_sensitivity)
{
	if (type != other.type)
		return false;
	if (qAbs(angle - other.angle) > 1e-05)
		return false;
	if (rotatable != other.rotatable)
		return false;
	if (line_spacing != other.line_spacing)
		return false;
	if (line_offset != other.line_offset)
		return false;
	
	if (type == PointPattern)
	{
		if (offset_along_line != other.offset_along_line)
			return false;
		if (point_distance != other.point_distance)
			return false;
		if ((point == NULL && other.point != NULL) ||
			(point != NULL && other.point == NULL))
			return false;
		if (point && !point->equals(other.point, case_sensitivity))
			return false;
	}
	else if (type == LinePattern)
	{
		if (!MapColor::equal(line_color, other.line_color))
			return false;
		if (line_width != other.line_width)
			return false;
	}
	
	if (name.compare(other.name, case_sensitivity) != 0)
		return false;
	
	return true;
}

void AreaSymbol::FillPattern::createRenderables(QRectF extent, float delta_rotation, const MapCoord& pattern_origin, ObjectRenderables& output)
{
	if (line_spacing <= 0)
		return;
	if (type == PointPattern && point_distance <= 0)
		return;
	
	// Make rotation unique
	double rotation = angle;
	if (rotatable)
		rotation += delta_rotation;
	rotation = fmod(1.0 * rotation, M_PI);
	if (rotation < 0)
		rotation = M_PI + rotation;
	assert(rotation >= 0 && rotation <= M_PI);
	
	// Helpers
	LineSymbol line;
	PathObject path(NULL);
	PointObject point_object(point);
	if (point && point->isRotatable())
		point_object.setRotation(delta_rotation);
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
	float delta_line_offset = 0;
	float delta_along_line_offset = 0;
	if (rotatable)
	{
		MapCoordF line_normal(0, -1);
		line_normal.rotate(rotation);
		line_normal.setY(-line_normal.getY());
		delta_line_offset = line_normal.dot(MapCoordF(pattern_origin));
		
		MapCoordF line_tangent(1, 0);
		line_tangent.rotate(rotation);
		line_tangent.setY(-line_tangent.getY());
		delta_along_line_offset = line_tangent.dot(MapCoordF(pattern_origin));
	}
	
	const float offset = 0.001f * line_offset + delta_line_offset;
	if (qAbs(rotation - M_PI/2) < 0.0001)
	{
		// Special case: vertical lines
		delta_along_line_offset = -delta_along_line_offset;
		
		double first = offset + ceil((extent.left() - offset) / (0.001*line_spacing)) * 0.001*line_spacing;
		for (double cur = first; cur < extent.right(); cur += 0.001*line_spacing)
		{
			coords[0] = MapCoordF(cur, extent.top());
			coords[1] = MapCoordF(cur, extent.bottom());
			createLine(coords, delta_along_line_offset, &line, &path, &point_object, output);
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
			createLine(coords, delta_along_line_offset, &line, &path, &point_object, output);
		}
	}
	else
	{
		// General case
		if (rotation < M_PI / 2)
			delta_along_line_offset = -delta_along_line_offset;
		
		float xfactor = 1.0f / sin(rotation);
		float yfactor = 1.0f / cos(rotation);
		
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
				createLine(coords, delta_along_line_offset, &line, &path, &point_object, output);
				
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
				createLine(coords, delta_along_line_offset, &line, &path, &point_object, output);
				
				// Move to next position
				start_x += dist_x;
				end_y += dist_y;
			} while (true);
		}
	}
}

void AreaSymbol::FillPattern::createLine(MapCoordVectorF& coords, float delta_offset, LineSymbol* line, PathObject* path, PointObject* point_object, ObjectRenderables& output)
{
	if (type == LinePattern)
		line->createRenderables(path, path->getRawCoordinateVector(), coords, output);
	else
	{
		MapCoordF to_end = MapCoordF(coords[1].getX() - coords[0].getX(), coords[1].getY() - coords[0].getY());
		float length = to_end.length();
		float dir_x = to_end.getX() / length;
		float dir_y = to_end.getY() / length;
		
		float offset = 0.001f * offset_along_line + delta_offset;
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

Symbol* AreaSymbol::duplicate(const MapColorMap* color_map) const
{
	AreaSymbol* new_area = new AreaSymbol();
	new_area->duplicateImplCommon(this);
	new_area->color = color_map ? color_map->value(color) : color;
	new_area->minimum_area = minimum_area;
	new_area->patterns = patterns;
	for (int i = 0; i < (int)new_area->patterns.size(); ++i)
	{
		if (new_area->patterns[i].type == FillPattern::PointPattern)
			new_area->patterns[i].point = static_cast<PointSymbol*>(new_area->patterns[i].point->duplicate(color_map));
		else if (new_area->patterns[i].type == FillPattern::LinePattern && color_map)
			new_area->patterns[i].line_color = color_map->value(new_area->patterns[i].line_color);
	}
	return new_area;
}

void AreaSymbol::createRenderables(Object* object, const MapCoordVector& flags, const MapCoordVectorF& coords, ObjectRenderables& output)
{
	if (coords.size() < 3)
		return;
	
	Map* map = object->getMap();
	if (map && map->isAreaHatchingEnabled())
		Symbol::createBaselineRenderables(object, this, flags, coords, output, true);
	else
		createRenderablesNormal(object, flags, coords, output);
}
void AreaSymbol::createRenderablesNormal(Object* object, const MapCoordVector& flags, const MapCoordVectorF& coords, ObjectRenderables& output)
{
	if (coords.size() < 3)
		return;
	
	// The shape output is even created if the area is not filled with a color
	// because the QPainterPath created by it is needed as clip path for the fill objects
	PathObject* path = static_cast<PathObject*>(object);
	AreaRenderable* color_fill = new AreaRenderable(this, coords, flags, &path->getPathCoordinateVector());
	output.insertRenderable(color_fill);
	
	QPainterPath* old_clip_path = output.getClipPath();
	output.setClipPath(color_fill->getPainterPath());
	int size = (int)patterns.size();
	for (int i = 0; i < size; ++i)
		patterns[i].createRenderables(color_fill->getExtent(), path->getPatternRotation(), path->getPatternOrigin(), output);
	output.setClipPath(old_clip_path);
}
void AreaSymbol::colorDeleted(const MapColor* color)
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
			patterns[i].point->colorDeleted(color);
		else if (patterns[i].line_color == color)
		{
			patterns[i].line_color = NULL;
			change = true;
		}
	}
	
	if (change)
		resetIcon();
}
bool AreaSymbol::containsColor(const MapColor* color) const
{
	if (color == this->color)
		return true;
	
	for (int i = 0; i < (int)patterns.size(); ++i)
	{
		if (patterns[i].type == FillPattern::PointPattern && patterns[i].point && patterns[i].point->containsColor(color))
			return true;
		else if (patterns[i].type == FillPattern::LinePattern && patterns[i].line_color == color)
			return true;
	}
	
	return false;
}

const MapColor* AreaSymbol::getDominantColorGuess() const
{
	if (color)
		return color;
	
	// Hope that the first pattern's color is representative
	for (int i = 0; i < (int)patterns.size(); ++i)
	{
		if (patterns[i].type == FillPattern::PointPattern && patterns[i].point)
		{
			const MapColor* dominant_color = patterns[i].point->getDominantColorGuess();
			if (dominant_color) return dominant_color;
		}
		else if (patterns[i].type == FillPattern::LinePattern && patterns[i].line_color)
			return patterns[i].line_color;
	}
	
	return NULL;
}

void AreaSymbol::scale(double factor)
{
	minimum_area = qRound(factor*factor * minimum_area);
	
	int size = (int)patterns.size();
	for (int i = 0; i < size; ++i)
		patterns[i].scale(factor);
	
	resetIcon();
}

bool AreaSymbol::hasRotatableFillPattern() const
{
	for (int i = 0, size = (int)patterns.size(); i < size; ++i)
	{
		if (patterns[i].rotatable)
			return true;
	}
	return false;
}

bool AreaSymbol::loadImpl(QIODevice* file, int version, Map* map)
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

void AreaSymbol::saveImpl(QXmlStreamWriter& xml, const Map& map) const
{
	xml.writeStartElement("area_symbol");
	xml.writeAttribute("inner_color", QString::number(map.findColorIndex(color)));
	xml.writeAttribute("min_area", QString::number(minimum_area));
	
	int num_patterns = (int)patterns.size();
	xml.writeAttribute("patterns", QString::number(num_patterns));
	for (int i = 0; i < num_patterns; ++i)
		patterns[i].save(xml, map);
	xml.writeEndElement(/*area_symbol*/);
}

bool AreaSymbol::loadImpl(QXmlStreamReader& xml, const Map& map, SymbolDictionary& symbol_dict)
{
	if (xml.name() != "area_symbol")
		return false;
	
	QXmlStreamAttributes attributes = xml.attributes();
	int temp = attributes.value("inner_color").toString().toInt();
	color = map.getColor(temp);
	minimum_area = attributes.value("min_area").toString().toInt();
	
	int num_patterns = attributes.value("patterns").toString().toInt();
	patterns.reserve(num_patterns % 5); // 5 is not the limit
	while (xml.readNextStartElement())
	{
		if (xml.name() == "pattern")
		{
			patterns.push_back(FillPattern());
			patterns.back().load(xml, map, symbol_dict);
		}
		else
			xml.skipCurrentElement();
	}
	
	return true;
}

bool AreaSymbol::equalsImpl(Symbol* other, Qt::CaseSensitivity case_sensitivity)
{
	AreaSymbol* area = static_cast<AreaSymbol*>(other);
	if (!MapColor::equal(color, area->color))
		return false;
	if (minimum_area != area->minimum_area)
		return false;
	
	// TODO: Fill patterns are only compared in order
	if (patterns.size() != area->patterns.size())
		return false;
	for (size_t i = 0, end = patterns.size(); i < end; ++i)
	{
		if (!patterns[i].equals(area->patterns[i], case_sensitivity))
			return false;
	}
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
	
	
	QFormLayout* general_layout = new QFormLayout();
	
	color_edit = new ColorDropDown(map);
	general_layout->addRow(tr("Area color:"), color_edit);
	
	minimum_size_edit = Util::SpinBox::create(1, 0.0, 999999.9, trUtf8("mm²"));
	general_layout->addRow(tr("Minimum size:"), minimum_size_edit);
	
	general_layout->addItem(Util::SpacerItem::create(this));
	
	
	QVBoxLayout* fill_patterns_list_layout = new QVBoxLayout();
	
	fill_patterns_list_layout->addWidget(Util::Headline::create(tr("Fills")), 0);
	
	pattern_list = new QListWidget();
	fill_patterns_list_layout->addWidget(pattern_list, 1);
	
	del_pattern_button = new QPushButton(QIcon(":/images/minus.png"), "");
	add_pattern_button = new QToolButton();
	add_pattern_button->setIcon(QIcon(":/images/plus.png"));
	add_pattern_button->setToolButtonStyle(Qt::ToolButtonIconOnly);
	add_pattern_button->setPopupMode(QToolButton::InstantPopup);
	add_pattern_button->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
	add_pattern_button->setMinimumSize(del_pattern_button->sizeHint());
	QMenu* add_fill_button_menu = new QMenu(add_pattern_button);
	add_fill_button_menu->addAction(tr("Line fill"), this, SLOT(addLinePattern()));
	add_fill_button_menu->addAction(tr("Pattern fill"), this, SLOT(addPointPattern()));
	add_pattern_button->setMenu(add_fill_button_menu);
	
	QHBoxLayout* buttons_layout = new QHBoxLayout();
	buttons_layout->addStretch(1);
	buttons_layout->addWidget(add_pattern_button);
	buttons_layout->addWidget(del_pattern_button);
	buttons_layout->addStretch(1);
	fill_patterns_list_layout->addLayout(buttons_layout, 0);
	
	
	QFormLayout* fill_pattern_layout = new QFormLayout();
	
	pattern_name_edit = new QLabel();
	fill_pattern_layout->addRow(pattern_name_edit);
	
	fill_pattern_layout->addItem(Util::SpacerItem::create(this));
	
	
	/* From here, stacked widgets are used to unify the layout of pattern type dependant fields. */
	QStackedWidget* single_line_headline = new QStackedWidget();
	connect(this, SIGNAL(switchPatternEdits(int)), single_line_headline, SLOT(setCurrentIndex(int)));
	fill_pattern_layout->addRow(single_line_headline);
	
	QStackedWidget* single_line_label1 = new QStackedWidget();
	connect(this, SIGNAL(switchPatternEdits(int)), single_line_label1, SLOT(setCurrentIndex(int)));
	QStackedWidget* single_line_edit1  = new QStackedWidget();
	connect(this, SIGNAL(switchPatternEdits(int)), single_line_edit1, SLOT(setCurrentIndex(int)));
	fill_pattern_layout->addRow(single_line_label1, single_line_edit1);
	
	QStackedWidget* single_line_label2 = new QStackedWidget();
	connect(this, SIGNAL(switchPatternEdits(int)), single_line_label2, SLOT(setCurrentIndex(int)));
	QStackedWidget* single_line_edit2  = new QStackedWidget();
	connect(this, SIGNAL(switchPatternEdits(int)), single_line_edit2, SLOT(setCurrentIndex(int)));
	fill_pattern_layout->addRow(single_line_label2, single_line_edit2);
	
	QStackedWidget* single_line_label3 = new QStackedWidget();
	connect(this, SIGNAL(switchPatternEdits(int)), single_line_label3, SLOT(setCurrentIndex(int)));
	pattern_line_offset_edit = Util::SpinBox::create(2, -999999.9, 999999.9, tr("mm"));
	fill_pattern_layout->addRow(single_line_label3, pattern_line_offset_edit);
	
	fill_pattern_layout->addItem(Util::SpacerItem::create(this));
	
	QStackedWidget* parallel_lines_headline = new QStackedWidget();
	connect(this, SIGNAL(switchPatternEdits(int)), parallel_lines_headline, SLOT(setCurrentIndex(int)));
	fill_pattern_layout->addRow(parallel_lines_headline);
	
	QStackedWidget* parallel_lines_label1 = new QStackedWidget();
	connect(this, SIGNAL(switchPatternEdits(int)), parallel_lines_label1, SLOT(setCurrentIndex(int)));
	pattern_spacing_edit = Util::SpinBox::create(2, 0.0, 999999.9, tr("mm"));
	fill_pattern_layout->addRow(parallel_lines_label1, pattern_spacing_edit);
	
	/* Stacked widgets, index 0: line pattern fields */
	single_line_headline->addWidget(Util::Headline::create(tr("Single line")));
	
	QLabel* pattern_linewidth_label = new QLabel(tr("Line width:"));
	pattern_linewidth_edit = Util::SpinBox::create(2, 0.0, 999999.9, tr("mm"));
	single_line_label1->addWidget(pattern_linewidth_label);
	single_line_edit1 ->addWidget(pattern_linewidth_edit);
	
	QLabel* pattern_color_label = new QLabel(tr("Line color:"));
	pattern_color_edit = new ColorDropDown(map);
	single_line_label2->addWidget(pattern_color_label);
	single_line_edit2 ->addWidget(pattern_color_edit);
	
	single_line_label3->addWidget(new QLabel(tr("Line offset:")));
	
	parallel_lines_headline->addWidget(Util::Headline::create(tr("Parallel lines")));
	
	parallel_lines_label1->addWidget(new QLabel(tr("Line spacing:")));
	
	/* Stacked widgets, index 1: point pattern fields */
	single_line_headline->addWidget(Util::Headline::create(tr("Single row")));
	
	QLabel* pattern_pointdist_label = new QLabel(tr("Pattern interval:"));
	pattern_pointdist_edit = Util::SpinBox::create(2, 0, 999999.9, tr("mm"));
	single_line_label1->addWidget(pattern_pointdist_label);
	single_line_edit1 ->addWidget(pattern_pointdist_edit);
	
	QLabel* pattern_offset_along_line_label = new QLabel(tr("Pattern offset:"));
	pattern_offset_along_line_edit = Util::SpinBox::create(2, -999999.9, 999999.9, tr("mm"));
	single_line_label2->addWidget(pattern_offset_along_line_label);
	single_line_edit2 ->addWidget(pattern_offset_along_line_edit);
	
	single_line_label3->addWidget(new QLabel(tr("Row offset:")));
	
	parallel_lines_headline->addWidget(Util::Headline::create(tr("Parallel rows")));
	
	parallel_lines_label1->addWidget(new QLabel(tr("Row spacing:")));
	
	/* General fields again. Not stacked. */
	fill_pattern_layout->addItem(Util::SpacerItem::create(this));
	
	
	fill_pattern_layout->addRow(Util::Headline::create(tr("Fill rotation")));
	
	pattern_angle_edit = Util::SpinBox::create(1, 0.0, 360.0, trUtf8("°"));
	fill_pattern_layout->addRow(tr("Angle:"), pattern_angle_edit);
	
	pattern_rotatable_check = new QCheckBox(tr("adjustable per object"));
	fill_pattern_layout->addRow("", pattern_rotatable_check);
	
	
	QHBoxLayout* fill_patterns_layout = new QHBoxLayout();
	fill_patterns_layout->addLayout(fill_patterns_list_layout, 1);
	fill_patterns_layout->addItem(Util::SpacerItem::create(this));
	fill_patterns_layout->addLayout(fill_pattern_layout, 3);
	
	QBoxLayout* layout = new QVBoxLayout();
	layout->addLayout(general_layout);
	layout->addLayout(fill_patterns_layout);
	
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
	updatePatternWidgets();
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
			QString name = tr("Pattern fill %1").arg(point_pattern_num);
			symbol->patterns[i].name = name;
			symbol->patterns[i].point->setName(name);
			pattern_list->item(i)->setText(name);
			renamePropertiesGroup(point_pattern_num + 1, name);
		}
		else
		{
			line_pattern_num++;
			QString name = tr("Line fill %1").arg(line_pattern_num);
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
	pattern_name_edit->setText(pattern_active ? QString("<b>%1</b>").arg(active_pattern->name) : tr("No fill selected"));
	
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
		emit switchPatternEdits(0);
		
		pattern_linewidth_edit->blockSignals(true);
		pattern_linewidth_edit->setValue(0.001 * pattern->line_width);
		pattern_linewidth_edit->setEnabled(pattern_active);
		pattern_linewidth_edit->blockSignals(false);
		
		pattern_color_edit->blockSignals(true);
		pattern_color_edit->setColor(pattern->line_color);
		pattern_color_edit->setEnabled(pattern_active);
		pattern_color_edit->blockSignals(false);
	}
	else
	{
		emit switchPatternEdits(1);
		
		pattern_offset_along_line_edit->blockSignals(true);
		pattern_offset_along_line_edit->setValue(0.001 * pattern->offset_along_line);
		pattern_offset_along_line_edit->setEnabled(pattern_active);
		pattern_offset_along_line_edit->blockSignals(false);
		
		pattern_pointdist_edit->blockSignals(true);
		pattern_pointdist_edit->setValue(0.001 * pattern->point_distance);
		pattern_pointdist_edit->setEnabled(pattern_active);
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
		active_pattern->point->setRotatable(true);
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
	else if (map->getNumColors() > 0)
	{
		// Default color for lines
		active_pattern->line_color = map->getColor(0);
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
		qWarning() << "AreaSymbolSettings::deleteActivePattern(): no fill selected."; // not translated
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
	active_pattern->angle = value * M_PI / 180.0;
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
