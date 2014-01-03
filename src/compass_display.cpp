/*
 *    Copyright 2013 Thomas Schöps
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

#include "compass_display.h"

#include <QPainter>

#include "compass.h"
#include "map_widget.h"
#include "util.h"


CompassDisplay::CompassDisplay(MapWidget* map_widget) : QObject(NULL)
{
	this->widget = map_widget;
	
	enabled = false;
	have_value = false;
	
	widget->setCompassDisplay(this);
}

CompassDisplay::~CompassDisplay()
{
	widget->setCompassDisplay(NULL);
}

void CompassDisplay::enable(bool enabled)
{
	if (enabled == this->enabled)
		return;
	
	if (enabled && ! this->enabled)
	{
		Compass::getInstance().startUsage();
		Compass::getInstance().connectToAzimuthChanges(this, SLOT(valueChanged(float)));
		last_update_time = QTime::currentTime();
		have_value = false;
	}
	else if (!enabled && this->enabled)
	{
		Compass::getInstance().stopUsage();
	}
	
	this->enabled = enabled;
	
	updateMapWidget();
}

void CompassDisplay::paint(QPainter* painter)
{
	if (!enabled)
		return;
	
	painter->setBrush(Qt::NoBrush);
	QRectF bounding_box = calcBoundingBox();
	QPointF midpoint = bounding_box.center();
	QPointF endpoint = QPointF(midpoint.x(), bounding_box.top());
	QLineF line(midpoint, endpoint);
	
	// Draw alignment cue
	if (have_value)
	{
		const float max_alignment_angle = 10;
		if (qAbs(value_azimuth) < max_alignment_angle)
		{
			float alignment_factor = (max_alignment_angle - qAbs(value_azimuth)) / max_alignment_angle;
			float radius = alignment_factor * 0.25f * bounding_box.height();
			painter->setPen(Qt::NoPen);
			painter->setBrush(Qt::green);
			painter->drawEllipse(line.pointAt(0.5f), radius, radius);
		}
	}
	
	// Draw up marker
	float line_width = Util::mmToPixelLogical(0.3f);
	
	painter->setPen(QPen(Qt::white, line_width));
	painter->drawLine(line);
	
	painter->setPen(QPen(Qt::black, line_width));
	painter->drawLine(midpoint + QPointF(line_width, 0), endpoint + QPointF(line_width, 0));
	painter->drawLine(midpoint + QPointF(-1 * line_width, 0), endpoint + QPointF(-1 * line_width, 0));
	
	// Draw needle
	if (have_value)
	{
		painter->setPen(QPen(Qt::red, line_width));
		line.setAngle(90 + value_azimuth);
		line.setLength(line.length() * 0.5f * (1 + value_calibration));
		painter->drawLine(line);
	}
}

void CompassDisplay::valueChanged(float azimuth_deg)
{
	const int update_interval = 200;
	QTime current_time = QTime::currentTime();
	if (qAbs(last_update_time.msecsTo(current_time)) < update_interval)
		return;
	last_update_time = current_time;
	
	value_azimuth = azimuth_deg;
	value_calibration = 1;
	have_value = true;
	updateMapWidget();
}

void CompassDisplay::updateMapWidget()
{
	widget->update(calcBoundingBox().toAlignedRect());
}

QRectF CompassDisplay::calcBoundingBox()
{
	float start = Util::mmToPixelLogical(2.0f);
	float width = Util::mmToPixelLogical(20.0f);
	return QRectF(start, start, width, width);
}
