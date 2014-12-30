/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2014 Kai Pastor
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


#include "map_view.h"

#include <QRectF>

#include "../map.h"
#include "../map_widget.h"
#include "../util.h"
#include "../util/xml_stream_util.h"


namespace literal
{
	static const QLatin1String zoom("zoom");
	static const QLatin1String rotation("rotation");
	static const QLatin1String position_x("position_x");
	static const QLatin1String position_y("position_y");
	static const QLatin1String view_x("view_x");
	static const QLatin1String view_y("view_y");
	static const QLatin1String grid("grid");
	static const QLatin1String overprinting_simulation_enabled("overprinting_simulation_enabled");
	static const QLatin1String map("map");
	static const QLatin1String opacity("opacity");
	static const QLatin1String visible("visible");
	static const QLatin1String templates("templates");
	static const QLatin1String hidden("hidden");
	static const QLatin1String ref("ref");
	static const QLatin1String template_string("template");
}

const double MapView::zoom_in_limit = 512;
const double MapView::zoom_out_limit = 1 / 16.0;


MapView::MapView(Map* map) : map(map)
{
	zoom = 1;
	rotation = 0;
	position_x = 0;
	position_y = 0;
	view_x = 0;
	view_y = 0;
	map_visibility = new TemplateVisibility();
	map_visibility->visible = true;
	all_templates_hidden = false;
	grid_visible = false;
	overprinting_simulation_enabled = false;
	updateTransform();
}

MapView::~MapView()
{
	foreach (TemplateVisibility* vis, template_visibilities)
		delete vis;
	delete map_visibility;
}

#ifndef NO_NATIVE_FILE_FORMAT

void MapView::load(QIODevice* file, int version)
{
	file->read((char*)&zoom, sizeof(double));
	file->read((char*)&rotation, sizeof(double));
	file->read((char*)&position_x, sizeof(qint64));
	file->read((char*)&position_y, sizeof(qint64));
	file->read((char*)&view_x, sizeof(int));
	file->read((char*)&view_y, sizeof(int));
	file->read((char*)&pan_offset, sizeof(QPoint));
	updateTransform();
	
	if (version >= 26)
	{
		file->read((char*)&map_visibility->visible, sizeof(bool));
		file->read((char*)&map_visibility->opacity, sizeof(float));
	}
	
	int num_template_visibilities;
	file->read((char*)&num_template_visibilities, sizeof(int));
	
	for (int i = 0; i < num_template_visibilities; ++i)
	{
		int pos;
		file->read((char*)&pos, sizeof(int));
		
		TemplateVisibility* vis = getTemplateVisibility(map->getTemplate(pos));
		file->read((char*)&vis->visible, sizeof(bool));
		file->read((char*)&vis->opacity, sizeof(float));
	}
	
	if (version >= 29)
		file->read((char*)&all_templates_hidden, sizeof(bool));
	
	if (version >= 24)
		file->read((char*)&grid_visible, sizeof(bool));
}

#endif

void MapView::save(QXmlStreamWriter& xml, const QLatin1String& element_name, bool skip_templates)
{
	XmlElementWriter mapview_element(xml, element_name);
	mapview_element.writeAttribute(literal::zoom, zoom);
	mapview_element.writeAttribute(literal::rotation, rotation);
	mapview_element.writeAttribute(literal::position_x, position_x);
	mapview_element.writeAttribute(literal::position_y, position_y);
	mapview_element.writeAttribute(literal::view_x, view_x);
	mapview_element.writeAttribute(literal::view_y, view_y);
	mapview_element.writeAttribute(literal::grid, grid_visible);
	mapview_element.writeAttribute(literal::overprinting_simulation_enabled, overprinting_simulation_enabled);
	
	{
		XmlElementWriter map_element(xml, literal::map);
		map_element.writeAttribute(literal::opacity, map_visibility->opacity);
		map_element.writeAttribute(literal::visible, map_visibility->visible);
	}
	
	{
		if (!skip_templates)
		{
			XmlElementWriter templates_element(xml, literal::templates);
			templates_element.writeAttribute(literal::hidden, all_templates_hidden);
			templates_element.writeAttribute(XmlStreamLiteral::count, template_visibilities.size());
			
			QHash<const Template*, TemplateVisibility*>::const_iterator it = template_visibilities.constBegin();
			for ( ; it != template_visibilities.constEnd(); ++it)
			{
				XmlElementWriter ref_element(xml, literal::ref);
				ref_element.writeAttribute(literal::template_string, map->findTemplateIndex(it.key()));
				ref_element.writeAttribute(literal::visible, (*it)->visible);
				ref_element.writeAttribute(literal::opacity, (*it)->opacity);
			}
		}
	}
}

void MapView::load(QXmlStreamReader& xml)
{
	XmlElementReader mapview_element(xml);
	zoom = mapview_element.attribute<double>(literal::zoom);
	if (zoom < 0.001)
		zoom = 1.0;
	rotation = mapview_element.attribute<double>(literal::rotation);
	position_x = mapview_element.attribute<qint64>(literal::position_x);
	position_y = mapview_element.attribute<qint64>(literal::position_y);
	view_x = mapview_element.attribute<int>(literal::view_x);
	view_y = mapview_element.attribute<int>(literal::view_y);
	grid_visible = mapview_element.attribute<bool>(literal::grid);
	overprinting_simulation_enabled = mapview_element.attribute<bool>(literal::overprinting_simulation_enabled);
	updateTransform();
	
	while (xml.readNextStartElement())
	{
		if (xml.name() == literal::map)
		{
			XmlElementReader map_element(xml);
			map_visibility->opacity = map_element.attribute<float>(literal::opacity);
			if (map_element.hasAttribute(literal::visible))
				map_visibility->visible = map_element.attribute<bool>(literal::visible);
			else
				map_visibility->visible = true;
		}
		else if (xml.name() == literal::templates)
		{
			XmlElementReader templates_element(xml);
			int num_template_visibilities = templates_element.attribute<int>(XmlStreamLiteral::count);
			if (num_template_visibilities > 0)
				template_visibilities.reserve(qMin(num_template_visibilities, 20)); // 20 is not a limit
			all_templates_hidden = templates_element.attribute<bool>(literal::hidden);
			
			while (xml.readNextStartElement())
			{
				if (xml.name() == literal::ref)
				{
					XmlElementReader ref_element(xml);
					int pos = ref_element.attribute<int>(literal::template_string);
					if (pos >= 0 && pos < map->getNumTemplates())
					{
						TemplateVisibility* vis = getTemplateVisibility(map->getTemplate(pos));
						vis->visible = ref_element.attribute<bool>(literal::visible);
						vis->opacity = ref_element.attribute<float>(literal::opacity);
					}
				}
				else
					xml.skipCurrentElement();
			}
		}
		else
			xml.skipCurrentElement(); // unsupported
	}
}

void MapView::addMapWidget(MapWidget* widget)
{
	widgets.push_back(widget);
	map->addMapWidget(widget);
}

void MapView::removeMapWidget(MapWidget* widget)
{
	widgets.erase(std::remove(widgets.begin(), widgets.end(), widget), widgets.end());
	map->removeMapWidget(widget);
}

void MapView::updateAllMapWidgets()
{
	for (auto widget : widgets)
		widget->updateEverything();
}

double MapView::lengthToPixel(qint64 length) const
{
	return Util::mmToPixelPhysical(zoom * (length / 1000.0));
}

qint64 MapView::pixelToLength(double pixel) const
{
	return qRound64(1000 * Util::pixelToMMPhysical(pixel / zoom));
}

QRectF MapView::calculateViewedRect(QRectF rect) const
{
	QPointF min = rect.topLeft();
	QPointF max = rect.bottomRight();
	MapCoordF top_left = viewToMapF(min.x(), min.y());
	MapCoordF top_right = viewToMapF(max.x(), min.y());
	MapCoordF bottom_right = viewToMapF(max.x(), max.y());
	MapCoordF bottom_left = viewToMapF(min.x(), max.y());
	
	rect.setCoords(top_left.getX(), top_left.getY(), top_left.getX(), top_left.getY());
	rectInclude(rect, QPointF(top_right.getX(), top_right.getY()));
	rectInclude(rect, QPointF(bottom_right.getX(), bottom_right.getY()));
	rectInclude(rect, QPointF(bottom_left.getX(), bottom_left.getY()));
	rect.adjust(-0.001, -0.001, +0.001, +0.001);
	return rect;
}

QRectF MapView::calculateViewBoundingBox(QRectF rect) const
{
	QPointF min = rect.topLeft();
	MapCoord map_min = MapCoord(min.x(), min.y());
	QPointF max = rect.bottomRight();
	MapCoord map_max = MapCoord(max.x(), max.y());
	
	QPointF top_left = mapToView(map_min);
	QPointF bottom_right = mapToView(map_max);
	
	MapCoord map_top_right = MapCoord(max.x(), min.y());
	QPointF top_right = mapToView(map_top_right);
	
	MapCoord map_bottom_left = MapCoord(min.x(), max.y());
	QPointF bottom_left = mapToView(map_bottom_left);
	
	rect.setCoords(top_left.x(), top_left.y(), top_left.x(), top_left.y());
	rectInclude(rect, QPointF(top_right.x(), top_right.y()));
	rectInclude(rect, QPointF(bottom_right.x(), bottom_right.y()));
	rectInclude(rect, QPointF(bottom_left.x(), bottom_left.y()));
	rect.adjust(-1.0, -1.0, +1.0, +1.0);
	return rect;
}

void MapView::setPanOffset(QPoint offset)
{
	pan_offset = offset;
	for (auto widget : widgets)
		widget->setPanOffset(pan_offset);
}

void MapView::finishPanning(QPoint offset)
{
	MapCoordF rotated_offset(offset.x(), offset.y());
	rotated_offset.rotate(-rotation);
	
	pan_offset = QPoint();
	qint64 move_x = -pixelToLength(rotated_offset.getX());
	qint64 move_y = -pixelToLength(rotated_offset.getY());
	
	position_x += move_x;
	position_y += move_y;
	updateTransform();
	
	for (auto widget : widgets)
		widget->finishPanning(move_x, move_y);
}

bool MapView::zoomSteps(float num_steps, bool preserve_cursor_pos, QPointF cursor_pos_view)
{
	num_steps *= 0.5f;
	
	if (num_steps > 0)
	{
		// Zooming in - adjust camera position so the cursor stays at the same position on the map
		if (getZoom() >= zoom_in_limit)
			return false;
		
		bool set_to_limit = false;
		double zoom_to = pow(2, (log10(getZoom()) / LOG2) + num_steps);
		double zoom_factor = zoom_to / getZoom();
		if (getZoom() * zoom_factor > zoom_in_limit)
		{
			zoom_factor = zoom_in_limit / getZoom();
			set_to_limit = true;
		}
		
		MapCoordF mouse_pos_map(0, 0);
		MapCoordF mouse_pos_to_view_center(0, 0);
		if (preserve_cursor_pos)
		{
			mouse_pos_map = viewToMapF(cursor_pos_view);
			mouse_pos_to_view_center = MapCoordF(getPositionX()/1000.0 - mouse_pos_map.getX(), getPositionY()/1000.0 - mouse_pos_map.getY());
			mouse_pos_to_view_center = MapCoordF(mouse_pos_to_view_center.getX() * 1 / zoom_factor, mouse_pos_to_view_center.getY() * 1 / zoom_factor);
		}
		
		setZoom(set_to_limit ? zoom_in_limit : (getZoom() * zoom_factor));
		if (preserve_cursor_pos)
		{
			setPositionX(qRound64(1000 * (mouse_pos_map.getX() + mouse_pos_to_view_center.getX())));
			setPositionY(qRound64(1000 * (mouse_pos_map.getY() + mouse_pos_to_view_center.getY())));
		}
	}
	else
	{
		// Zooming out
		if (getZoom() <= zoom_out_limit)
			return false;
		
		bool set_to_limit = false;
		double zoom_to = pow(2, (log10(getZoom()) / LOG2) + num_steps);
		double zoom_factor = zoom_to / getZoom();
		if (getZoom() * zoom_factor < zoom_out_limit)
		{
			zoom_factor = zoom_out_limit / getZoom();
			set_to_limit = true;
		}
		
		MapCoordF mouse_pos_map(0, 0);
		MapCoordF mouse_pos_to_view_center(0, 0);
		if (preserve_cursor_pos)
		{
			mouse_pos_map = viewToMapF(cursor_pos_view);
			mouse_pos_to_view_center = MapCoordF(getPositionX()/1000.0 - mouse_pos_map.getX(), getPositionY()/1000.0 - mouse_pos_map.getY());
			mouse_pos_to_view_center = MapCoordF(mouse_pos_to_view_center.getX() * 1 / zoom_factor, mouse_pos_to_view_center.getY() * 1 / zoom_factor);
		}
		
		setZoom(set_to_limit ? zoom_out_limit : (getZoom() * zoom_factor));
		
		if (preserve_cursor_pos)
		{
			setPositionX(qRound64(1000 * (mouse_pos_map.getX() + mouse_pos_to_view_center.getX())));
			setPositionY(qRound64(1000 * (mouse_pos_map.getY() + mouse_pos_to_view_center.getY())));
		}
		else
		{
			mouse_pos_map = viewToMapF(cursor_pos_view);
			for (auto widget : widgets)
				widget->updateCursorposLabel(mouse_pos_map);
		}
	}
	return true;
}

void MapView::setZoom(float value)
{
	float zoom_factor = value / zoom;
	for (int i = 0; i < (int)widgets.size(); ++i)
		widgets[i]->zoom(zoom_factor);
	
	zoom = value;
	updateTransform();
	
	for (auto widget : widgets)
		widget->updateZoomLabel();
}

void MapView::setZoom(double value, QPointF center)
{
	value = qBound(zoom_out_limit, value, zoom_in_limit);
	double zoom_factor = value / getZoom();
	
	MapCoordF zoom_center_map = viewToMapF(center);
	MapCoordF view_center_map = MapCoordF(getPositionX()/1000.0 - zoom_center_map.getX(), getPositionY()/1000.0 - zoom_center_map.getY());
	view_center_map = MapCoordF(view_center_map.getX() / zoom_factor, view_center_map.getY() / zoom_factor);
	
	setZoom(value);
	
	setPositionX(qRound64(1000 * (zoom_center_map.getX() + view_center_map.getX())));
	setPositionY(qRound64(1000 * (zoom_center_map.getY() + view_center_map.getY())));
}

void MapView::setPositionX(qint64 value)
{
	qint64 offset = value - position_x;
	for (auto widget : widgets)
		widget->moveView(offset, 0);
	
	position_x = value;
	updateTransform();
}

void MapView::setPositionY(qint64 value)
{
	qint64 offset = value - position_y;
	for (auto widget : widgets)
		widget->moveView(0, offset);
	
	position_y = value;
	updateTransform();
}

void MapView::updateTransform()
{
	double final_zoom = lengthToPixel(1000);
	double final_zoom_cosr = final_zoom * cos(rotation);
	double final_zoom_sinr = final_zoom * sin(rotation);
	double pos_x_by_1000 = position_x / 1000.0;
	double pos_y_by_1000 = position_y / 1000.0;
	
	// Create map_to_view
	map_to_view.setMatrix(final_zoom_cosr, -final_zoom_sinr, -final_zoom_cosr * pos_x_by_1000 + final_zoom_sinr * pos_y_by_1000 - view_x,
	                      final_zoom_sinr,  final_zoom_cosr, -final_zoom_sinr * pos_x_by_1000 - final_zoom_cosr * pos_y_by_1000 - view_y,
	                      0, 0, 1);
	view_to_map     = map_to_view.inverted();
	world_transform = map_to_view.transposed();
}

const TemplateVisibility* MapView::getMapVisibility() const
{
	return map_visibility;
}

TemplateVisibility* MapView::getMapVisibility()
{
	return map_visibility;
}

bool MapView::isTemplateVisible(const Template* temp) const
{
	if (template_visibilities.contains(temp))
	{
		const TemplateVisibility* vis = template_visibilities.value(temp);
		return vis->visible && vis->opacity > 0;
	}
	else
		return false;
}

const TemplateVisibility* MapView::getTemplateVisibility(const Template* temp) const
{
	if (!template_visibilities.contains(temp))
	{
		static const TemplateVisibility dummy;
		return &dummy;
	}
	return template_visibilities.value(temp);
}

TemplateVisibility* MapView::getTemplateVisibility(const Template* temp)
{
	if (!template_visibilities.contains(temp))
	{
		template_visibilities.insert(temp, new TemplateVisibility());
	}
	
	return const_cast<TemplateVisibility*>(static_cast<const MapView*>(this)->getTemplateVisibility(temp));
}

void MapView::deleteTemplateVisibility(const Template* temp)
{
	delete template_visibilities.value(temp);
	template_visibilities.remove(temp);
}

void MapView::setHideAllTemplates(bool value)
{
	if (all_templates_hidden != value)
	{
		all_templates_hidden = value;
		updateAllMapWidgets();
	}
}

void MapView::setGridVisible(bool visible)
{
	if (grid_visible != visible)
	{
		grid_visible = visible;
		updateAllMapWidgets();
	}
}

void MapView::setOverprintingSimulationEnabled(bool enabled)
{
	if (overprinting_simulation_enabled != enabled)
	{
		overprinting_simulation_enabled = enabled;
		updateAllMapWidgets();
	}
}
