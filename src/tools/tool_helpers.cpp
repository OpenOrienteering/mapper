/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2013-2017 Kai Pastor
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


#include "tool_helpers.h"

#include <cmath>
#include <limits>
#include <utility>
#include <vector>

#include <Qt>
#include <QtGlobal>
#include <QtMath>
#include <QCoreApplication>
#include <QFontMetrics>
#include <QLatin1Char>
#include <QLineF>
#include <QLocale>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPoint>
#include <QPointF>
#include <QRectF>
#include <QSettings>
#include <QString>
#include <QVariant>
#include <QWidget>

#include "settings.h"
#include "core/map.h"
#include "core/map_grid.h"
#include "core/map_part.h"
#include "core/map_view.h"
#include "core/objects/object.h"
#include "gui/map/map_widget.h"
#include "tools/tool.h"
#include "util/util.h"


namespace OpenOrienteering {

// ### ConstrainAngleToolHelper ###

ConstrainAngleToolHelper::ConstrainAngleToolHelper()
{
	connect(&Settings::getInstance(), &Settings::settingsChanged, this, &ConstrainAngleToolHelper::settingsChanged);
}

ConstrainAngleToolHelper::~ConstrainAngleToolHelper()
{
	// nothing, not inlined!
}


void ConstrainAngleToolHelper::setCenter(const MapCoordF& center)
{
	if (this->center != center)
	{
		this->center = center;
		emit displayChanged();
	}
}

void ConstrainAngleToolHelper::addAngle(qreal angle)
{
	angle = std::fmod(angle, 2*M_PI);
	if (angle < 0.0)
		angle += 2*M_PI;
	
	if (qFuzzyCompare(angle, 2*M_PI))
		angle = 0;
	
	// Fuzzy compare for existing angle as otherwise very similar angles will be added
	for (auto value : angles)
	{
		const double min_delta = 0.25 * 2*M_PI / 360;	// minimum 1/4 degree difference
		if (qAbs(angle - value) < min_delta || qAbs(angle - value) > 2*M_PI - min_delta)
			return;
	}
	
	if (Q_LIKELY(angle >= 0.0))
	{
		angles.insert(angle);
		have_default_angles_only = false;
	}
	else
	{
		Q_UNREACHABLE();
	}
}

void ConstrainAngleToolHelper::addAngles(qreal base, qreal stepping)
{
	for (double angle = base; angle < base + 2*M_PI; angle += stepping)
		addAngle(angle);
}

void ConstrainAngleToolHelper::addAnglesDeg(qreal base, qreal stepping)
{
	addAngles(qDegreesToRadians(base), qDegreesToRadians(stepping));
}

void ConstrainAngleToolHelper::addDefaultAnglesDeg(qreal base)
{
	addAnglesDeg(base, Settings::getInstance().getSettingCached(Settings::MapEditor_FixedAngleStepping).toDouble());
	have_default_angles_only = true;
	default_angles_base = base;
}

void ConstrainAngleToolHelper::clearAngles()
{
	angles.clear();
	have_default_angles_only = false;
	if (active_angle > -1)
	{
		active_angle = -1;
		emitActiveAngleChanged();
	}
}

double ConstrainAngleToolHelper::getConstrainedCursorPos(const QPoint& in_pos, QPointF& out_pos, MapWidget* widget)
{
	MapCoordF in_pos_map = widget->viewportToMapF(in_pos);
	MapCoordF out_pos_map;
	double angle = getConstrainedCursorPosMap(in_pos_map, out_pos_map);
	out_pos = widget->mapToViewport(out_pos_map);
	return angle;
}

double ConstrainAngleToolHelper::getConstrainedCursorPosMap(const MapCoordF& in_pos, MapCoordF& out_pos)
{
	MapCoordF to_cursor = in_pos - center;
	double in_angle = -to_cursor.angle();
	if (!active)
	{
		out_pos = in_pos;
		return in_angle;
	}
	
	double lower_angle = in_angle, lower_angle_delta = 999;
	double higher_angle = in_angle, higher_angle_delta = -999;
	for (auto value : angles)
	{
		double delta = in_angle - value;
		if (delta < -M_PI)
			delta = in_angle - (value - 2*M_PI);
		else if (delta > M_PI)
			delta = (in_angle - 2*M_PI) - value;
		
		if (delta > 0)
		{
			if (delta < lower_angle_delta)
			{
				lower_angle_delta = delta;
				lower_angle = value;
			}
		}
		else
		{
			if (delta > higher_angle_delta)
			{
				higher_angle_delta = delta;
				higher_angle = value;
			}
		}
	}
	
	double new_active_angle;
	if (lower_angle_delta < -higher_angle_delta)
		new_active_angle = lower_angle;
	else
		new_active_angle = higher_angle;
	if (new_active_angle != active_angle)
	{
		emitActiveAngleChanged();
		active_angle = new_active_angle;
	}
	
	MapCoordF unit_direction = MapCoordF(cos(active_angle), -sin(active_angle));
	to_cursor = MapCoordF::dotProduct(unit_direction, to_cursor) * unit_direction;
	out_pos = center + to_cursor;
	
	return active_angle;
}

double ConstrainAngleToolHelper::getConstrainedCursorPositions(const MapCoordF& in_pos_map, MapCoordF& out_pos_map, QPointF& out_pos, MapWidget* widget)
{
	double angle = getConstrainedCursorPosMap(in_pos_map, out_pos_map);
	out_pos = widget->mapToViewport(out_pos_map);
	return angle;
}

void ConstrainAngleToolHelper::setActive(bool active, const MapCoordF& center)
{
	if (active)
	{
		if (!this->active || (this->center != center))
			emit displayChanged();
		this->center = center;
	}
	else
	{
		if (active_angle > -1)
		{
			active_angle = -1;
			emitActiveAngleChanged();
		}
		else
			emit displayChanged();
	}
	this->active = active;
}

void ConstrainAngleToolHelper::setActive(bool active)
{
	if (active)
	{
		if (!this->active)
			emit displayChanged();
	}
	else
	{
		if (active_angle > -1)
		{
			active_angle = -1;
			emitActiveAngleChanged();
		}
		else
			emit displayChanged();
	}
	this->active = active;
}

void ConstrainAngleToolHelper::draw(QPainter* painter, MapWidget* widget)
{
	constexpr auto reduced_opacity = qreal(0.5);
	
	if (!active) return;
	QPointF center_point = widget->mapToViewport(center);
	painter->setOpacity(reduced_opacity);
	painter->setPen(MapEditorTool::inactive_color);
	painter->setBrush(Qt::NoBrush);
	for (auto value : angles)
	{
		if (value == active_angle)
		{
			painter->setPen(MapEditorTool::active_color);
			painter->setOpacity(1);
		}
		
		QPointF outer_point = center_point + getDisplayRadius() * QPointF(cos(value), -sin(value));
		painter->drawLine(center_point, outer_point);
		
		if (value == active_angle)
		{
			painter->setPen(MapEditorTool::inactive_color);
			painter->setOpacity(reduced_opacity);
		}
	}
	painter->setOpacity(1);
}

void ConstrainAngleToolHelper::includeDirtyRect(QRectF& rect)
{
	if (!active) return;
	rectIncludeSafe(rect, center);
}

void ConstrainAngleToolHelper::settingsChanged()
{
	if (have_default_angles_only)
	{
		clearAngles();
		addDefaultAnglesDeg(default_angles_base);
	}
}



// ### SnappingToolHelper ###

SnappingToolHelper::SnappingToolHelper(MapEditorTool* tool, SnapObjects filter)
: filter(filter)
, map(tool->map())
{
	point_handles.setScaleFactor(tool->scaleFactor());
}

SnappingToolHelper::~SnappingToolHelper()
{
	// nothing, not inlined
}


void SnappingToolHelper::setFilter(SnapObjects filter)
{
	this->filter = filter;
	if (!(filter & snapped_type))
		snapped_type = NoSnapping;
}

SnappingToolHelper::SnapObjects SnappingToolHelper::getFilter() const
{
	return filter;
}

MapCoord SnappingToolHelper::snapToObject(const MapCoordF& position, MapWidget* widget, SnappingToolHelperSnapInfo* info, Object* exclude_object)
{
	auto snap_distance = widget->getMapView()->pixelToLength(Settings::getInstance().getMapEditorSnapDistancePx() / 1000);
	auto closest_distance_sq = float(snap_distance * snap_distance); /// \todo Change to qreal when Path::calcClosestPointOnPath accepts that.
	auto result_position = MapCoord { position };
	SnappingToolHelperSnapInfo result_info;
	result_info.type = NoSnapping;
	result_info.object = nullptr;
	result_info.coord_index = std::numeric_limits<decltype(result_info.coord_index)>::max();
	result_info.path_coord.pos = MapCoordF(0, 0);
	result_info.path_coord.index = std::numeric_limits<decltype(result_info.path_coord.index)>::max();
	result_info.path_coord.clen = -1;
	result_info.path_coord.param = -1;
	
	if (filter & (ObjectCorners | ObjectPaths))
	{
		// Find map objects at the given position
		SelectionInfoVector objects;
		map->findAllObjectsAt(position, snap_distance, true, false, false, true, objects);
		
		// Find closest snap spot from map objects
		for (const auto& info : objects)
		{
			Object* object = info.second;
			if (object == exclude_object)
				continue;
			
			auto distance_sq = float(-1); /// \todo Change to qreal when Path::calcClosestPointOnPath accepts that.
			if (object->getType() == Object::Point && filter & ObjectCorners)
			{
				PointObject* point = object->asPoint();
				distance_sq = point->getCoordF().distanceSquaredTo(position);
				if (distance_sq < closest_distance_sq)
				{
					closest_distance_sq = distance_sq;
					result_position = point->getCoord();
					result_info.type = ObjectCorners;
					result_info.object = object;
					result_info.coord_index = 0;
				}
			}
			else if (object->getType() == Object::Path)
			{
				const PathObject* path = object->asPath();
				if (filter & ObjectPaths)
				{
					PathCoord path_coord;
					path->calcClosestPointOnPath(position, distance_sq, path_coord);
					if (distance_sq < closest_distance_sq)
					{
						closest_distance_sq = distance_sq;
						result_position = MapCoord(path_coord.pos);
						result_info.object = object;
						if (path_coord.param == 0)
						{
							result_info.type = ObjectCorners;
							result_info.coord_index = path_coord.index;
						}
						else
						{
							result_info.type = ObjectPaths;
							result_info.coord_index = std::numeric_limits<decltype(result_info.coord_index)>::max();
							result_info.path_coord = path_coord;
						}
					}
				}
				else
				{
					MapCoordVector::size_type index;
					path->calcClosestCoordinate(position, distance_sq, index);
					if (distance_sq < closest_distance_sq)
					{
						closest_distance_sq = distance_sq;
						result_position = path->getCoordinate(index);
						result_info.type = ObjectCorners;
						result_info.object = object;
						result_info.coord_index = index;
					}
				}
			}
			else if (object->getType() == Object::Text)
			{
				// No snapping to texts
				continue;
			}
		}
	}
	
	// Find closest grid snap position
	if ((filter & GridCorners) && widget->getMapView()->isGridVisible() &&
		map->getGrid().isSnappingEnabled() && map->getGrid().getDisplayMode() == MapGrid::AllLines)
	{
		MapCoordF closest_grid_point = map->getGrid().getClosestPointOnGrid(position, map);
		auto distance_sq = float(closest_grid_point.distanceSquaredTo(position)); /// \todo Change to qreal when Path::calcClosestPointOnPath accepts that.
		if (distance_sq < closest_distance_sq)
		{
			closest_distance_sq = distance_sq;
			result_position = MapCoord(closest_grid_point);
			result_info.type = GridCorners;
			result_info.object = nullptr;
			result_info.coord_index = std::numeric_limits<decltype(result_info.coord_index)>::max();
		}
	}
	
	// Return
	if (snap_mark != result_position || snapped_type != result_info.type)
	{
		snap_mark = result_position;
		snapped_type = result_info.type;
		emit displayChanged();
	}
	
	if (info)
		*info = result_info;
	return result_position;
}

bool SnappingToolHelper::snapToDirection(const MapCoordF& position, MapWidget* widget, ConstrainAngleToolHelper* angle_tool, MapCoord* out_snap_position)
{
	// As getting a direction from the map grid is not supported, remove grid from filter
	int filter_grid = filter & GridCorners;
	filter = SnapObjects(filter & ~filter_grid);
	
	// Snap to position
	SnappingToolHelperSnapInfo info;
	MapCoord snap_position = snapToObject(position, widget, &info);
	if (out_snap_position)
		*out_snap_position = snap_position;
	
	// Add grid to filter again, if it was there originally
	filter = SnapObjects(filter | filter_grid);
	
	// Get direction from result
	switch (info.type)
	{
	case ObjectCorners:
		if (info.object->getType() == Object::Point)
		{
			const PointObject* point = info.object->asPoint();
			angle_tool->clearAngles();
			angle_tool->addAngles(point->getRotation() - M_PI/2, M_PI/2);
			return true;
		}
		else if (info.object->getType() == Object::Path)
		{
			const PathObject* path = info.object->asPath();
			angle_tool->clearAngles();
			bool ok;
			// Forward tangent
			MapCoordF tangent = path->findPartForIndex(info.coord_index)->calculateTangent(info.coord_index, false, ok);
			if (ok)
				angle_tool->addAngles(-tangent.angle(), M_PI/2);
			// Backward tangent
			tangent = path->findPartForIndex(info.coord_index)->calculateTangent(info.coord_index, true, ok);
			if (ok)
				angle_tool->addAngles(-tangent.angle(), M_PI/2);
			return true;
		}
		return false;
		
	case ObjectPaths:
		{
			const PathObject* path = info.object->asPath();
			angle_tool->clearAngles();
			auto part   = path->findPartForIndex(info.path_coord.index);
			auto split  = SplitPathCoord::at(part->path_coords, info.path_coord.clen);
			auto right = split.tangentVector().perpRight();
			angle_tool->addAngles(-right.angle(), M_PI/2);
		}
		return true;
		
	default:
		return false;
	}
}

void SnappingToolHelper::draw(QPainter* painter, MapWidget* widget)
{
	auto handle_type = PointHandles::EndHandle;
	switch (snapped_type)
	{
	case NoSnapping:
		break;
		
	case ObjectPaths:
		handle_type = PointHandles::NormalHandle;
		// fall through
	default:
		point_handles.draw(painter, widget->mapToViewport(snap_mark), handle_type, PointHandles::NormalHandleState);
	}
}

void SnappingToolHelper::includeDirtyRect(QRectF& rect)
{
	if (snapped_type != NoSnapping)
		rectIncludeSafe(rect, QPointF(snap_mark));
}



// ### FollowPathToolHelper ###

FollowPathToolHelper::FollowPathToolHelper()
{
	// nothing else
}

void FollowPathToolHelper::startFollowingFromCoord(const PathObject* path, MapCoordVector::size_type coord_index)
{
	path->update();
	PathCoord coord = path->findPathCoordForIndex(coord_index);
	startFollowingFromPathCoord(path, coord);
}

void FollowPathToolHelper::startFollowingFromPathCoord(const PathObject* path, const PathCoord& coord)
{
	path->update();
	this->path = path;
	
	start_clen = coord.clen;
	end_clen   = start_clen;
	part_index = path->findPartIndexForIndex(coord.index);
	drag_forward = true;
}

std::unique_ptr<PathObject> FollowPathToolHelper::updateFollowing(const PathCoord& end_coord)
{
	std::unique_ptr<PathObject> result;
	
	if (path && path->findPartIndexForIndex(end_coord.index) == part_index)
	{
		// Update end_clen
		auto new_end_clen = end_coord.clen;
		const auto& part = path->parts()[part_index];
		if (part.isClosed())
		{
			// Positive length to add to end_clen to get to new_end_clen with wrapping
			auto path_length = qreal(part.length());
			auto half_path_length = path_length / 2;
			auto forward_diff = fmod_pos(new_end_clen - end_clen, path_length);
			auto delta_forward = forward_diff >= 0 && forward_diff < half_path_length;
			
			if (delta_forward
			    && !drag_forward
			    && fmod_pos(end_clen - start_clen, path_length) > half_path_length
			    && fmod_pos(new_end_clen - start_clen, path_length) <= half_path_length )
			{
				drag_forward = true;
			}
			else if (!delta_forward
			         && drag_forward
			         && fmod_pos(end_clen - start_clen, path_length) <= half_path_length
			         && fmod_pos(new_end_clen - start_clen, path_length) > half_path_length)
			{
				drag_forward = false;
			}
		}
		else
		{
			drag_forward = new_end_clen >= start_clen;
		}
		
		end_clen = new_end_clen;
		if (end_clen != start_clen)
		{
			// Create output path
			result.reset(new PathObject { part });
			if (drag_forward)
			{
				result->changePathBounds(0, start_clen, end_clen);
			}
			else
			{
				result->changePathBounds(0, end_clen, start_clen);
				result->reverse();
			}
		}
	}
	
	return result;
}



// ### AzimuthInfoHelper ###

namespace
{
	const QString show_azimuth_key = QStringLiteral("MapEditor/show_azimuth_info");  // clazy:exclude=non-pod-global-static
	
}


AzimuthInfoHelper::AzimuthInfoHelper(const QWidget* widget, QColor color)
: text_color(std::move(color))
, text_font(widget->font())
, azimuth_template(QCoreApplication::translate("OpenOrienteering::UnitOfMeasurement", "%1°", "degree"))
, distance_template(QCoreApplication::translate("OpenOrienteering::UnitOfMeasurement", "%1 m", "meter"))
{
	auto size = text_font.pixelSize();
	if (size >= 0)
		text_font.setPixelSize(size*2);
	else
		text_font.setPointSizeF(text_font.pointSizeF()*2);
	
	auto metrics = QFontMetrics{text_font};
	line_0_offset = -metrics.leading() / 2 - metrics.descent();
	line_1_offset = line_0_offset + metrics.lineSpacing();
	
	// Approximation of the length needed to draw "359.9°" or "1,200 m".
	auto display_radius = text_offset + metrics.width(QLatin1Char('5')) * 7;
	display_rect = QRectF(-display_radius, -display_radius, 2*display_radius, 2*display_radius);
	
	active = QSettings().value(show_azimuth_key).toBool();
}


void AzimuthInfoHelper::setActive(bool active)
{
	this->active = active;
	QSettings().setValue(show_azimuth_key, active);
}


QRectF AzimuthInfoHelper::dirtyRect(MapWidget* widget, const MapCoordF& pos_map) const
{
	auto map_rect = QRectF{widget->viewportToMapF(display_rect.topLeft()),
	                       widget->viewportToMapF(display_rect.bottomRight())};
	return map_rect.translated(pos_map - map_rect.center());
}


void AzimuthInfoHelper::draw(QPainter* painter, const MapWidget* widget, const Map* map, const MapCoordF& start_pos, const MapCoordF& end_pos)
{
	QLineF drag_vector(start_pos, end_pos);
	const auto distance = drag_vector.length();
	if (qIsNull(distance))
		return;
	
	QLocale locale;
	auto angle = qreal(450) - drag_vector.angle();
	if (angle >= 360)
		angle -= 360;
	auto azimuth_string = azimuth_template.arg(locale.toString(angle, 'f', 1));
	auto distance_string = distance_template.arg(locale.toString(0.001 * map->getScaleDenominator() * distance, 'f', 0));

	// rotate the text for better legibility
	QPainterPath line_0, line_1;
	line_0.addText(0, line_0_offset, text_font, azimuth_string);
	line_1.addText(0, line_1_offset, text_font, distance_string);

	auto text_angle = std::fmod(angle + qRadiansToDegrees(widget->getMapView()->getRotation()) + qreal(360 + 270), 360);
	auto offset = qreal(text_offset);
	if (text_angle > 90 && text_angle < 270)
	{
		// westwards
		text_angle -= 180;
	}
	else
	{
		// eastwards
		auto line_0_width = line_0.controlPointRect().width();
		auto line_1_width = line_1.controlPointRect().width();
		auto delta_width = line_1_width - line_0_width;
		if (delta_width > 0)
		{
			line_0.translate(delta_width, 0);
			offset = -line_1_width - text_offset;
		}
		else
		{
			line_1.translate(-delta_width, 0);
			offset = -line_0_width - text_offset;
		}
	}

	painter->save();

	painter->translate(widget->mapToViewport(end_pos));
	painter->rotate(text_angle);
	painter->translate(offset, 0);

	// give the numbers white outline
	painter->setPen(QPen(Qt::white, 3));
	painter->drawPath(line_0);
	painter->drawPath(line_1);

	painter->setPen(Qt::NoPen);
	painter->setBrush(text_color);
	painter->drawPath(line_0);
	painter->drawPath(line_1);

	painter->restore();
}


}  // namespace OpenOrienteering
