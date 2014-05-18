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

#include "point_handles.h"

#include "../map_widget.h"
#include "../object.h"
#include "../object_text.h"
#include "../settings.h"

PointHandles::PointHandles(int scale_factor)
: scale_factor(scale_factor)
, handle_image(loadHandleImage(scale_factor))
{
	; // nothing
}

QRgb PointHandles::stateColor(PointHandleState state) const
{
	switch (state)
	{
	case NormalHandleState:    return qRgb(  0,   0, 255);
	case ActiveHandleState:    return qRgb(255, 150,   0);
	case SelectedHandleState:  return qRgb(255,   0,   0);
	case DisabledHandleState:  return qRgb(106, 106, 106);
	default:                   Q_ASSERT(false);
	                           return qRgb(255,   0,   0);
	}
}

void PointHandles::draw(QPainter* painter, MapWidget* widget, Object* object, int hover_point, bool draw_curve_handles, PointHandleState base_state) const
{
	if (object->getType() == Object::Point)
	{
		PointObject* point = reinterpret_cast<PointObject*>(object);
		draw(painter, widget->mapToViewport(point->getCoordF()), NormalHandle, (hover_point == 0) ? ActiveHandleState : base_state);
	}
	else if (object->getType() == Object::Text)
	{
		TextObject* text = reinterpret_cast<TextObject*>(object);
		std::vector<QPointF> text_handles(text->controlPoints());
		for (std::size_t i = 0; i < text_handles.size(); ++i)
			draw(painter, widget->mapToViewport(text_handles[i]), NormalHandle, (hover_point == (int)i) ? ActiveHandleState : base_state);
	}
	else if (object->getType() == Object::Path)
	{
		painter->setBrush(Qt::NoBrush); // for handle lines
		
		PathObject* path = reinterpret_cast<PathObject*>(object);
		
		int num_parts = path->getNumParts();
		for (int part_index = 0; part_index < num_parts; ++part_index)
		{
			PathObject::PathPart& part = path->getPart(part_index);
			bool have_curve = part.isClosed() && part.getNumCoords() > 3 && path->getCoordinate(part.end_index - 3).isCurveStart();
			PointHandleType handle_type = NormalHandle;
			
			for (int i = part.start_index; i <= part.end_index; ++i)
			{
				MapCoord coord = path->getCoordinate(i);
				if (coord.isClosePoint())
					continue;
				QPointF point = widget->mapToViewport(coord);
				bool is_active = hover_point == i;
				
				if (i == part.start_index && !part.isClosed()) // || (i > part.start_index && path->getCoordinate(i-1).isHolePoint()))
					handle_type = StartHandle;
				else if (i == part.end_index && !part.isClosed()) // || coord.isHolePoint())
					handle_type = EndHandle;
				else
					handle_type = coord.isDashPoint() ? DashHandle : NormalHandle;
				
				// Draw incoming curve handle
				QPointF curve_handle;
				if (draw_curve_handles && have_curve)
				{
					int curve_index = (i == part.start_index) ? (part.end_index - 1) : (i - 1);
					curve_handle = widget->mapToViewport(path->getCoordinate(curve_index));
					drawCurveHandleLine(painter, point, curve_handle, handle_type, is_active ? ActiveHandleState : base_state);
					draw(painter, curve_handle, CurveHandle, (is_active || hover_point == curve_index) ? ActiveHandleState : base_state);
					have_curve = false;
				}
				
				// Draw outgoing curve handle, first part
				if (draw_curve_handles && coord.isCurveStart())
				{
					curve_handle = widget->mapToViewport(path->getCoordinate(i+1));
					drawCurveHandleLine(painter, point, curve_handle,handle_type,  is_active ? ActiveHandleState : base_state);
				}
				
				// Draw point
				draw(painter, point, handle_type, is_active ? ActiveHandleState : base_state);
				
				// Draw outgoing curve handle, second part
				if (coord.isCurveStart())
				{
					if (draw_curve_handles)
					{
						draw(painter, curve_handle, CurveHandle, (is_active || hover_point == i + 1) ? ActiveHandleState : base_state);
						have_curve = true;
					}
					i += 2;
				}
			}
		}
	}
	else
		Q_ASSERT(false);
}

void PointHandles::draw(QPainter* painter, QPointF position, PointHandleType type, PointHandleState state) const
{
	int width = scale_factor * 11;
	int offset = (width - 1) / 2;
	painter->drawImage(qRound(position.x()) - offset, qRound(position.y()) - offset, image(), (int)type * width, (int)state * width, width, width);
}

void PointHandles::drawCurveHandleLine(QPainter* painter, QPointF anchor_point, QPointF curve_handle, PointHandleType type, PointHandleState state) const
{
	const float handle_radius = 3 * scale_factor;
	if (scale_factor > 1)
		painter->setPen(QPen(QBrush(stateColor(state)), scale_factor));
	else
		painter->setPen(stateColor(state));
	
	QPointF to_handle = curve_handle - anchor_point;
	float to_handle_len = to_handle.x()*to_handle.x() + to_handle.y()*to_handle.y();
	if (to_handle_len > 0.00001f)
	{
		to_handle_len = sqrt(to_handle_len);
		if (type == StartHandle)
			anchor_point += scale_factor * 5 / qMax(qAbs(to_handle.x()), qAbs(to_handle.y())) * to_handle;
		else if (type == DashHandle)
			anchor_point += scale_factor * to_handle * (3 / to_handle_len);
		else //if (type == NormalHandle)
			anchor_point += scale_factor * 3 / qMax(qAbs(to_handle.x()), qAbs(to_handle.y())) * to_handle;
		
		curve_handle -= to_handle * (handle_radius / to_handle_len);
	}
	
	painter->drawLine(anchor_point, curve_handle);
}

// static
const QImage PointHandles::loadHandleImage(int factor)
{
	static const QStringList image_names = (
	  QStringList() << "" // not used
	                << ":/images/point-handles.png"
	                << ":/images/point-handles-2x.png"
	                << "" // not used
					<< ":/images/point-handles-4x.png"
	);
	
	Q_ASSERT(factor < image_names.size());
	
	// NOTE: If this fails, check MapEditorTool::newScaleFactor()!
	Q_ASSERT(!image_names[factor].isEmpty());
	
	static int shared_image_factor = factor;
	static QImage shared_image = QImage(image_names[shared_image_factor]);
	if (shared_image_factor != factor)
	{
		shared_image_factor = factor;
		shared_image = QImage(image_names[shared_image_factor]);
	}
	
	return shared_image;
}

