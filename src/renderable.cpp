/*
 *    Copyright 2012 Thomas Schöps
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


#include "renderable.h"

#include "map_color.h"
#include "symbol_point.h"
#include "symbol_line.h"
#include "symbol_area.h"

Renderable::Renderable()
{
	clip_path = NULL;
}
Renderable::~Renderable()
{
}

// ### DotRenderable ###

DotRenderable::DotRenderable(PointSymbol* symbol, MapCoordF coord) : Renderable()
{
	this->symbol = symbol;
	
	double x = coord.getX();
	double y = coord.getY();
	double radius = (0.001 * symbol->getInnerRadius());
	extent = QRectF(x - radius, y - radius, 2 * radius, 2 * radius);
}
void DotRenderable::getRenderStates(RenderStates& out)
{
	PointSymbol* point = reinterpret_cast<PointSymbol*>(symbol);
	
	out.color_priority = point->getInnerColor()->priority;
	assert(out.color_priority < 3000);
	out.mode = RenderStates::BrushOnly;
	out.pen_width = 0;
	out.clip_path = clip_path;
}
void DotRenderable::render(QPainter& painter)
{
	/*if (forceMinSize && rect.width() * scale < 1.5f)
		painter.drawEllipse(rect.center(), 0.5f * (1/scale), 0.5f * (1/scale));
	else*/
		painter.drawEllipse(extent);
}

// ### CircleRenderable ###

CircleRenderable::CircleRenderable(PointSymbol* symbol, MapCoordF coord) : Renderable()
{
	this->symbol = symbol;
	
	double x = coord.getX();
	double y = coord.getY();
	line_width = 0.001f * symbol->getOuterWidth();
	double radius = (0.001 * symbol->getInnerRadius()) + 0.5f * line_width;
	rect = QRectF(x - radius, y - radius, 2 * radius, 2 * radius);
	extent = QRectF(rect.x() - 0.5*line_width, rect.y() - 0.5*line_width, rect.width() + line_width, rect.height() + line_width);
}
void CircleRenderable::getRenderStates(RenderStates& out)
{
	PointSymbol* point = reinterpret_cast<PointSymbol*>(symbol);
	
	out.color_priority = point->getOuterColor()->priority;
	assert(out.color_priority < 3000);
	out.mode = RenderStates::PenOnly;
	out.pen_width = line_width;
	out.clip_path = clip_path;
}
void CircleRenderable::render(QPainter& painter)
{
	/*if (forceMinSize && rect.width() * scale < 1.5f)
		painter.drawEllipse(rect.center(), 0.5f * (1/scale), 0.5f * (1/scale));
	else*/
		painter.drawEllipse(rect);
}

// ### LineRenderable ###

LineRenderable::LineRenderable(LineSymbol* symbol, const MapCoordVectorF& transformed_coords, const MapCoordVector& coords, bool closed) : Renderable()
{
	this->symbol = symbol;
	
	line_width = 0.001f * symbol->getLineWidth();
	
	switch (symbol->getCapStyle())
	{
		case LineSymbol::FlatCap:		cap_style = Qt::FlatCap;	break;
		case LineSymbol::RoundCap:		cap_style = Qt::RoundCap;	break;
		case LineSymbol::SquareCap:		cap_style = Qt::SquareCap;	break;
		case LineSymbol::PointedCap:	cap_style = Qt::FlatCap;	break;
	}
	switch (symbol->getJoinStyle())
	{
		case LineSymbol::BevelJoin:		join_style = Qt::BevelJoin;	break;
		case LineSymbol::MiterJoin:		join_style = Qt::MiterJoin;	break;
		case LineSymbol::RoundJoin:		join_style = Qt::RoundJoin;	break;
	}
	
	int size = (int)coords.size();
	assert(size >= 2);
	
	path.moveTo(transformed_coords[0].toQPointF());
	for (int i = 1; i < size; ++i)
	{
		if (coords[i-1].isCurveStart())
		{
			assert(i < size - 2);
			path.cubicTo(transformed_coords[i].toQPointF(), transformed_coords[i+1].toQPointF(), transformed_coords[i+2].toQPointF());
			i += 2;
		}
		else
			path.lineTo(transformed_coords[i].toQPointF());
	}
	
	if (closed)
		path.closeSubpath();
	
	// Get extent
	const QRectF rect = path.controlPointRect();
	float extra_border = line_width * 0.5f;
	if (symbol->getJoinStyle() == LineSymbol::MiterJoin)
		extra_border *= (2 * LineSymbol::miterLimit()) + 0.1f;
	else if (symbol->getCapStyle() == LineSymbol::SquareCap)
		extra_border *= 1.5f;
	extent = QRectF(rect.x() - extra_border, rect.y() - extra_border, rect.width() + 2*extra_border, rect.height() + 2*extra_border);
	assert(extent.right() < 999999);	// assert if bogus values are returned
}
void LineRenderable::getRenderStates(RenderStates& out)
{
	LineSymbol* line = reinterpret_cast<LineSymbol*>(symbol);
	
	out.color_priority = line->getColor()->priority;
	out.mode = RenderStates::PenOnly;
	out.pen_width = line_width;
	out.clip_path = clip_path;
}
void LineRenderable::render(QPainter& painter)
{
	QPen pen(painter.pen());
	pen.setCapStyle(cap_style);
	pen.setJoinStyle(join_style);
	if (join_style == Qt::MiterJoin)
		pen.setMiterLimit(LineSymbol::miterLimit());
	painter.setPen(pen);
	painter.drawPath(path);
	
	// DEBUG: show all control points
	/*QPen debugPen(QColor(Qt::red));
	painter.setPen(debugPen);
	for (int i = 0; i < path.elementCount(); ++i)
	{
		const QPainterPath::Element& e = path.elementAt(i);
		painter.drawEllipse(QPointF(e.x, e.y), 2, 2);
	}
	painter.setPen(pen);*/
}

// ### AreaRenderable ###

AreaRenderable::AreaRenderable(AreaSymbol* symbol, const MapCoordVectorF& transformed_coords, const MapCoordVector& coords) : Renderable()
{
	this->symbol = symbol;
	
	// Special case: first coord
	path.moveTo(transformed_coords[0].toQPointF());
	
	// Coords 1 to size - 1
	int size = (int)coords.size();
	for (int i = 1; i < size; ++i)
	{
		if (coords[i-1].isCurveStart())
		{
			assert(i < size - 2);
			path.cubicTo(transformed_coords[i].toQPointF(), transformed_coords[i+1].toQPointF(), transformed_coords[i+2].toQPointF());
			i += 2;
		}
		/*else if (coords[i].getFirstHolePoint())
		{
			path.closeSubpath();
			path.moveTo(transformed_coords[i].toQPointF());
		}*/
		else
			path.lineTo(transformed_coords[i].toQPointF());
	}
	
	// Close path
	path.closeSubpath();
	
	//if (disableHoles)
	//	path.setFillRule(Qt::WindingFill);
	
	// Get extent
	extent = path.controlPointRect();
	assert(extent.right() < 999999);	// assert if bogus values are returned
}
void AreaRenderable::getRenderStates(RenderStates& out)
{
	AreaSymbol* area = reinterpret_cast<AreaSymbol*>(symbol);
	if (area->getColor() != NULL)
	{
		out.color_priority = area->getColor()->priority;
		out.mode = RenderStates::BrushOnly;
		out.pen_width = 0;
		out.clip_path = clip_path;
	}
	else
		out.color_priority = MapColor::Reserved;
}
void AreaRenderable::render(QPainter& painter)
{
	painter.drawPath(path);
}
