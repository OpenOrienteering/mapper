/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2014, 2015 Kai Pastor
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


MapView::MapView(Map* map)
 : map{ map }
 , zoom{ 1.0 }
 , rotation{ 0.0 }
 , map_visibility{ new TemplateVisibility { 1.0f, true } }
 , all_templates_hidden{ false }
 , grid_visible{ false }
 , overprinting_simulation_enabled{ false }
{
	updateTransform(NoChange);
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
	qint64 center_x, center_y;
	int unused;
	file->read((char*)&zoom, sizeof(double));
	file->read((char*)&rotation, sizeof(double));
	file->read((char*)&center_x, sizeof(qint64));
	file->read((char*)&center_y, sizeof(qint64));
	file->read((char*)&unused /*view_x*/, sizeof(int));
	file->read((char*)&unused /*view_y*/, sizeof(int));
	file->read((char*)&pan_offset, sizeof(QPoint));
	
	center_pos = MapCoord::fromNative(center_x, center_y);
	updateTransform(CenterChange | ZoomChange | RotationChange);
	
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
	mapview_element.writeAttribute(literal::position_x, center_pos.nativeX());
	mapview_element.writeAttribute(literal::position_y, center_pos.nativeY());
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
	auto center_x = mapview_element.attribute<qint32>(literal::position_x);
	auto center_y = mapview_element.attribute<qint32>(literal::position_y);
	center_pos = MapCoord::fromNative(center_x, center_y);
	grid_visible = mapview_element.attribute<bool>(literal::grid);
	overprinting_simulation_enabled = mapview_element.attribute<bool>(literal::overprinting_simulation_enabled);
	updateTransform(CenterChange | ZoomChange | RotationChange);
	
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

MapCoord MapView::viewToMap(double x, double y) const
{
	return MapCoord(view_to_map.m11() * x + view_to_map.m12() * y + view_to_map.m13(),
	                view_to_map.m21() * x + view_to_map.m22() * y + view_to_map.m23());
}

MapCoordF MapView::viewToMapF(double x, double y) const
{
	return MapCoordF(view_to_map.m11() * x + view_to_map.m12() * y + view_to_map.m13(),
	                 view_to_map.m21() * x + view_to_map.m22() * y + view_to_map.m23());
}

QPointF MapView::mapToView(MapCoord coords) const
{
	return QPointF(map_to_view.m11() * coords.x() + map_to_view.m12() * coords.y() + map_to_view.m13(),
	               map_to_view.m21() * coords.x() + map_to_view.m22() * coords.y() + map_to_view.m23());
}

QPointF MapView::mapToView(MapCoordF coords) const
{
	return QPointF(map_to_view.m11() * coords.x() + map_to_view.m12() * coords.y() + map_to_view.m13(),
	               map_to_view.m21() * coords.x() + map_to_view.m22() * coords.y() + map_to_view.m23());
}

qreal MapView::lengthToPixel(qreal length) const
{
	return Util::mmToPixelPhysical(zoom * length / 1000.0);
}

qreal MapView::pixelToLength(qreal pixel) const
{
	return Util::pixelToMMPhysical(pixel / zoom) * 1000.0;
}

QRectF MapView::calculateViewedRect(QRectF rect) const
{
	auto top_left     = viewToMapF(rect.topLeft());
	auto top_right    = viewToMapF(rect.topRight());
	auto bottom_right = viewToMapF(rect.bottomRight());
	auto bottom_left  = viewToMapF(rect.bottomLeft());
	
	rect = QRectF{ top_left, bottom_right }.normalized();
	rectInclude(rect, top_right);
	rectInclude(rect, bottom_left);
	rect.adjust(-0.001, -0.001, +0.001, +0.001);
	return rect;
}

QRectF MapView::calculateViewBoundingBox(QRectF rect) const
{
	auto top_left     = mapToView(static_cast<MapCoordF>(rect.topLeft()));
	auto top_right    = mapToView(static_cast<MapCoordF>(rect.topRight()));
	auto bottom_right = mapToView(static_cast<MapCoordF>(rect.bottomRight()));
	auto bottom_left  = mapToView(static_cast<MapCoordF>(rect.bottomLeft()));
	
	rect = QRectF{ top_left, bottom_right }.normalized();
	rectInclude(rect, top_right);
	rectInclude(rect, bottom_left);
	rect.adjust(-1.0, -1.0, +1.0, +1.0);
	return rect;
}

void MapView::setPanOffset(QPoint offset)
{
	if (offset != pan_offset)
	{
		pan_offset = offset;
		for (auto widget : widgets)
			widget->setPanOffset(pan_offset);
	}
}

void MapView::finishPanning(QPoint offset)
{
	setPanOffset({0,0});
	
	auto rotated_offset = MapCoord::fromNative(qRound64(-pixelToLength(offset.x())),
	                                           qRound64(-pixelToLength(offset.y())) );
	auto rotated_offset_f = MapCoordF{ rotated_offset };
	rotated_offset_f.rotate(-rotation);
	auto move = MapCoord{ rotated_offset_f };
	setCenter(center() + move);
}

void MapView::zoomSteps(float num_steps, bool preserve_cursor_pos, QPointF cursor_pos_view)
{
	auto zoom_to = getZoom() * pow(sqrt(2.0), num_steps);
	
	if (preserve_cursor_pos)
	{
		setZoom(zoom_to, cursor_pos_view);
	}
	else
	{
		setZoom(zoom_to);
		
		auto mouse_pos_map = viewToMapF(cursor_pos_view);
		for (auto widget : widgets)
			widget->updateCursorposLabel(mouse_pos_map);
	}
}

void MapView::setZoom(double value, QPointF center)
{
	auto pos = this->center();
	auto zoom_pos = viewToMap(center);
	auto old_zoom = getZoom();
	
	setZoom(value);
	
	if (!qFuzzyCompare(old_zoom, getZoom()))
	{
		auto zoom_factor = getZoom() / old_zoom ;
		setCenter(zoom_pos + (pos - zoom_pos) / zoom_factor);
	}
}

void MapView::setZoom(double value)
{
	zoom = qBound(zoom_out_limit, value, zoom_in_limit);
	updateTransform(ZoomChange);
}

void MapView::setRotation(float value)
{
	rotation = value;
	updateTransform(RotationChange);
}

void MapView::setCenter(MapCoord pos)
{
	center_pos = pos;
	updateTransform(CenterChange);
}

void MapView::updateTransform(ChangeFlags change)
{
	double final_zoom = calculateFinalZoomFactor();
	double final_zoom_cosr = final_zoom * cos(rotation);
	double final_zoom_sinr = final_zoom * sin(rotation);
	auto center_x = center_pos.x();
	auto center_y = center_pos.y();
	
	// Create map_to_view
	map_to_view.setMatrix(final_zoom_cosr, -final_zoom_sinr, -final_zoom_cosr * center_x + final_zoom_sinr * center_y,
	                      final_zoom_sinr,  final_zoom_cosr, -final_zoom_sinr * center_x - final_zoom_cosr * center_y,
	                      0, 0, 1);
	view_to_map     = map_to_view.inverted();
	world_transform = map_to_view.transposed();
	
	for (auto widget : widgets)
		widget->viewChanged(change);
}

const TemplateVisibility* MapView::effectiveMapVisibility() const
{
	static TemplateVisibility opaque    { 1.0f, true };
	static TemplateVisibility invisible { 0.0f, false };
	if (all_templates_hidden)
		return &opaque;
	else if (map_visibility->opacity < 0.005f)
		return &invisible;
	else
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
		static const TemplateVisibility dummy { 1.0f, false };
		return &dummy;
	}
	return template_visibilities.value(temp);
}

TemplateVisibility* MapView::getTemplateVisibility(const Template* temp)
{
	if (!template_visibilities.contains(temp))
	{
		template_visibilities.insert(temp, new TemplateVisibility { 1.0, true });
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
