/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2014-2020, 2025 Kai Pastor
 *    Copyright 2025 Matthias Kühlewein
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
	static const QLatin1String template_sets("template_sets");
	static const QLatin1String template_set("template_set");
	static const QLatin1String active_template_set("active_template_set");
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
, grid_visible{ false }
, overprinting_simulation_enabled{ false }
{
	Q_ASSERT(map);
	updateTransform();
	connect(map, &Map::templateAboutToBeAdded, this, &MapView::onAboutToAddTemplate);
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
	
	// maintain legacy format:
	saveTemplateSet(xml, template_details, template_visibility_sets.getVisibility(0));
	
	if (getNumberOfVisibilitySets() < 2)
		return;		// don't duplicate first and only set
	
	XmlElementWriter template_sets_element(xml, literal::template_sets);
	template_sets_element.writeAttribute(XmlStreamLiteral::count, getNumberOfVisibilitySets());
	template_sets_element.writeAttribute(literal::active_template_set, getActiveVisibilityIndex());
	
	for (int i = 1; i < getNumberOfVisibilitySets(); ++i)
	{
		XmlElementWriter template_set_element(xml, literal::template_set);
		saveTemplateSet(xml, template_details, template_visibility_sets.getVisibility(i));
	}
}

void MapView::saveTemplateSet(QXmlStreamWriter& xml, bool template_details, const TemplateVisibilitySet& template_set) const
{
	{
		XmlElementWriter map_element(xml, literal::map);
		map_element.writeAttribute(literal::opacity, template_set.map_visibility.opacity);
		map_element.writeAttribute(literal::visible, template_set.map_visibility.visible);
	}
	
	{
		XmlElementWriter templates_element(xml, literal::templates);
		templates_element.writeAttribute(literal::hidden, template_set.all_templates_hidden);
		if (template_details)
		{
			templates_element.writeAttribute(XmlStreamLiteral::count, template_set.template_visibilities.size());
			for (const auto& entry : template_set.template_visibilities)
			{
				auto const index = map->findTemplateIndex(entry.templ);
				if (index < 0)
				{
					qWarning("Template visibility found for unknown template");
					continue;
				}
				
				XmlElementWriter ref_element(xml, literal::ref);
				ref_element.writeAttribute(literal::template_string, index);
				ref_element.writeAttribute(literal::visible, entry.visible);
				ref_element.writeAttribute(literal::opacity, entry.opacity);
			}
		}
	}
}

void MapView::load(QXmlStreamReader& xml, int version)
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
	
	int current_template_set = 0;
	int active_visibility_index = 0;
	auto& template_set = template_visibility_sets.accessVisibility(0);
	
	while (xml.readNextStartElement())
	{
		if (xml.name() == literal::template_sets)
		{
			XmlElementReader template_sets_element(xml);
			auto visibility_sets_num = template_sets_element.attribute<int>(XmlStreamLiteral::count);
			template_visibility_sets.template_visibility_sets.reserve(visibility_sets_num);
			active_visibility_index = template_sets_element.attribute<int>(literal::active_template_set);	// stored for later usage
			while (xml.readNextStartElement())
			{
				if (xml.name() == literal::template_set)
				{
					XmlElementReader template_set_element(xml);
					template_visibility_sets.template_visibility_sets.push_back(TemplateVisibilitySet());
					auto& template_set = template_visibility_sets.accessVisibility(++current_template_set);
					template_visibility_sets.setActiveVisibilityIndex(current_template_set);
					while (xml.readNextStartElement())
					{
						loadMapAndTemplates(xml, template_set, version);
					}
				}
				else
				{
					xml.skipCurrentElement();
				}
			}
		}
		else
		{
			loadMapAndTemplates(xml, template_set, version);
		}
	}
	if (active_visibility_index >= getNumberOfVisibilitySets())
		active_visibility_index = 0;
	template_visibility_sets.setActiveVisibilityIndex(active_visibility_index);
	
	emit viewChanged(CenterChange | ZoomChange | RotationChange);
	emit visibilityChanged(MultipleFeatures, true);
}

void MapView::loadMapAndTemplates(QXmlStreamReader& xml, TemplateVisibilitySet& template_set, int version)
{
	if (xml.name() == literal::map)
	{
		XmlElementReader map_element(xml);
		TemplateVisibility map_visibility;
		map_visibility.opacity = map_element.attribute<qreal>(literal::opacity);
		// Some old maps before version 6 lack visible="true".
		if (version >= 6)
			map_visibility.visible = map_element.attribute<bool>(literal::visible);
		else
			map_visibility.visible = true;
		template_set.map_visibility = map_visibility;
	}
	else if (xml.name() == literal::templates)
	{
		XmlElementReader templates_element(xml);
		auto num_template_visibilities = templates_element.attribute<unsigned int>(XmlStreamLiteral::count);
		template_set.template_visibilities.reserve(qBound(20u, num_template_visibilities, 1000u));
		template_set.all_templates_hidden = templates_element.attribute<bool>(literal::hidden);
		
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


void MapView::updateAllMapWidgets(VisibilityFeature change)
{
	emit visibilityChanged(change, true);
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
	if (template_visibility_sets.getCurrentVisibility().all_templates_hidden)
		return { 1.0f, true };
	else if (template_visibility_sets.getCurrentVisibility().map_visibility.opacity < 0.005)
		return { 0.0f, false };
	else
		return template_visibility_sets.getCurrentVisibility().map_visibility;
}

void MapView::setMapVisibility(TemplateVisibility vis)
{
	if (template_visibility_sets.getCurrentVisibility().map_visibility != vis)
	{
		template_visibility_sets.accessCurrentVisibility().map_visibility = vis;
		emit visibilityChanged(VisibilityFeature::MapVisible, vis.visible && vis.opacity > 0, nullptr);
	}
}

bool MapView::isTemplateVisible(const Template* templ) const
{
	if (template_visibility_sets.existsCurrentTemplateVisibility(templ))
	{
		auto current_visibility = template_visibility_sets.getCurrentTemplateVisibility(templ);
		return current_visibility.visible && current_visibility.opacity > 0;
	}
	return false;
}

TemplateVisibility MapView::getTemplateVisibility(const Template* templ) const
{
	if (template_visibility_sets.existsCurrentTemplateVisibility(templ))
		return template_visibility_sets.getCurrentTemplateVisibility(templ);
	else
		return { 1.0f, false };
}

void MapView::setTemplateVisibility(Template* templ, TemplateVisibility vis)
{
	if (setTemplateVisibilityHelper(templ, vis))
	{
		auto const visible = vis.visible && vis.opacity > 0;
		emit visibilityChanged(VisibilityFeature::TemplateVisible, visible, templ);
	}
}

bool MapView::setTemplateVisibilityHelper(const Template *templ, TemplateVisibility vis)
{
	if (!template_visibility_sets.existsCurrentTemplateVisibility(templ))
	{
		template_visibility_sets.addTemplate(templ, vis);
		return true;
	}
	if (template_visibility_sets.getCurrentTemplateVisibility(templ) != vis)	// can this condition ever be false?
	{
		template_visibility_sets.setCurrentTemplateVisibility(templ, vis);
		return true;
	}
	return false;
}

TemplateVisibility MapView::getMapVisibility() const
{
	return template_visibility_sets.getCurrentVisibility().map_visibility;
}

bool MapView::areAllTemplatesHidden() const
{
	return template_visibility_sets.getCurrentVisibility().all_templates_hidden;
}

void MapView::onAboutToAddTemplate(int, Template* templ)
{
	setTemplateVisibilityHelper(templ, { 1.0f, false });
}

void MapView::onTemplateAdded(int, Template* templ)
{
	setTemplateVisibility(templ, { 1.0f, true });
}

void MapView::onTemplateDeleted(int, const Template* templ)
{
	template_visibility_sets.deleteTemplate(templ);
}


void MapView::setAllTemplatesHidden(bool value)
{
	if (template_visibility_sets.getCurrentVisibility().all_templates_hidden != value)
	{
		template_visibility_sets.accessCurrentVisibility().all_templates_hidden = value;
		emit visibilityChanged(VisibilityFeature::AllTemplatesHidden, value);	// paramter 'value' is not used for VisibilityFeature::AllTemplatesHidden
	}
}

void MapView::setGridVisible(bool visible)
{
	if (grid_visible != visible)
	{
		grid_visible = visible;
		emit visibilityChanged(VisibilityFeature::GridVisible, visible);	// paramter 'visible' is not used for VisibilityFeature::GridVisible
	}
}

void MapView::setOverprintingSimulationEnabled(bool enabled)
{
	if (overprinting_simulation_enabled != enabled)
	{
		overprinting_simulation_enabled = enabled;
		emit visibilityChanged(VisibilityFeature::OverprintingEnabled, enabled);	// paramter 'enabled' is not used for VisibilityFeature::OverprintingEnabled
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
		auto templ = map->getTemplate(i);
		auto visibility = getTemplateVisibility(templ);
		if (visibility.hasAlpha() || templ->hasAlpha())
			return true;
	}
	
	return false;
}

bool MapView::applyVisibilitySet(int new_visibility_set)
{
	Q_ASSERT(new_visibility_set < getNumberOfVisibilitySets());
	Q_ASSERT(template_visibility_sets.getVisibility(new_visibility_set).template_visibilities.size() == template_visibility_sets.getCurrentVisibility().template_visibilities.size());
	
	if (new_visibility_set != getActiveVisibilityIndex())
	{
		auto number_of_templates = (int)template_visibility_sets.getVisibility(new_visibility_set).template_visibilities.size();
		for (int i = 0; i < number_of_templates; ++i)
		{
			auto new_template_visibility = template_visibility_sets.getTemplateVisibility(new_visibility_set, i);
			if (new_template_visibility != template_visibility_sets.getTemplateVisibility(getActiveVisibilityIndex(), i))
			{
				auto const visible = new_template_visibility.visible && new_template_visibility.opacity > 0;
				emit visibilityChanged(VisibilityFeature::TemplateVisible, visible, const_cast<Template*>(new_template_visibility.templ));
			}
		}

		auto new_map_visibility = template_visibility_sets.getVisibility(new_visibility_set).map_visibility;
		if (template_visibility_sets.getCurrentVisibility().map_visibility != new_map_visibility)
		{
			emit visibilityChanged(VisibilityFeature::MapVisible, new_map_visibility.visible && new_map_visibility.opacity > 0, nullptr);	// actual visibility is not used for VisibilityFeature::MapVisible
		}
		
		if (template_visibility_sets.getCurrentVisibility().all_templates_hidden != template_visibility_sets.getVisibility(new_visibility_set).all_templates_hidden)
			return true;
	}
	
	return false;
}

void MapView::setVisibilitySet(int new_visibility_set)
{
	auto emit_change = applyVisibilitySet(new_visibility_set);
	template_visibility_sets.setActiveVisibilityIndex(new_visibility_set);
	if (emit_change)
		emit visibilityChanged(VisibilityFeature::AllTemplatesHidden, true);	// 'true' is not used for VisibilityFeature::AllTemplatesHidden
}

void MapView::addVisibilitySet()
{
	template_visibility_sets.duplicateVisibility();
}

void MapView::deleteVisibilitySet()
{
	auto new_visibility_set = getActiveVisibilityIndex();
	if (new_visibility_set < getNumberOfVisibilitySets() - 1)
		++new_visibility_set;	// deleting not the last set => the following set will be applied
	else
		--new_visibility_set;	// deleting the last set => the previous set will be applied
	auto emit_change = applyVisibilitySet(new_visibility_set);
	template_visibility_sets.deleteVisibility();
	if (emit_change)
		emit visibilityChanged(VisibilityFeature::AllTemplatesHidden, true);	// 'true' is not used for VisibilityFeature::AllTemplatesHidden
}

// ### TemplateVisibilitySet ###

TemplateVisibilitySet::TemplateVisibilitySet()
{
	map_visibility = {1.0f, true};
}

TemplateVisibilitySets::TemplateVisibilitySets()
{
	template_visibility_sets.push_back(TemplateVisibilitySet());
	active_visibility_index = 0;
}

void TemplateVisibilitySets::duplicateVisibility()
{
	template_visibility_sets.insert(template_visibility_sets.begin() + active_visibility_index + 1, getCurrentVisibility());
}

void TemplateVisibilitySets::deleteVisibility()
{
	Q_ASSERT(getNumberOfVisibilitySets() > 1);
	Q_ASSERT(active_visibility_index < getNumberOfVisibilitySets());
	
	template_visibility_sets.erase(template_visibility_sets.begin() + active_visibility_index);
	if (active_visibility_index >= getNumberOfVisibilitySets())
		--active_visibility_index;
}

void TemplateVisibilitySets::deleteTemplate(const Template* templ)
{
	for (auto& template_set : template_visibility_sets)
	{
		template_set.template_visibilities.erase(template_set.findVisibility(templ));
	}
}

void TemplateVisibilitySets::addTemplate(const Template *templ, TemplateVisibility vis)
{
	for (auto& template_set : template_visibility_sets)
	{
		// another check is needed to differentiate between adding a template during loading vs. adding a template by the user
		if (!template_set.existsVisibility(templ))
			template_set.template_visibilities.emplace_back(templ, vis);
	}
}

TemplateVisibilityEntry TemplateVisibilitySets::getCurrentTemplateVisibility(const Template* templ) const
{
	return *(template_visibility_sets.at(active_visibility_index).findVisibility(templ));
}

TemplateVisibilityEntry TemplateVisibilitySets::getTemplateVisibility(int template_set, int index) const
{
	return template_visibility_sets.at(template_set).template_visibilities.at(index);
}

void TemplateVisibilitySets::setCurrentTemplateVisibility(const Template* templ, TemplateVisibility vis)
{
	template_visibility_sets.at(active_visibility_index).findVisibility(templ)->opacity = vis.opacity;
	template_visibility_sets.at(active_visibility_index).findVisibility(templ)->visible = vis.visible;
}

bool TemplateVisibilitySets::existsCurrentTemplateVisibility(const Template* templ) const
{
	return getCurrentVisibility().existsVisibility(templ);
}

TemplateVisibilitySet::TemplateVisibilityVector::const_iterator TemplateVisibilitySet::findVisibility(const Template* templ) const
{
	return std::find_if(begin(template_visibilities), end(template_visibilities), [templ](const TemplateVisibilityEntry& entry)
	{
		return entry.templ == templ;
	} );
}

TemplateVisibilitySet::TemplateVisibilityVector::iterator TemplateVisibilitySet::findVisibility(const Template* templ)
{
	return std::find_if(begin(template_visibilities), end(template_visibilities), [templ](const TemplateVisibilityEntry& entry)
	{
		return entry.templ == templ;
	} );
}

bool TemplateVisibilitySet::existsVisibility(const Template* templ) const
{
	return end(template_visibilities) != std::find_if(begin(template_visibilities), end(template_visibilities), [templ](const TemplateVisibilityEntry& entry)
	{
		return entry.templ == templ;
	} );
}

}  // namespace OpenOrienteering
