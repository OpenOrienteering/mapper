/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2015 Kai Pastor
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

#include "core/map_color.h"
#include "map.h"
#include "object.h"
#include "object_text.h"
#include "settings.h"
#include "symbol_area.h"
#include "symbol_line.h"
#include "symbol_point.h"
#include "symbol_text.h"
#include "util.h"

// ### DotRenderable ###

DotRenderable::DotRenderable(const PointSymbol* symbol, MapCoordF coord)
 : Renderable(symbol->getInnerColor()->getPriority())
{
	double x = coord.getX();
	double y = coord.getY();
	double radius = (0.001 * symbol->getInnerRadius());
	extent = QRectF(x - radius, y - radius, 2 * radius, 2 * radius);
}

DotRenderable::DotRenderable(const DotRenderable& other)
 : Renderable(other)
{
	; // nothing
}

PainterConfig DotRenderable::getPainterConfig(QPainterPath* clip_path) const
{
	return { color_priority, PainterConfig::BrushOnly, 0, clip_path };
}

void DotRenderable::render(QPainter &painter, const RenderConfig &config) const
{
	if (config.options.testFlag(RenderConfig::ForceMinSize) && extent.width() * config.scaling < 1.5f)
		painter.drawEllipse(extent.center(), 0.5f / config.scaling, 0.5f * config.scaling);
	else
		painter.drawEllipse(extent);
}



// ### CircleRenderable ###

CircleRenderable::CircleRenderable(const PointSymbol* symbol, MapCoordF coord)
 : Renderable(symbol->getOuterColor()->getPriority())
 , line_width(0.001f * symbol->getOuterWidth())
{
	double x = coord.getX();
	double y = coord.getY();
	double radius = (0.001 * symbol->getInnerRadius()) + 0.5f * line_width;
	rect = QRectF(x - radius, y - radius, 2 * radius, 2 * radius);
	extent = QRectF(rect.x() - 0.5*line_width, rect.y() - 0.5*line_width, rect.width() + line_width, rect.height() + line_width);
}

CircleRenderable::CircleRenderable(const CircleRenderable& other)
 : Renderable(other)
 , line_width(other.line_width)
 , rect(other.rect)
{
	; // nothing
}

PainterConfig CircleRenderable::getPainterConfig(QPainterPath* clip_path) const
{
	return { color_priority, PainterConfig::PenOnly, line_width, clip_path };
}

void CircleRenderable::render(QPainter &painter, const RenderConfig &config) const
{
	if (config.options.testFlag(RenderConfig::ForceMinSize) && rect.width() * config.scaling < 1.5f)
		painter.drawEllipse(rect.center(), 0.5f / config.scaling, 0.5f / config.scaling);
	else
		painter.drawEllipse(rect);
}



// ### LineRenderable ###

LineRenderable::LineRenderable(const LineSymbol* symbol, const VirtualPath& virtual_path, bool closed)
 : Renderable(symbol->getColor()->getPriority())
 , line_width(0.001f * symbol->getLineWidth())
{
	Q_ASSERT(virtual_path.size() >= 2);
	
	float half_line_width = (color_priority < 0) ? 0.0f : 0.5f * line_width;
	
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
	
	auto& flags  = virtual_path.coords.flags;
	auto& coords = virtual_path.coords;
	
	bool has_curve = false;
	bool hole = false;
	bool gap = false;
	QPainterPath first_subpath;
	
	auto i = virtual_path.first_index;
	path.moveTo(QPointF(coords[i]));
	extent = QRectF(coords[i].getX(), coords[i].getY(), 0.0001f, 0.0001f);
	extentIncludeCap(i, half_line_width, false, symbol, virtual_path);
	
	for (++i; i <= virtual_path.last_index; ++i)
	{
		if (gap)
		{
			if (flags[i].isHolePoint())
			{
				gap = false;
				hole = true;
			}
			else if (flags[i].isGapPoint())
			{
				gap = false;
				if (first_subpath.isEmpty() && closed)
				{
					first_subpath = path;
					path = QPainterPath();
				}
				path.moveTo(QPointF(coords[i]));
				extentIncludeCap(i, half_line_width, false, symbol, virtual_path);
			}
			continue;
		}
		
		if (hole)
		{
			Q_ASSERT(!flags[i].isHolePoint() && "Two hole points in a row!");
			if (first_subpath.isEmpty() && closed)
			{
				first_subpath = path;
				path = QPainterPath();
			}
			path.moveTo(QPointF(coords[i]));
			extentIncludeCap(i, half_line_width, false, symbol, virtual_path);
			hole = false;
			continue;
		}
		
		if (flags[i-1].isCurveStart())
		{
			Q_ASSERT(i < virtual_path.last_index-1);
			has_curve = true;
			path.cubicTo(QPointF(coords[i]), QPointF(coords[i+1]), QPointF(coords[i+2]));
			i += 2;
		}
		else
			path.lineTo(QPointF(coords[i]));
		
		if (flags[i].isHolePoint())
			hole = true;
		else if (flags[i].isGapPoint())
			gap = true;
		
		if ((i < virtual_path.last_index && !hole && !gap) || (i == virtual_path.last_index && closed))
			extentIncludeJoin(i, half_line_width, symbol, virtual_path);
		else
			extentIncludeCap(i, half_line_width, true, symbol, virtual_path);
	}
	
	if (closed)
	{
		if (first_subpath.isEmpty())
			path.closeSubpath();
		else
			path.connectPath(first_subpath);
	}
	
	// If we do not have the path coords, but there was a curve, calculate path coords.
	if (has_curve)
	{
		//  This happens for point symbols with curved lines in them.
		const auto& path_coords = virtual_path.path_coords;
		Q_ASSERT(path_coords.front().param == 0.0);
		Q_ASSERT(path_coords.back().param == 0.0);
		for (auto i = path_coords.size()-1; i > 0; --i)
		{
			if (path_coords[i].param != 0.0)
			{
				const auto& pos = path_coords[i].pos;
				auto to_coord   = pos - path_coords[i-1].pos;
				auto to_next    = path_coords[i+1].pos - pos;
				to_coord.normalize();
				to_next.normalize();
				auto right = to_coord + to_next;
				right.perpRight();
				right.normalize();
				right *= half_line_width;
				
				rectInclude(extent, QPointF(pos.getX() + right.getX(), pos.getY() + right.getY()));
				rectInclude(extent, QPointF(pos.getX() - right.getX(), pos.getY() - right.getY()));
			}
		}
	}
	Q_ASSERT(extent.right() < 999999);	// assert if bogus values are returned
}

LineRenderable::LineRenderable(const LineSymbol* symbol, QPointF first, QPointF second)
 : Renderable(symbol->getColor()->getPriority())
 , line_width(0.001f * symbol->getLineWidth())
 , cap_style(Qt::FlatCap)
 , join_style(Qt::MiterJoin)
{
	extent = QRectF(first, second).normalized();
	
	path.moveTo(first);
	path.lineTo(second);
}

void LineRenderable::extentIncludeCap(quint32 i, float half_line_width, bool end_cap, const LineSymbol* symbol, const VirtualPath& path)
{
	auto coords = path.coords;
	if (symbol->getCapStyle() == LineSymbol::RoundCap)
	{
		rectInclude(extent, QPointF(coords[i].getX() - half_line_width, coords[i].getY() - half_line_width));
		rectInclude(extent, QPointF(coords[i].getX() + half_line_width, coords[i].getY() + half_line_width));
		return;
	}
	
	MapCoordF right = path.calculateTangent(i);
	right.perpRight();
	right.normalize();
	rectInclude(extent, QPointF(coords[i].getX() + half_line_width * right.getX(), coords[i].getY() + half_line_width * right.getY()));
	rectInclude(extent, QPointF(coords[i].getX() - half_line_width * right.getX(), coords[i].getY() - half_line_width * right.getY()));
	
	if (symbol->getCapStyle() == LineSymbol::SquareCap)
	{
		MapCoordF back = right;
		back.perpRight();
		back.normalize();
		
		float sign = end_cap ? -1 : 1;
		rectInclude(extent, QPointF(coords[i].getX() + half_line_width * (-right.getX() + sign*back.getX()), coords[i].getY() + half_line_width * (-right.getY() + sign*back.getY())));
		rectInclude(extent, QPointF(coords[i].getX() + half_line_width * (right.getX() + sign*back.getX()), coords[i].getY() + half_line_width * (right.getY() + sign*back.getY())));
	}
}

void LineRenderable::extentIncludeJoin(quint32 i, float half_line_width, const LineSymbol* symbol, const VirtualPath& path)
{
	auto coords = path.coords;
	if (symbol->getJoinStyle() == LineSymbol::RoundJoin)
	{
		rectInclude(extent, QPointF(coords[i].getX() - half_line_width, coords[i].getY() - half_line_width));
		rectInclude(extent, QPointF(coords[i].getX() + half_line_width, coords[i].getY() + half_line_width));
		return;
	}
	
	auto factor = half_line_width;
	auto params = path.calculateTangentScaling(i);
	params.first.perpRight();
	params.first.normalize();
	if (symbol->getJoinStyle() == LineSymbol::MiterJoin)
	{
		factor *= qMin(params.second, 2.0 * LineSymbol::miterLimit());
	}
	rectInclude(extent, QPointF(coords[i].getX() + factor * params.first.getX(), coords[i].getY() + factor * params.first.getY()));
	rectInclude(extent, QPointF(coords[i].getX() - factor * params.first.getX(), coords[i].getY() - factor * params.first.getY()));
}

LineRenderable::LineRenderable(const LineRenderable& other)
 : Renderable(other)
 , line_width(other.line_width)
 , path(other.path)
 , cap_style(other.cap_style)
 , join_style(other.join_style)
{
	; // nothing
}

PainterConfig LineRenderable::getPainterConfig(QPainterPath* clip_path) const
{
	return { color_priority, PainterConfig::PenOnly, line_width, clip_path };
}

void LineRenderable::render(QPainter &painter, const RenderConfig &config) const
{
	QPen pen(painter.pen());
	pen.setCapStyle(cap_style);
	pen.setJoinStyle(join_style);
	if (join_style == Qt::MiterJoin)
		pen.setMiterLimit(LineSymbol::miterLimit());
	painter.setPen(pen);
	
	// One-time adjustment for line width
	QRectF bounding_box = config.bounding_box.adjusted(-line_width, -line_width, line_width, line_width);
	const int count = path.elementCount();
	if (count <= 2 || bounding_box.contains(path.controlPointRect()))
	{
		// path fully contained
		painter.drawPath(path);
	}
	else
	{
		// Manually clip the path with bounding_box, this seems to be faster.
		// The code splits up the painter path into new paths which intersect
		// the view rect and renders these only.
		// NOTE: this does not work correctly with miter joins, but this
		//       should be a minor issue.
		QPainterPath::Element element = path.elementAt(0);
		QPainterPath::Element last_element = path.elementAt(count-1);
		bool path_closed = (element.x == last_element.x) && (element.y == last_element.y);
		
		QPainterPath part_path;
		QPainterPath first_path;
		bool path_started = false;
		bool part_finished = false;
		bool current_part_is_first = bounding_box.contains(QPointF(element.x, element.y));
		
		QPainterPath::Element prev_element = element;
		for (int i = 1; i < count; ++i)
		{
			element = path.elementAt(i);
			if (element.isLineTo())
			{
				qreal min_x, min_y, max_x, max_y;
				if (prev_element.x < element.x)
				{
					min_x = prev_element.x;
					max_x = element.x;
				}
				else
				{
					min_x = element.x;
					max_x = prev_element.x;
				}
				if (prev_element.y < element.y)
				{
					min_y = prev_element.y;
					max_y = element.y;
				}
				else
				{
					min_y = element.y;
					max_y = prev_element.y;
				}
				if ( min_x <= bounding_box.right()  &&
				     max_x >= bounding_box.left()   &&
				     min_y <= bounding_box.bottom() &&
				     max_y >= bounding_box.top() )
				{
					if (!path_started)
					{
						part_path = QPainterPath();
						part_path.moveTo(prev_element.x, prev_element.y);
						path_started = true;
					}
					part_path.lineTo(element.x, element.y);
				}
				else if (path_started)
				{
					part_finished = true;
				}
				else
				{
					current_part_is_first = false;
				}
			}
			else if (element.isCurveTo())
			{
				Q_ASSERT(i < count - 2);
				QPainterPath::Element next_element = path.elementAt(i + 1);
				QPainterPath::Element end_element = path.elementAt(i + 2);
				
				qreal min_x = qMin(prev_element.x, qMin(element.x, qMin(next_element.x, end_element.x)));
				qreal min_y = qMin(prev_element.y, qMin(element.y, qMin(next_element.y, end_element.y)));
				qreal max_x = qMax(prev_element.x, qMax(element.x, qMax(next_element.x, end_element.x)));
				qreal max_y = qMax(prev_element.y, qMax(element.y, qMax(next_element.y, end_element.y)));
				
				if ( min_x <= bounding_box.right()  &&
				     max_x >= bounding_box.left()   &&
				     min_y <= bounding_box.bottom() &&
				     max_y >= bounding_box.top() )
				{
					if (!path_started)
					{
						part_path = QPainterPath();
						part_path.moveTo(prev_element.x, prev_element.y);
						path_started = true;
					}
					part_path.cubicTo(element.x, element.y, next_element.x, next_element.y, end_element.x, end_element.y);
				}
				else if (path_started)
				{
					part_finished = true;
				}
				else
				{
					current_part_is_first = false;
				}
			}
			else if (element.isMoveTo() && path_started)
			{
				part_path.moveTo(element.x, element.y);
			}
			
			if (part_finished)
			{
				if (current_part_is_first && path_closed)
				{
					current_part_is_first = false;
					first_path = part_path;
				}
				else
				{
					painter.drawPath(part_path);
				}
				
				path_started = false;
				part_finished = false;
			}
			
			prev_element = element;
		}
		
		if (path_started)
		{
			if (path_closed && !first_path.isEmpty())
				part_path.connectPath(first_path);
			
			painter.drawPath(part_path);
		}
	}
	
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

AreaRenderable::AreaRenderable(const AreaSymbol* symbol, const PathPartVector& path_parts)
 : Renderable(symbol->getColor() ? symbol->getColor()->getPriority() : MapColor::Reserved)
{
	if (!path_parts.empty())
	{
		auto part = begin(path_parts);
		if (part->size() > 2)
		{
			extent = part->path_coords.calculateExtent();
			addSubpath(*part);
			
			auto last = end(path_parts);
			for (++part; part != last; ++part)
			{
				rectInclude(extent, part->path_coords.calculateExtent());
				addSubpath(*part);
			}
		}
	}
	Q_ASSERT(extent.right() < 999999);	// assert if bogus values are returned
}

AreaRenderable::AreaRenderable(const AreaSymbol* symbol, const VirtualPath& virtual_path)
 : Renderable(symbol->getColor() ? symbol->getColor()->getPriority() : MapColor::Reserved)
{
	extent = virtual_path.path_coords.calculateExtent();
	addSubpath(virtual_path);
}

AreaRenderable::AreaRenderable(const AreaRenderable& other)
 : Renderable(other)
{
	path = other.path;
}

void AreaRenderable::addSubpath(const VirtualPath& virtual_path)
{
	auto& flags  = virtual_path.coords.flags;
	auto& coords = virtual_path.coords;
	Q_ASSERT(!flags.data().empty());
	
	auto i = virtual_path.first_index;
	path.moveTo(QPointF(coords[i]));
	for (++i; i <= virtual_path.last_index; ++i)
	{
		if (flags[i-1].isCurveStart())
		{
			Q_ASSERT(i+2 < coords.size());
			path.cubicTo(QPointF(coords[i]), QPointF(coords[i+1]), QPointF(coords[i+2]));
			i += 2;
		}
		else
		{
			path.lineTo(QPointF(coords[i]));
		}
	}
	path.closeSubpath();
}

PainterConfig AreaRenderable::getPainterConfig(QPainterPath* clip_path) const
{
	return { color_priority, PainterConfig::BrushOnly, 0, clip_path };
}

void AreaRenderable::render(QPainter &painter, const RenderConfig &) const
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

TextRenderable::TextRenderable(const TextSymbol* symbol, const TextObject* text_object, const MapColor* color, double anchor_x, double anchor_y, bool framing_line)
 : Renderable(color->getPriority())
{
	const QFont& font(symbol->getQFont());
	const QFontMetricsF& metrics(symbol->getFontMetrics());
	this->anchor_x = anchor_x;
	this->anchor_y = anchor_y;
	this->rotation = text_object->getRotation();
	scale_factor = symbol->getFontSize() / TextSymbol::internal_point_size;
	this->framing_line = framing_line;
	framing_line_width = framing_line ? (2 * 0.001 * symbol->getFramingLineHalfWidth() / scale_factor) : 0;
	
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
	extent = QRectF(scale_factor * (extent.left() - 0.5f * framing_line_width),
					scale_factor * (extent.top() - 0.5f * framing_line_width),
					scale_factor * (extent.width() + framing_line_width),
					scale_factor * (extent.height() + framing_line_width));
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
	
	Q_ASSERT(extent.right() < 999999);	// assert if bogus values are returned
}

TextRenderable::TextRenderable(const TextRenderable& other)
 : Renderable(other)
{
	path = other.path;
	anchor_x = other.anchor_x;
	anchor_y = other.anchor_y;
	rotation = other.rotation;
	scale_factor = other.scale_factor;
	framing_line = other.framing_line;
	framing_line_width = other.framing_line_width;
}

PainterConfig TextRenderable::getPainterConfig(QPainterPath* clip_path) const
{
	return framing_line ? PainterConfig{ color_priority, PainterConfig::PenOnly, framing_line_width, clip_path }
	                    : PainterConfig{ color_priority, PainterConfig::BrushOnly, 0, clip_path };
}

void TextRenderable::render(QPainter &painter, const RenderConfig &config) const
{
	painter.save();
	
	bool disable_antialiasing = config.options.testFlag(RenderConfig::Screen) && !(Settings::getInstance().getSettingCached(Settings::MapDisplay_TextAntialiasing).toBool());
	if (disable_antialiasing)
		painter.setRenderHint(QPainter::Antialiasing, false);
	
	if (framing_line)
	{
		QPen pen(painter.pen());
		pen.setJoinStyle(Qt::MiterJoin);
		pen.setMiterLimit(0.5);
		painter.setPen(pen);
	}
	
	painter.translate(anchor_x, anchor_y);
	if (rotation != 0)
		painter.rotate(-rotation * 180 / M_PI);
	painter.scale(scale_factor, scale_factor);
	painter.drawPath(path);
	
	painter.restore();
}
