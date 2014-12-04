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

#include <QDebug>
#include <QPainter>
#include <QRectF>

#include "../map.h"
#include "../map_widget.h"
#include "../util.h"
#include "../util/xml_stream_util.h"


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
	update();
	//map->addMapView(this);
}
MapView::~MapView()
{
	//map->removeMapView(this);
	
	foreach (TemplateVisibility* vis, template_visibilities)
		delete vis;
	delete map_visibility;
}

void MapView::load(QIODevice* file, int version)
{
	file->read((char*)&zoom, sizeof(double));
	file->read((char*)&rotation, sizeof(double));
	file->read((char*)&position_x, sizeof(qint64));
	file->read((char*)&position_y, sizeof(qint64));
	file->read((char*)&view_x, sizeof(int));
	file->read((char*)&view_y, sizeof(int));
	file->read((char*)&drag_offset, sizeof(QPoint));
	update();
	
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

void MapView::save(QXmlStreamWriter& xml, const QString& element_name, bool skip_templates)
{
	xml.writeStartElement(element_name);
	
	xml.writeAttribute("zoom", QString::number(zoom));
	xml.writeAttribute("rotation", QString::number(rotation));
	xml.writeAttribute("position_x", QString::number(position_x));
	xml.writeAttribute("position_y", QString::number(position_y));
	xml.writeAttribute("view_x", QString::number(view_x));
	xml.writeAttribute("view_y", QString::number(view_y));
	xml.writeAttribute("drag_offset_x", QString::number(drag_offset.x()));
	xml.writeAttribute("drag_offset_y", QString::number(drag_offset.y()));
	if (grid_visible)
		xml.writeAttribute("grid", "true");
	if (overprinting_simulation_enabled)
		xml.writeAttribute("overprinting_simulation_enabled", "true");
	
	xml.writeEmptyElement("map");
	if (!map_visibility->visible)
		xml.writeAttribute("visible", "false");
	xml.writeAttribute("opacity", QString::number(map_visibility->opacity));
	
	xml.writeStartElement("templates");
	if (all_templates_hidden)
		xml.writeAttribute("hidden", "true");
	if (!skip_templates)
	{
		int num_template_visibilities = template_visibilities.size();
		xml.writeAttribute("count", QString::number(num_template_visibilities));
		
		QHash<const Template*, TemplateVisibility*>::const_iterator it = template_visibilities.constBegin();
		for ( ; it != template_visibilities.constEnd(); ++it)
		{
			xml.writeEmptyElement("ref");
			int pos = map->findTemplateIndex(it.key());
			xml.writeAttribute("template", QString::number(pos));
			xml.writeAttribute("visible", (*it)->visible ? "true" : "false");
			xml.writeAttribute("opacity", QString::number((*it)->opacity));
		}
	}
	xml.writeEndElement(/*templates*/);
	
	xml.writeEndElement(/*map_view*/); 
}

void MapView::load(QXmlStreamReader& xml)
{
	{	// Limit scope of variable "attributes" to this block
		QXmlStreamAttributes attributes = xml.attributes();
		zoom = attributes.value("zoom").toString().toDouble();
		if (zoom < 0.001)
			zoom = 1.0;
		rotation = attributes.value("rotation").toString().toDouble();
		position_x = attributes.value("position_x").toString().toLongLong();
		position_y = attributes.value("position_y").toString().toLongLong();
		view_x = attributes.value("view_x").toString().toInt();
		view_y = attributes.value("view_y").toString().toInt();
		drag_offset.setX(attributes.value("drag_offset_x").toString().toInt());
		drag_offset.setY(attributes.value("drag_offset_y").toString().toInt());
		grid_visible = (attributes.value("grid") == "true");
		overprinting_simulation_enabled = (attributes.value("overprinting_simulation_enabled") == "true");
		update();
	}
	
	while (xml.readNextStartElement())
	{
		if (xml.name() == "map")
		{
			map_visibility->visible = !(xml.attributes().value("visible") == "false");
			map_visibility->opacity = xml.attributes().value("opacity").toString().toFloat();
			xml.skipCurrentElement();
		}
		else if (xml.name() == "templates")
		{
			int num_template_visibilities = xml.attributes().value("count").toString().toInt();
			template_visibilities.reserve(qMin(num_template_visibilities, 20)); // 20 is not a limit
			all_templates_hidden = (xml.attributes().value("hidden") == "true");
			
			while (xml.readNextStartElement())
			{
				if (xml.name() == "ref")
				{
					int pos = xml.attributes().value("template").toString().toInt();
					if (pos >= 0 && pos < map->getNumTemplates())
					{
						TemplateVisibility* vis = getTemplateVisibility(map->getTemplate(pos));
						vis->visible = !(xml.attributes().value("visible") == "false");
						vis->opacity = xml.attributes().value("opacity").toString().toFloat();
					}
					else
						qDebug() << QString("Invalid template visibility reference %1 at %2:%3").
						            arg(pos).arg(xml.lineNumber()).arg(xml.columnNumber());
				}
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
	for (int i = 0; i < (int)widgets.size(); ++i)
	{
		if (widgets[i] == widget)
		{
			widgets.erase(widgets.begin() + i);
			return;
		}
	}
	Q_ASSERT(false);
	
	map->removeMapWidget(widget);
}
void MapView::updateAllMapWidgets()
{
	for (int i = 0; i < (int)widgets.size(); ++i)
		widgets[i]->updateEverything();
}

double MapView::lengthToPixel(qint64 length)
{
	return Util::mmToPixelPhysical(zoom * (length / 1000.0));
}
qint64 MapView::pixelToLength(double pixel)
{
	return qRound64(1000 * Util::pixelToMMPhysical(pixel / zoom));
}

QRectF MapView::calculateViewedRect(QRectF view_rect)
{
	QPointF min = view_rect.topLeft();
	QPointF max = view_rect.bottomRight();
	MapCoordF top_left = viewToMapF(min.x(), min.y());
	MapCoordF top_right = viewToMapF(max.x(), min.y());
	MapCoordF bottom_right = viewToMapF(max.x(), max.y());
	MapCoordF bottom_left = viewToMapF(min.x(), max.y());
	
	QRectF result = QRectF(top_left.getX(), top_left.getY(), 0, 0);
	rectInclude(result, QPointF(top_right.getX(), top_right.getY()));
	rectInclude(result, QPointF(bottom_right.getX(), bottom_right.getY()));
	rectInclude(result, QPointF(bottom_left.getX(), bottom_left.getY()));
	
	return QRectF(result.left() - 0.001, result.top() - 0.001, result.width() + 0.002, result.height() + 0.002);
}
QRectF MapView::calculateViewBoundingBox(QRectF map_rect)
{
	QPointF min = map_rect.topLeft();
	MapCoord map_min = MapCoord(min.x(), min.y());
	QPointF max = map_rect.bottomRight();
	MapCoord map_max = MapCoord(max.x(), max.y());
	
	QPointF top_left;
	mapToView(map_min, top_left.rx(), top_left.ry());
	QPointF bottom_right;
	mapToView(map_max, bottom_right.rx(), bottom_right.ry());
	
	MapCoord map_top_right = MapCoord(max.x(), min.y());
	QPointF top_right;
	mapToView(map_top_right, top_right.rx(), top_right.ry());
	
	MapCoord map_bottom_left = MapCoord(min.x(), max.y());
	QPointF bottom_left;
	mapToView(map_bottom_left, bottom_left.rx(), bottom_left.ry());
	
	QRectF result = QRectF(top_left.x(), top_left.y(), 0, 0);
	rectInclude(result, QPointF(top_right.x(), top_right.y()));
	rectInclude(result, QPointF(bottom_right.x(), bottom_right.y()));
	rectInclude(result, QPointF(bottom_left.x(), bottom_left.y()));
	
	return QRectF(result.left() - 1, result.top() - 1, result.width() + 2, result.height() + 2);
}

void MapView::applyTransform(QPainter* painter)
{
	QTransform world_transform;
	// NOTE: transposing the matrix here ...
	world_transform.setMatrix(map_to_view.get(0, 0), map_to_view.get(1, 0), map_to_view.get(2, 0),
							  map_to_view.get(0, 1), map_to_view.get(1, 1), map_to_view.get(2, 1),
							  map_to_view.get(0, 2), map_to_view.get(1, 2), map_to_view.get(2, 2));
	painter->setWorldTransform(world_transform, true);
}

void MapView::setDragOffset(QPoint offset, bool do_update)
{
	drag_offset = offset;
	for (int i = 0; i < (int)widgets.size(); ++i)
		widgets[i]->setDragOffset(drag_offset, do_update);
}

void MapView::completeDragging(QPoint offset, bool do_update)
{
	MapCoordF rotated_offset(offset.x(), offset.y());
	rotated_offset.rotate(-1 * rotation);
	
	drag_offset = QPoint(0, 0);
	qint64 move_x = -pixelToLength(rotated_offset.getX());
	qint64 move_y = -pixelToLength(rotated_offset.getY());
	
	position_x += move_x;
	position_y += move_y;
	update();
	
	for (int i = 0; i < (int)widgets.size(); ++i)
		widgets[i]->completeDragging(move_x, move_y, do_update);
}

bool MapView::zoomSteps(float num_steps, bool preserve_cursor_pos, QPointF cursor_pos_view)
{
	num_steps = 0.5f * num_steps;
	
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
			for (int i = 0; i < (int)widgets.size(); ++i)
				widgets[i]->updateCursorposLabel(mouse_pos_map);
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
	update();
	
	for (int i = 0; i < (int)widgets.size(); ++i)
		widgets[i]->updateZoomLabel();
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
	for (int i = 0; i < (int)widgets.size(); ++i)
		widgets[i]->moveView(offset, 0);
	
	position_x = value;
	update();
}
void MapView::setPositionY(qint64 value)
{
	qint64 offset = value - position_y;
	for (int i = 0; i < (int)widgets.size(); ++i)
		widgets[i]->moveView(0, offset);
	
	position_y = value;
	update();
}

void MapView::update()
{
	double cosr = cos(rotation);
	double sinr = sin(rotation);
	
	double final_zoom = lengthToPixel(1000);
	
	// Create map_to_view
	map_to_view.setSize(3, 3);
	map_to_view.set(0, 0, final_zoom * cosr);
	map_to_view.set(0, 1, final_zoom * (-sinr));
	map_to_view.set(1, 0, final_zoom * sinr);
	map_to_view.set(1, 1, final_zoom * cosr);
	map_to_view.set(0, 2, -final_zoom*(position_x/1000.0)*cosr + final_zoom*(position_y/1000.0)*sinr - view_x);
	map_to_view.set(1, 2, -final_zoom*(position_x/1000.0)*sinr - final_zoom*(position_y/1000.0)*cosr - view_y);
	map_to_view.set(2, 0, 0);
	map_to_view.set(2, 1, 0);
	map_to_view.set(2, 2, 1);
	
	// Create view_to_map
	map_to_view.invert(view_to_map);
}

TemplateVisibility *MapView::getMapVisibility()
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

TemplateVisibility* MapView::getTemplateVisibility(Template* temp)
{
	if (!template_visibilities.contains(temp))
	{
		TemplateVisibility* vis = new TemplateVisibility();
		template_visibilities.insert(temp, vis);
		return vis;
	}
	else
		return template_visibilities.value(temp);
}

void MapView::deleteTemplateVisibility(Template* temp)
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

