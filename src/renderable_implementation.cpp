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

#include "renderable_implementation.h"

#include <qmath.h>

#include "map.h"
#include "map_color.h"
#include "object.h"
#include "object_text.h"
#include "symbol_point.h"
#include "symbol_line.h"
#include "symbol_area.h"
#include "symbol_text.h"
#include "util.h"

// ### DotRenderable ###

DotRenderable::DotRenderable(PointSymbol* symbol, MapCoordF coord) : Renderable()
{
	color_priority = symbol->getInnerColor()->priority;
	double x = coord.getX();
	double y = coord.getY();
	double radius = (0.001 * symbol->getInnerRadius());
	extent = QRectF(x - radius, y - radius, 2 * radius, 2 * radius);
}
DotRenderable::DotRenderable(const DotRenderable& other) : Renderable(other)
{
}
void DotRenderable::getRenderStates(RenderStates& out) const
{
	out.color_priority = color_priority;
	assert(out.color_priority < 3000);
	out.mode = RenderStates::BrushOnly;
	out.pen_width = 0;
	out.clip_path = clip_path;
}
void DotRenderable::render(QPainter& painter, bool force_min_size, float scaling)
{
	if (force_min_size && extent.width() * scaling < 1.5f)
		painter.drawEllipse(extent.center(), 0.5f * (1/scaling), 0.5f * (1/scaling));
	else
		painter.drawEllipse(extent);
}

// ### CircleRenderable ###

CircleRenderable::CircleRenderable(PointSymbol* symbol, MapCoordF coord) : Renderable()
{
	color_priority = symbol->getOuterColor()->priority;
	double x = coord.getX();
	double y = coord.getY();
	line_width = 0.001f * symbol->getOuterWidth();
	double radius = (0.001 * symbol->getInnerRadius()) + 0.5f * line_width;
	rect = QRectF(x - radius, y - radius, 2 * radius, 2 * radius);
	extent = QRectF(rect.x() - 0.5*line_width, rect.y() - 0.5*line_width, rect.width() + line_width, rect.height() + line_width);
}
CircleRenderable::CircleRenderable(const CircleRenderable& other): Renderable(other)
{
	rect = other.rect;
	line_width = other.line_width;
}
void CircleRenderable::getRenderStates(RenderStates& out) const
{
	out.color_priority = color_priority;
	assert(out.color_priority < 3000);
	out.mode = RenderStates::PenOnly;
	out.pen_width = line_width;
	out.clip_path = clip_path;
}
void CircleRenderable::render(QPainter& painter, bool force_min_size, float scaling)
{
	if (force_min_size && rect.width() * scaling < 1.5f)
		painter.drawEllipse(rect.center(), 0.5f * (1/scaling), 0.5f * (1/scaling));
	else
		painter.drawEllipse(rect);
}

// ### LineRenderable ###

LineRenderable::LineRenderable(LineSymbol* symbol, const MapCoordVectorF& transformed_coords, const MapCoordVector& coords, const PathCoordVector& path_coords, bool closed) : Renderable()
{
	assert(transformed_coords.size() == coords.size());
	color_priority = symbol->getColor()->priority;
	line_width = 0.001f * symbol->getLineWidth();
	float half_line_width = 0.5f * line_width;
	
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
	
	bool hole = false;
	QPainterPath first_subpath;
	
	path.moveTo(transformed_coords[0].toQPointF());
	extent = QRectF(transformed_coords[0].getX(), transformed_coords[0].getY(), 0.0001f, 0.0001f);
	extentIncludeCap(0, half_line_width, false, symbol, transformed_coords, coords, closed);
	
	for (int i = 1; i < size; ++i)
	{
		if (hole)
		{
			assert(!coords[i].isHolePoint() && "Two hole points in a row!");
			if (first_subpath.isEmpty() && closed)
			{
				first_subpath = path;
				path = QPainterPath();
			}
			path.moveTo(transformed_coords[i].toQPointF());
			extentIncludeCap(i, half_line_width, false, symbol, transformed_coords, coords, closed);
			hole = false;
			continue;
		}
		
		if (coords[i-1].isCurveStart())
		{
            assert(i < size - 2);
			path.cubicTo(transformed_coords[i].toQPointF(), transformed_coords[i+1].toQPointF(), transformed_coords[i+2].toQPointF());
			i += 2;
		}
		else
			path.lineTo(transformed_coords[i].toQPointF());
		
		if (coords[i].isHolePoint())
			hole = true;
		
		if ((i < size - 1 && !hole) || (i == size - 1 && closed))
			extentIncludeJoin(i, half_line_width, symbol, transformed_coords, coords, closed);
		else
			extentIncludeCap(i, half_line_width, true, symbol, transformed_coords, coords, closed);
	}
	
	if (closed)
	{
		if (first_subpath.isEmpty())
			path.closeSubpath();
		else
			path.connectPath(first_subpath);
	}
	
	// Extend extent with bezier curve points from the path coords
	int path_coords_size = path_coords.size();
	for (int i = 1; i < path_coords_size - 1; ++i)
	{
		if (path_coords[i].param == 1)
			continue;
		
		MapCoordF to_coord = MapCoordF(path_coords[i].pos.getX() - path_coords[i-1].pos.getX(), path_coords[i].pos.getY() - path_coords[i-1].pos.getY());
		MapCoordF to_next = MapCoordF(path_coords[i+1].pos.getX() - path_coords[i].pos.getX(), path_coords[i+1].pos.getY() - path_coords[i].pos.getY());
		to_coord.normalize();
		to_next.normalize();
		MapCoordF right = MapCoordF(-to_coord.getY() - to_next.getY(), to_next.getX() + to_coord.getX());
		right.normalize();
		
		rectInclude(extent, QPointF(path_coords[i].pos.getX() + half_line_width * right.getX(), path_coords[i].pos.getY() + half_line_width * right.getY()));
		rectInclude(extent, QPointF(path_coords[i].pos.getX() - half_line_width * right.getX(), path_coords[i].pos.getY() - half_line_width * right.getY()));
	}
	
	assert(extent.right() < 999999);	// assert if bogus values are returned
}
void LineRenderable::extentIncludeCap(int i, float half_line_width, bool end_cap, LineSymbol* symbol, const MapCoordVectorF& transformed_coords, const MapCoordVector& coords, bool closed)
{
	if (symbol->getCapStyle() == LineSymbol::RoundCap)
	{
		rectInclude(extent, QPointF(transformed_coords[i].getX() - half_line_width, transformed_coords[i].getY() - half_line_width));
		rectInclude(extent, QPointF(transformed_coords[i].getX() + half_line_width, transformed_coords[i].getY() + half_line_width));
		return;
	}
	
	MapCoordF right = PathCoord::calculateRightVector(coords, transformed_coords, closed, i, NULL);
	rectInclude(extent, QPointF(transformed_coords[i].getX() + half_line_width * right.getX(), transformed_coords[i].getY() + half_line_width * right.getY()));
	rectInclude(extent, QPointF(transformed_coords[i].getX() - half_line_width * right.getX(), transformed_coords[i].getY() - half_line_width * right.getY()));
	
	if (symbol->getCapStyle() == LineSymbol::SquareCap)
	{
		MapCoordF back = right;
		back.perpRight();
		
		float sign = end_cap ? -1 : 1;
		rectInclude(extent, QPointF(transformed_coords[i].getX() + half_line_width * (-right.getX() + sign*back.getX()), transformed_coords[i].getY() + half_line_width * (-right.getY() + sign*back.getY())));
		rectInclude(extent, QPointF(transformed_coords[i].getX() + half_line_width * (right.getX() + sign*back.getX()), transformed_coords[i].getY() + half_line_width * (right.getY() + sign*back.getY())));
	}
}
void LineRenderable::extentIncludeJoin(int i, float half_line_width, LineSymbol* symbol, const MapCoordVectorF& transformed_coords, const MapCoordVector& coords, bool closed)
{
	if (symbol->getJoinStyle() == LineSymbol::RoundJoin)
	{
		rectInclude(extent, QPointF(transformed_coords[i].getX() - half_line_width, transformed_coords[i].getY() - half_line_width));
		rectInclude(extent, QPointF(transformed_coords[i].getX() + half_line_width, transformed_coords[i].getY() + half_line_width));
		return;
	}
	
	float scaling = 1;
	MapCoordF right = PathCoord::calculateRightVector(coords, transformed_coords, closed, i, (symbol->getJoinStyle() == LineSymbol::MiterJoin) ? &scaling : NULL);
	float factor = scaling * half_line_width;
	rectInclude(extent, QPointF(transformed_coords[i].getX() + factor * right.getX(), transformed_coords[i].getY() + factor * right.getY()));
	rectInclude(extent, QPointF(transformed_coords[i].getX() - factor * right.getX(), transformed_coords[i].getY() - factor * right.getY()));
}
LineRenderable::LineRenderable(const LineRenderable& other) : Renderable(other)
{
	path = other.path;
	line_width = other.line_width;
	cap_style = other.cap_style;
	join_style = other.join_style;
}
void LineRenderable::getRenderStates(RenderStates& out) const
{
	out.color_priority = color_priority;
	out.mode = RenderStates::PenOnly;
	out.pen_width = line_width;
	out.clip_path = clip_path;
}
void LineRenderable::render(QPainter& painter, bool force_min_size, float scaling)
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
		painter.drawEllipse(QPointF(e.x, e.y), 0.2f, 0.2f);
	}
	painter.setPen(pen);*/
}

// ### AreaRenderable ###

AreaRenderable::AreaRenderable(AreaSymbol* symbol, const MapCoordVectorF& transformed_coords, const MapCoordVector& coords, const PathCoordVector* path_coords) : Renderable()
{
	assert(transformed_coords.size() >= 3 && transformed_coords.size() == coords.size());
	color_priority = symbol->getColor() ? symbol->getColor()->priority : MapColor::Reserved;
	
	// Special case: first coord
	path.moveTo(transformed_coords[0].toQPointF());
	
	// Coords 1 to size - 1
	bool have_hole = false;
	int size = (int)coords.size();
	for (int i = 1; i < size; ++i)
	{
		if (have_hole)
		{
			path.closeSubpath();
			path.moveTo(transformed_coords[i].toQPointF());
			
			have_hole = false;
			continue;
		}
		
		if (coords[i-1].isCurveStart())
		{
			assert(i < size - 2);
			path.cubicTo(transformed_coords[i].toQPointF(), transformed_coords[i+1].toQPointF(), transformed_coords[i+2].toQPointF());
			i += 2;
		}
		else
			path.lineTo(transformed_coords[i].toQPointF());
		
		if (coords[i].isHolePoint())
			have_hole = true;
	}
	
	// Close path
	path.closeSubpath();
	
	//if (disable_holes)
	//	path.setFillRule(Qt::WindingFill);
	
	// Get extent
	if (path_coords && path_coords->size() > 0)
	{
		int path_coords_size = path_coords->size();
		extent = QRectF(path_coords->at(0).pos.getX(), path_coords->at(0).pos.getY(), 0.0001f, 0.0001f);
		for (int i = 1; i < path_coords_size; ++i)
			rectInclude(extent, path_coords->at(i).pos.toQPointF());
	}
	else
		extent = path.controlPointRect();
	assert(extent.right() < 999999);	// assert if bogus values are returned
}
AreaRenderable::AreaRenderable(const AreaRenderable& other) : Renderable(other)
{
	path = other.path;
}
void AreaRenderable::getRenderStates(RenderStates& out) const
{
	out.color_priority = color_priority;
	out.mode = RenderStates::BrushOnly;
	out.pen_width = 0;
	out.clip_path = clip_path;
}
void AreaRenderable::render(QPainter& painter, bool force_min_size, float scaling)
{
	painter.drawPath(path);
	
	// DEBUG: show all control points
	/*QPen pen(painter.pen());
	QBrush brush(painter.brush());
	QPen debugPen(QColor(Qt::red));
	painter.setPen(debugPen);
	painter.setBrush(Qt::NoBrush);
	for (int i = 0; i < path.elementCount(); ++i)
	{
		const QPainterPath::Element& e = path.elementAt(i);
		painter.drawEllipse(QPointF(e.x, e.y), 0.1f, 0.1f);
	}
	painter.setPen(pen);
	painter.setBrush(brush);*/
}

// ### TextRenderable ###

TextRenderable::TextRenderable(TextSymbol* symbol, TextObject* text_object, double anchor_x, double anchor_y)
{
	const QFont& font(symbol->getQFont());
	const QFontMetricsF& metrics(symbol->getFontMetrics());
	color_priority = symbol->getColor()->priority;
	this->anchor_x = anchor_x;
	this->anchor_y = anchor_y;
	this->rotation = text_object->getRotation();
	scale_factor = symbol->getFontSize() / TextSymbol::internal_point_size;
	
	path.setFillRule(Qt::WindingFill);	// Otherwise, when text and an underline intersect, holes appear
	
	int num_lines = text_object->getNumLines();
	for (int i=0; i < num_lines; i++)
	{
		const TextObjectLineInfo* line_info = text_object->getLineInfo(i);
		
		double line_y = line_info->line_y;
		
		double underline_x0 = 0.0;
		double underline_y0 = line_info->line_y + metrics.underlinePos();
		double underline_y1 = underline_y0 + metrics.lineWidth();
		
		int num_parts = line_info->part_infos.size();
		for (int j=0; j < num_parts; j++)
		{
			const TextObjectPartInfo& part(line_info->part_infos.at(j));
			if (font.underline())
			{
				if (j > 0)
				{
					// draw underline for gap between parts as rectangle
					// TODO: watch out for inconsistency between text and gap underline
					path.moveTo(underline_x0, underline_y0);
					path.lineTo(part.part_x,  underline_y0);
					path.lineTo(part.part_x,  underline_y1);
					path.lineTo(underline_x0, underline_y1);
					path.closeSubpath();
				}
				underline_x0 = part.part_x;
			}
			path.addText(part.part_x, line_y, font, part.part_text);
		}
	}
	
	extent = path.controlPointRect();
	extent = QRectF(scale_factor * extent.left(), scale_factor * extent.top(), scale_factor * extent.width(), scale_factor * extent.height());
	if (rotation != 0)
	{
		float rcos = cos(-rotation);
		float rsin = sin(-rotation);
		
		std::vector<QPointF> extent_corners;
		extent_corners.push_back(extent.topLeft());
		extent_corners.push_back(extent.topRight());
		extent_corners.push_back(extent.bottomRight());
		extent_corners.push_back(extent.bottomLeft());
		
		for (int i = 0; i < 4; ++i)
		{
			float x = extent_corners[i].x() * rcos - extent_corners[i].y() * rsin;
			float y = extent_corners[i].y() * rcos + extent_corners[i].x() * rsin;
			
			if (i == 0)
				extent = QRectF(x, y, 0, 0);
			else
				rectInclude(extent, QPointF(x, y));
		}
	}
	extent = QRectF(extent.left() + anchor_x, extent.top() + anchor_y, extent.width(), extent.height());
	
	assert(extent.right() < 999999);	// assert if bogus values are returned
}

TextRenderable::TextRenderable(const TextRenderable& other) : Renderable(other)
{
	path = other.path;
	anchor_x = other.anchor_x;
	anchor_y = other.anchor_y;
	rotation = other.rotation;
	scale_factor = other.scale_factor;
}

void TextRenderable::getRenderStates(RenderStates& out) const
{
	out.color_priority = color_priority;
	out.mode = RenderStates::BrushOnly;
	out.pen_width = 0;
	out.clip_path = clip_path;
}

void TextRenderable::render(QPainter& painter, bool force_min_size, float scaling)
{
	// NOTE: mini-optimization to prevent the save-restore for un-rotated texts which could be used when the scale-hack is no longer necessary
	/*if (rotation == 0)
		painter.drawPath(path);
	else
	{*/
		painter.save();
		painter.translate(anchor_x, anchor_y);
		if (rotation != 0)
			painter.rotate(-rotation * 180 / M_PI);
		painter.scale(scale_factor, scale_factor);
		painter.drawPath(path);
		painter.restore();
	//}
}
