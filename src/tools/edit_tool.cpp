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

#include "edit_tool.h"

#include <cstddef>
#include <limits>
#include <map>
#include <memory>
#include <unordered_set>

#include <QtGlobal>
#include <QtMath>
#include <QCursor>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPixmap>
#include <QString>

#include "core/map.h"
#include "core/virtual_path.h"
#include "core/objects/object.h"
#include "core/objects/text_object.h"
#include "gui/map/map_widget.h"
#include "tools/object_selector.h"
#include "tools/tool_helpers.h"
#include "undo/object_undo.h"
#include "util/util.h"


namespace OpenOrienteering {

EditTool::EditTool(MapEditorController* editor, MapEditorTool::Type type, QAction* tool_action)
 : MapEditorToolBase { QCursor(QPixmap(QString::fromLatin1(":/images/cursor-hollow.png")), 1, 1), type, editor, tool_action }
 , object_selector { new ObjectSelector(map()) }
{
	; // nothing
}


EditTool::~EditTool() = default;



void EditTool::deleteSelectedObjects()
{
	map()->deleteSelectedObjects();
	updateStatusText();
}


void EditTool::createReplaceUndoStep(Object* object)
{
	auto undo_step = new ReplaceObjectsUndoStep(map());
	auto undo_duplicate = object->duplicate();
	undo_duplicate->setMap(map());
	undo_step->addObject(object, undo_duplicate);
	map()->push(undo_step);
	
	map()->setObjectsDirty();
}


bool EditTool::pointOverRectangle(const QPointF& point, const QRectF& rect) const
{
	auto click_tolerance = clickTolerance();
	if (point.x() < rect.left() - click_tolerance) return false;
	if (point.y() < rect.top() - click_tolerance) return false;
	if (point.x() > rect.right() + click_tolerance) return false;
	if (point.y() > rect.bottom() + click_tolerance) return false;
	if (point.x() > rect.left() + click_tolerance &&
		point.y() > rect.top() + click_tolerance &&
		point.x() < rect.right() - click_tolerance &&
		point.y() < rect.bottom() - click_tolerance) return false;
	return true;
}


MapCoordF EditTool::closestPointOnRect(MapCoordF point, const QRectF& rect)
{
	if (point.x() < rect.left()) point.setX(rect.left());
	if (point.y() < rect.top()) point.setY(rect.top());
	if (point.x() > rect.right()) point.setX(rect.right());
	if (point.y() > rect.bottom()) point.setY(rect.bottom());
	if (rect.height() > 0 && rect.width() > 0)
	{
		if ((point.x() - rect.left()) / rect.width() > (point.y() - rect.top()) / rect.height())
		{
			if ((point.x() - rect.left()) / rect.width() > (rect.bottom() - point.y()) / rect.height())
				point.setX(rect.right());
			else
				point.setY(rect.top());
		}
		else
		{
			if ((point.x() - rect.left()) / rect.width() > (rect.bottom() - point.y()) / rect.height())
				point.setY(rect.bottom());
			else
				point.setX(rect.left());
		}
	}
	return point;
}


void EditTool::setupAngleHelperFromEditedObjects()
{
	constexpr auto max_num_primary_directions = std::size_t(5);
	constexpr auto angle_window = (2 * M_PI) * 2 / 360.0;
	// Amount of all path length which has to be covered by an angle
	// to be classified as "primary angle"
	constexpr auto path_angle_threshold = qreal(1 / 5.0);
	
	angle_helper->clearAngles();
	
	std::unordered_set<qreal> primary_directions;
	for (const Object* object : editedObjects())
	{
		if (object->getType() == Object::Point)
		{
			primary_directions.insert(fmod_pos(object->asPoint()->getRotation(), M_PI / 2));
		}
		else if (object->getType() == Object::Text)
		{
			primary_directions.insert(fmod_pos(object->asText()->getRotation(), M_PI / 2));
		}
		else if (object->getType() == Object::Path)
		{
			const auto* path = object->asPath();
			// Maps angles to the path distance covered by them
			std::map<qreal, qreal> path_directions;
			
			// Collect segment directions, only looking at the first part
			auto& part = path->parts().front();
			auto path_length = part.path_coords.back().clen;
			for (auto c = part.first_index; c < part.last_index; c = part.nextCoordIndex(c))
			{
				if (!path->getCoordinate(c).isCurveStart())
				{
					auto segment = MapCoordF(path->getCoordinate(c + 1) - path->getCoordinate(c));
					auto angle = fmod_pos(-segment.angle(), M_PI / 2);
					auto length = segment.length();
					
					auto angle_it = path_directions.find(angle);
					if (angle_it != path_directions.end())
						angle_it->second += length;
					else
						path_directions.insert({angle, length});
				}
			}
			
			// Determine primary directions by moving a window over the collected angles
			// and determining maxima.
			// The iterators are the next angle which crosses the respective window border.
			auto angle_start = -angle_window;
			auto start_it = path_directions.begin();
			auto angle_end = qreal(0.0);
			auto end_it = path_directions.begin();
			
			auto length_increasing = true;
			auto cur_length = qreal(0.0);
			while (start_it != path_directions.end())
			{
				auto start_dist = start_it->first - angle_start;
				auto end_dist = (end_it == path_directions.end()) ?
					std::numeric_limits<qreal>::max() :
					(end_it->first - angle_end);
				if (start_dist > end_dist)
				{
					// A new angle enters the window (at the end)
					cur_length += end_it->second;
					length_increasing = true;
					++end_it;
					
					angle_start += end_dist;
					angle_end += end_dist;
				}
				else // if (start_dist <= end_dist)
				{
					// An angle leaves the window (at the start)
					// Check if we had a significant maximum.
					if (length_increasing &&
						cur_length / path_length >= path_angle_threshold)
					{
						// Find the average angle and insert it
						auto angle = qreal(0.0);
						auto total_weight = qreal(0.0);
						for (auto angle_it = start_it; angle_it != end_it; ++angle_it)
						{
							angle += angle_it->first * angle_it->second;
							total_weight += angle_it->second;
						}
						primary_directions.insert(angle / total_weight);
					}
					
					cur_length -= start_it->second;
					length_increasing = false;
					++start_it;
					
					angle_start += start_dist;
					angle_end += start_dist;
				}
			}
		}
		
		if (primary_directions.size() > max_num_primary_directions)
			break;
	}
	
	if (primary_directions.size() > max_num_primary_directions ||
		primary_directions.empty())
	{
		angle_helper->addDefaultAnglesDeg(0);
	}
	else
	{
		// Add base angles
		angle_helper->addAngles(0, M_PI / 2);
		
		// Add object angles
		for (auto angle : primary_directions)
			angle_helper->addAngles(angle, M_PI / 2);
	}
}


void EditTool::drawBoundingBox(QPainter* painter, MapWidget* widget, const QRectF& bounding_box, const QRgb& color)
{
	QPen pen(color);
	pen.setStyle(Qt::DashLine);
	if (scaleFactor() > 1)
		pen.setWidth(scaleFactor());
	painter->setPen(pen);
	painter->setBrush(Qt::NoBrush);
	painter->drawRect(widget->mapToViewport(bounding_box));
}


void EditTool::drawBoundingPath(QPainter* painter, MapWidget* widget, const std::vector<QPointF>& bounding_path, const QRgb& color)
{
	Q_ASSERT(!bounding_path.empty());
	
	QPen pen(color);
	pen.setStyle(Qt::DashLine);
	if (scaleFactor() > 1)
		pen.setWidth(scaleFactor());
	painter->setPen(pen);
	painter->setBrush(Qt::NoBrush);
	
	QPainterPath painter_path;
	painter_path.moveTo(widget->mapToViewport(bounding_path[0]));
	for (std::size_t i = 1; i < bounding_path.size(); ++i)
		painter_path.lineTo(widget->mapToViewport(bounding_path[i]));
	painter_path.closeSubpath();
	painter->drawPath(painter_path);
}


}  // namespace OpenOrienteering
