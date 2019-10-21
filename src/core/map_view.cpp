/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2014-2018  Kai Pastor
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

#include <algorithm>
#include <cmath>
#include <iterator>
#include <stdexcept>

#include <QRectF>

#include <Qt>
#include <QLatin1String>
#include <QString>
#include <QStringRef>
#include <QXmlStreamReader>

#include "core/map.h"
#include "core/map_coord.h"
#include "gui/util_gui.h"
#include "templates/template.h" // IWYU pragma: keep
#include "util/util.h"
#include "util/xml_stream_util.h"


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



namespace OpenOrienteering {

// ### TemplateVisibility ###

bool TemplateVisibility::hasAlpha() const
{
	return visible && opacity > 0 && opacity < 1;
}


// ### MapView ###

const double MapView::zoom_in_limit = 512;
const double MapView::zoom_out_limit = 1 / 16.0;


MapView::MapView(QObject* parent, Map* map)
: QObject  { parent }
, map      { map }
, zoom     { 1.0 }
, rotation { 0.0 }
, map_visibility{ 1.0f, true }
, all_templates_hidden{ false }
, grid_visible{ false }
, overprinting_simulation_enabled{ false }
{
	Q_ASSERT(map);
	updateTransform();
	connect(map, &Map::templateAdded, this, &MapView::onTemplateAdded);
	connect(map, &Map::templateDeleted, this, &MapView::onTemplateDeleted, Qt::QueuedConnection);
}

MapView::MapView(Map* map)
: MapView { map, map }
{
	// nothing else
}

MapView::~MapView()
{
	// nothing, not inlined
}



void MapView::save(QXmlStreamWriter& xml, const QLatin1String& element_name, bool template_details) const
{
	// We do not save transient attributes such as rotation (for compass) or pan offset.
	XmlElementWriter mapview_element(xml, element_name);
	mapview_element.writeAttribute(literal::zoom, zoom);
	mapview_element.writeAttribute(literal::position_x, center_pos.nativeX());
	mapview_element.writeAttribute(literal::position_y, center_pos.nativeY());
	mapview_element.writeAttribute(literal::grid, grid_visible);
	mapview_element.writeAttribute(literal::overprinting_simulation_enabled, overprinting_simulation_enabled);
	
	{
		XmlElementWriter map_element(xml, literal::map);
		map_element.writeAttribute(literal::opacity, map_visibility.opacity);
		map_element.writeAttribute(literal::visible, map_visibility.visible);
	}
	
	{
		XmlElementWriter templates_element(xml, literal::templates);
		templates_element.writeAttribute(literal::hidden, all_templates_hidden);
		if (template_details)
		{
			templates_element.writeAttribute(XmlStreamLiteral::count, template_visibilities.size());
			for (auto entry : template_visibilities)
			{
				XmlElementWriter ref_element(xml, literal::ref);
				ref_element.writeAttribute(literal::template_string, map->findTemplateIndex(entry.temp));
				ref_element.writeAttribute(literal::visible, entry.visible);
				ref_element.writeAttribute(literal::opacity, entry.opacity);
			}
		}
	}
}

void MapView::load(QXmlStreamReader& xml)
{
	// We do not load transient attributes such as rotation (for compass) or pan offset.
	XmlElementReader mapview_element(xml);
	zoom = qMin(mapview_element.attribute<double>(literal::zoom), zoom_in_limit);
	if (zoom < zoom_out_limit)
		zoom = 1.0;
	
	auto center_x = mapview_element.attribute<qint64>(literal::position_x);
	auto center_y = mapview_element.attribute<qint64>(literal::position_y);
	try
	{
		center_pos = MapCoord::fromNative64withOffset(center_x, center_y);
	}
	catch (std::range_error&)
	{
		// leave center_pos unchanged
	}
	updateTransform();
	
	grid_visible = mapview_element.attribute<bool>(literal::grid);
	overprinting_simulation_enabled = mapview_element.attribute<bool>(literal::overprinting_simulation_enabled);
	
	while (xml.readNextStartElement())
	{
		if (xml.name() == literal::map)
		{
			XmlElementReader map_element(xml);
			map_visibility.opacity = map_element.attribute<float>(literal::opacity);
			if (map_element.hasAttribute(literal::visible))
				map_visibility.visible = map_element.attribute<bool>(literal::visible);
			else
				map_visibility.visible = true;
		}
		else if (xml.name() == literal::templates)
		{
			XmlElementReader templates_element(xml);
			auto num_template_visibilities = templates_element.attribute<unsigned int>(XmlStreamLiteral::count);
			template_visibilities.reserve(qBound(20u, num_template_visibilities, 1000u));
			all_templates_hidden = templates_element.attribute<bool>(literal::hidden);
			
			while (xml.readNextStartElement())
			{
				if (xml.name() == literal::ref)
				{
					XmlElementReader ref_element(xml);
					int pos = ref_element.attribute<int>(literal::template_string);
					if (pos >= 0 && pos < map->getNumTemplates())
					{
						TemplateVisibility vis { 
						  qBound(0.0f, ref_element.attribute<float>(literal::opacity), 1.0f),
						  ref_element.attribute<bool>(literal::visible)
						};
						setTemplateVisibilityHelper(map->getTemplate(pos), vis);
					}
				}
				else
					xml.skipCurrentElement();
			}
		}
		else
			xml.skipCurrentElement(); // unsupported
	}
	
	emit viewChanged(CenterChange | ZoomChange | RotationChange);
	emit visibilityChanged(MultipleFeatures, true);
}

void MapView::updateAllMapWidgets()
{
	emit visibilityChanged(MultipleFeatures, true);
}

QPointF MapView::mapToView(const MapCoord& coords) const
{
	return map_to_view.map(MapCoordF(coords));
}

QPointF MapView::mapToView(const QPointF& coords) const
{
	return map_to_view.map(coords);
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

void MapView::setPanOffset(const QPoint& offset)
{
	if (offset != pan_offset)
	{
		pan_offset = offset;
		emit panOffsetChanged(offset);
	}
}

void MapView::finishPanning(const QPoint& offset)
{
	setPanOffset({0,0});
	try
	{
		auto rotated_offset = MapCoord::fromNative64(qRound64(-pixelToLength(offset.x())),
													 qRound64(-pixelToLength(offset.y())) );
		auto rotated_offset_f = MapCoordF{ rotated_offset };
		rotated_offset_f.rotate(-rotation);
		auto move = MapCoord{ rotated_offset_f };
		setCenter(center() + move);
	}
	catch (std::range_error&)
	{
		// Do nothing
	}
}

void MapView::zoomSteps(double num_steps, const QPointF& cursor_pos_view)
{
	auto zoom_to = getZoom() * pow(sqrt(2.0), num_steps);
	setZoom(zoom_to, cursor_pos_view);
}

void MapView::zoomSteps(double num_steps)
{
	auto zoom_to = getZoom() * pow(sqrt(2.0), num_steps);
	setZoom(zoom_to);
}

void MapView::setZoom(double value, const QPointF& center)
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
	updateTransform();
	emit viewChanged(ZoomChange);
}

void MapView::setRotation(double value)
{
	rotation = value;
	updateTransform();
	emit viewChanged(RotationChange);
}

void MapView::setCenter(const MapCoord& pos)
{
	center_pos = pos;
	updateTransform();
	emit viewChanged(CenterChange);
}

void MapView::updateTransform()
{
	double final_zoom = calculateFinalZoomFactor();
	double final_zoom_cosr = final_zoom * cos(rotation);
	double final_zoom_sinr = final_zoom * sin(rotation);
	auto center_x = center_pos.x();
	auto center_y = center_pos.y();
	
	// Create map_to_view
	map_to_view = { final_zoom_cosr,  final_zoom_sinr,
	                -final_zoom_sinr, final_zoom_cosr,
	                -final_zoom_cosr * center_x + final_zoom_sinr * center_y,
	                -final_zoom_sinr * center_x - final_zoom_cosr * center_y };
	view_to_map = map_to_view.inverted();
}


TemplateVisibility MapView::effectiveMapVisibility() const
{
	if (all_templates_hidden)
		return { 1.0f, true };
	else if (map_visibility.opacity < 0.005f)
		return { 0.0f, false };
	else
		return map_visibility;
}

void MapView::setMapVisibility(TemplateVisibility vis)
{
	if (map_visibility != vis)
	{
		map_visibility = vis;
		emit visibilityChanged(VisibilityFeature::MapVisible, vis.visible && vis.opacity > 0, nullptr);
	}
}

MapView::TemplateVisibilityVector::const_iterator MapView::findVisibility(const Template* temp) const
{
	return std::find_if(begin(template_visibilities), end(template_visibilities), [temp](const TemplateVisibilityEntry& entry)
	{
		return entry.temp == temp;
	} );
}

MapView::TemplateVisibilityVector::iterator MapView::findVisibility(const Template* temp)
{
	return std::find_if(begin(template_visibilities), end(template_visibilities), [temp](const TemplateVisibilityEntry& entry)
	{
		return entry.temp == temp;
	} );
}

bool MapView::isTemplateVisible(const Template* temp) const
{
	auto entry = findVisibility(temp);
	return entry != end(template_visibilities)
	       && entry->visible
	       && entry->opacity > 0;
}

TemplateVisibility MapView::getTemplateVisibility(const Template* temp) const
{
	auto entry = findVisibility(temp);
	if (entry != end(template_visibilities)) 
		return *entry;
	else
		return { 1.0f, false };
}

void MapView::setTemplateVisibility(Template* temp, TemplateVisibility vis)
{
	auto visible = vis.visible && vis.opacity > 0;
	if (visible
	    && temp->getTemplateState() != Template::Loaded
	    && !templateLoadingBlocked())
	{
		vis.visible = visible = temp->loadTemplateFile(false);
	}
	
	if (setTemplateVisibilityHelper(temp, vis))
	{
		emit visibilityChanged(VisibilityFeature::TemplateVisible, visible, temp);
	}
}

bool MapView::setTemplateVisibilityHelper(const Template *temp, TemplateVisibility vis)
{
	auto stored = findVisibility(temp);
	if (stored == end(template_visibilities)) 
	{
		template_visibilities.emplace_back(temp, vis);
		return true;
	}
	if (*stored != vis)
	{
		stored->opacity = vis.opacity;
		stored->visible = vis.visible;
		return true;
	}
	return false;
}

void MapView::onTemplateAdded(int, Template* temp)
{
	setTemplateVisibility(temp, { 1.0f, true });
}

void MapView::onTemplateDeleted(int, const Template* temp)
{
	template_visibilities.erase(findVisibility(temp));
}


void MapView::setAllTemplatesHidden(bool value)
{
	if (all_templates_hidden != value)
	{
		all_templates_hidden = value;
		emit visibilityChanged(VisibilityFeature::AllTemplatesHidden, value);
	}
}

void MapView::setGridVisible(bool visible)
{
	if (grid_visible != visible)
	{
		grid_visible = visible;
		emit visibilityChanged(VisibilityFeature::GridVisible, visible);
	}
}

void MapView::setOverprintingSimulationEnabled(bool enabled)
{
	if (overprinting_simulation_enabled != enabled)
	{
		overprinting_simulation_enabled = enabled;
		emit visibilityChanged(VisibilityFeature::OverprintingEnabled, enabled);
	}
}



bool MapView::hasAlpha() const
{
	auto map_visibility = effectiveMapVisibility();
	if (map_visibility.hasAlpha() || map->hasAlpha())
		return true;
	
	if (grid_visible && map->getGrid().hasAlpha())
		return true;
		
	for (int i = 0; i < map->getNumTemplates(); ++i)
	{
		auto temp = map->getTemplate(i);
		auto visibility = getTemplateVisibility(temp);
		if (visibility.hasAlpha() || temp->hasAlpha())
			return true;
	}
	
	return false;
}



void MapView::setTemplateLoadingBlocked(bool blocked)
{
	template_loading_blocked = blocked;
}


}  // namespace OpenOrienteering
