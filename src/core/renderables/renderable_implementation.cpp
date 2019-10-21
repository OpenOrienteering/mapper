/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2017 Kai Pastor
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

#include <cstddef>
#include <iterator>
#include <memory>
#include <vector>

#include <QtMath>
#include <QtNumeric>
#include <QFont>
#include <QFontMetricsF>
#include <QPaintEngine>
#include <QPainter>
#include <QPen>
#include <QPoint>
#include <QTransform>
// IWYU pragma: no_include <QVariant>

#include "settings.h"
#include "core/map_coord.h"
#include "core/virtual_coord_vector.h"
#include "core/virtual_path.h"
#include "core/objects/object.h"
#include "core/objects/text_object.h"
#include "core/renderables/renderable.h"
#include "core/symbols/area_symbol.h"
#include "core/symbols/line_symbol.h"
#include "core/symbols/point_symbol.h"
#include "core/symbols/text_symbol.h"
#include "util/util.h"

#ifdef QT_PRINTSUPPORT_LIB
#  include "advanced_pdf_printer.h"
#endif

// IWYU pragma: no_forward_declare QFontMetricsF


namespace {

/**
 * When painting to a PDF engine, the miter limit must be adjusted from Qt's
 * concept to PDF's concept. This should be done in the PDF engine, but this
 * isn't the case. This may even result in PDF files which are considered
 * invalid by Reader & Co.
 * 
 * The PDF miter limit could be precalculated for the Mapper use cases, but
 * this optimization would be lost when Qt gets fixed.
 * 
 * Upstream issue: QTBUG-52641
 */
inline void fixPenForPdf(QPen& pen, const QPainter& painter)
{
#ifdef QT_PRINTSUPPORT_LIB
	auto engine = painter.paintEngine()->type();
	if (Q_UNLIKELY(engine == QPaintEngine::Pdf
	               || engine == AdvancedPdfPrinter::paintEngineType()))
	{
		const auto miter_limit = pen.miterLimit();
		pen.setMiterLimit(qSqrt(1.0 + miter_limit * miter_limit * 4));
	}
#else
	Q_UNUSED(pen)
	Q_UNUSED(painter)
#endif
}

}  // namespace



namespace OpenOrienteering {

// ### DotRenderable ###

DotRenderable::DotRenderable(const PointSymbol* symbol, MapCoordF coord)
 : Renderable(symbol->getInnerColor())
{
	double x = coord.x();
	double y = coord.y();
	double radius = (0.001 * symbol->getInnerRadius());
	extent = QRectF(x - radius, y - radius, 2 * radius, 2 * radius);
}

PainterConfig DotRenderable::getPainterConfig(const QPainterPath* clip_path) const
{
	return { color_priority, PainterConfig::BrushOnly, 0, clip_path };
}

void DotRenderable::render(QPainter &painter, const RenderConfig &config) const
{
	if (config.options.testFlag(RenderConfig::ForceMinSize) && extent.width() * config.scaling < 1.5)
		painter.drawEllipse(extent.center(), 0.5 / config.scaling, 0.5 / config.scaling);
	else
		painter.drawEllipse(extent);
}



// ### CircleRenderable ###

CircleRenderable::CircleRenderable(const PointSymbol* symbol, MapCoordF coord)
 : Renderable(symbol->getOuterColor())
 , line_width(0.001 * symbol->getOuterWidth())
{
	double x = coord.x();
	double y = coord.y();
	double radius = (0.001 * symbol->getInnerRadius()) + line_width/2;
	rect = QRectF(x - radius, y - radius, 2 * radius, 2 * radius);
	extent = QRectF(rect.x() - 0.5*line_width, rect.y() - 0.5*line_width, rect.width() + line_width, rect.height() + line_width);
}

PainterConfig CircleRenderable::getPainterConfig(const QPainterPath* clip_path) const
{
	return { color_priority, PainterConfig::PenOnly, line_width, clip_path };
}

void CircleRenderable::render(QPainter &painter, const RenderConfig &config) const
{
	if (config.options.testFlag(RenderConfig::ForceMinSize) && rect.width() * config.scaling < 1.5)
		painter.drawEllipse(rect.center(), 0.5 / config.scaling, 0.5 / config.scaling);
	else
		painter.drawEllipse(rect);
}



// ### LineRenderable ###

LineRenderable::LineRenderable(const LineSymbol* symbol, const VirtualPath& virtual_path, bool closed)
 : Renderable(symbol->getColor())
 , line_width(0.001 * symbol->getLineWidth())
{
	Q_ASSERT(virtual_path.size() >= 2);
	
	qreal half_line_width = (color_priority < 0) ? 0 : line_width/2;
	
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
	QPainterPath first_subpath;
	
	auto i = virtual_path.first_index;
	bool gap = flags[i].isGapPoint();  // Line may start with a gap
	path.moveTo(coords[i]);
	extent = QRectF(coords[i].x(), coords[i].y(), 0.0001, 0.0001);
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
				path.moveTo(coords[i]);
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
			path.moveTo(coords[i]);
			extentIncludeCap(i, half_line_width, false, symbol, virtual_path);
			hole = false;
			continue;
		}
		
		if (flags[i-1].isCurveStart())
		{
			Q_ASSERT(i < virtual_path.last_index-1);
			has_curve = true;
			path.cubicTo(coords[i], coords[i+1], coords[i+2]);
			i += 2;
		}
		else
			path.lineTo(coords[i]);
		
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
		Q_ASSERT(path_coords.front().param == 0.0f);
		Q_ASSERT(path_coords.back().param == 0.0f);
		for (auto i = path_coords.size()-1; i > 0; --i)
		{
			if (path_coords[i].param != 0.0f)
			{
				const auto& pos = path_coords[i].pos;
				auto to_coord   = pos - path_coords[i-1].pos;
				auto to_next    = path_coords[i+1].pos - pos;
				to_coord.normalize();
				to_next.normalize();
				auto right = (to_coord + to_next).perpRight();
				right.setLength(half_line_width);
				
				rectInclude(extent, pos + right);
				rectInclude(extent, pos - right);
			}
		}
	}
	Q_ASSERT(extent.right() < 60000000);	// assert if bogus values are returned
}

LineRenderable::LineRenderable(const LineSymbol* symbol, QPointF first, QPointF second)
 : Renderable(symbol->getColor())
 , line_width(0.001 * symbol->getLineWidth())
 , cap_style(Qt::FlatCap)
 , join_style(Qt::MiterJoin)
{
	qreal half_line_width = (color_priority < 0) ? 0 : line_width/2;
	
	auto right = MapCoordF(second - first).perpRight();
	right.normalize();
	right *= half_line_width;
	
	extent.setTopLeft(first + right);
	rectInclude(extent, first - right);
	rectInclude(extent, second - right);
	rectInclude(extent, second + right);
	
	path.moveTo(first);
	path.lineTo(second);
}

void LineRenderable::extentIncludeCap(quint32 i, qreal half_line_width, bool end_cap, const LineSymbol* symbol, const VirtualPath& path)
{
	const auto& coord = path.coords[i];
	if (half_line_width < 0.0005)
	{
		rectInclude(extent, coord);
		return;
	}
	
	if (symbol->getCapStyle() == LineSymbol::RoundCap)
	{
		rectInclude(extent, QPointF(coord.x() - half_line_width, coord.y() - half_line_width));
		rectInclude(extent, QPointF(coord.x() + half_line_width, coord.y() + half_line_width));
		return;
	}
	
	auto right = path.calculateTangent(i).perpRight();
	right.normalize();
	rectInclude(extent, coord + half_line_width * right);
	rectInclude(extent, coord - half_line_width * right);
	
	if (symbol->getCapStyle() == LineSymbol::SquareCap)
	{
		auto back = right.perpRight();
		if (end_cap)
		    back = -back;
		rectInclude(extent, coord + half_line_width * (back - right));
		rectInclude(extent, coord + half_line_width * (back + right));
	}
}

void LineRenderable::extentIncludeJoin(quint32 i, qreal half_line_width, const LineSymbol* symbol, const VirtualPath& path)
{
	const auto& coord = path.coords[i];
	if (half_line_width < 0.0005)
	{
		rectInclude(extent, coord);
		return;
	}
	
	if (symbol->getJoinStyle() == LineSymbol::RoundJoin)
	{
		rectInclude(extent, QPointF(coord.x() - half_line_width, coord.y() - half_line_width));
		rectInclude(extent, QPointF(coord.x() + half_line_width, coord.y() + half_line_width));
		return;
	}
	
	bool ok_to_coord, ok_to_next;
	MapCoordF to_coord = path.calculateIncomingTangent(i, ok_to_coord);
	MapCoordF to_next = path.calculateOutgoingTangent(i, ok_to_next);
	if (!ok_to_next)
	{
		if (!ok_to_coord)
			return;
		to_next = to_coord;
	}
	else if (!ok_to_coord)
	{
		to_coord = to_next;
	}
	
	auto r0 = to_coord.perpRight();
	r0.setLength(half_line_width);
	auto r1 = to_next.perpRight();
	r1.setLength(half_line_width);
	
	auto to_coord_rhs = coord + r0;
	auto to_coord_lhs = coord - r0;
	auto to_next_rhs  = coord + r1;
	auto to_next_lhs  = coord - r1;
	if (symbol->getJoinStyle() == LineSymbol::BevelJoin)
	{
		rectInclude(extent, to_coord_rhs);
		rectInclude(extent, to_coord_lhs);
		rectInclude(extent, to_next_rhs);
		rectInclude(extent, to_next_lhs);
		return;
	}
	
	auto limit = line_width * LineSymbol::miterLimit();
	to_coord.setLength(limit);
	to_next.setLength(limit);
	
	const auto scaling = to_coord.y() * to_next.x() - to_coord.x() * to_next.y();
	if (qIsNull(scaling) || !qIsFinite(scaling))
		return; // straight line, no impact on extent
	
	// rhs boundary
	auto p = to_coord_rhs - to_next_rhs;
	auto factor = (to_next.y() * p.x() - to_next.x() * p.y()) / scaling;
	if (factor > 1)
	{
		// outer boundary, intersection exceeds miter limit
		rectInclude(extent, to_coord_rhs + to_coord);
		rectInclude(extent, to_next_rhs - to_next);
	}
	else if (factor > 0)
	{
		// outer boundary, intersection within miter limit
		rectInclude(extent, to_coord_rhs + to_coord * factor);
	}
	else
	{
		// inner boundary
		rectInclude(extent, to_coord_rhs);
		rectInclude(extent, to_next_rhs);
	}
	
	// lhs boundary
	p = to_coord_lhs - to_next_lhs;
	factor = (to_next.y() * p.x() - to_next.x() * p.y()) / scaling;
	if (factor > 1)
	{
		// outer boundary, intersection exceeds miter limit
		rectInclude(extent, to_coord_lhs + to_coord);
		rectInclude(extent, to_next_lhs - to_next);
	}
	else if (factor > 0)
	{
		// outer boundary, intersection within miter limit
		rectInclude(extent, to_coord_lhs + to_coord * factor);
	}
	else
	{
		// inner boundary, catch rare cases
		rectInclude(extent, to_coord_lhs);
		rectInclude(extent, to_next_lhs);
	}
}

PainterConfig LineRenderable::getPainterConfig(const QPainterPath* clip_path) const
{
	return { color_priority, PainterConfig::PenOnly, line_width, clip_path };
}

void LineRenderable::render(QPainter &painter, const RenderConfig &config) const
{
	QPen pen(painter.pen());
	pen.setCapStyle(cap_style);
	pen.setJoinStyle(join_style);
	if (join_style == Qt::MiterJoin)
	{
		pen.setMiterLimit(LineSymbol::miterLimit());
		fixPenForPdf(pen, painter);
	}
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
		bool current_part_is_first = bounding_box.contains(element);
		
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
 : Renderable(symbol->getColor())
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
	Q_ASSERT(extent.right() < 60000000);	// assert if bogus values are returned
}

AreaRenderable::AreaRenderable(const AreaSymbol* symbol, const VirtualPath& path)
 : Renderable(symbol->getColor())
{
	extent = path.path_coords.calculateExtent();
	addSubpath(path);
}

void AreaRenderable::addSubpath(const VirtualPath& virtual_path)
{
	auto& flags  = virtual_path.coords.flags;
	auto& coords = virtual_path.coords;
	Q_ASSERT(!flags.data().empty());
	
	auto i = virtual_path.first_index;
	path.moveTo(coords[i]);
	for (++i; i <= virtual_path.last_index; ++i)
	{
		if (flags[i-1].isCurveStart())
		{
			Q_ASSERT(i+2 < coords.size());
			path.cubicTo(coords[i], coords[i+1], coords[i+2]);
			i += 2;
		}
		else
		{
			path.lineTo(coords[i]);
		}
	}
	path.closeSubpath();
}

PainterConfig AreaRenderable::getPainterConfig(const QPainterPath* clip_path) const
{
	return { color_priority, PainterConfig::BrushOnly, 0, clip_path };
}

void AreaRenderable::render(QPainter &painter, const RenderConfig &/*config*/) const
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

TextRenderable::TextRenderable(const TextSymbol* symbol, const TextObject* text_object, const MapColor* color, double anchor_x, double anchor_y)
: Renderable { color }
, anchor_x   { anchor_x }
, anchor_y   { anchor_y }
, rotation   { 0.0 }
, scale_factor { symbol->getFontSize() / TextSymbol::internal_point_size }
{
	path.setFillRule(Qt::WindingFill);	// Otherwise, when text and an underline intersect, holes appear
	
	const QFont& font(symbol->getQFont());
	const QFontMetricsF& metrics(symbol->getFontMetrics());
	
	int num_lines = text_object->getNumLines();
	for (int i=0; i < num_lines; i++)
	{
		const TextObjectLineInfo* line_info = text_object->getLineInfo(i);
		
		double line_y = line_info->line_y;
		
		double underline_x0 = 0.0;
		double underline_y0 = line_info->line_y + metrics.underlinePos();
		double underline_y1 = underline_y0 + metrics.lineWidth();
		
		auto num_parts = line_info->part_infos.size();
		for (std::size_t j=0; j < num_parts; j++)
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
	
	QTransform t { 1.0, 0.0, 0.0, 1.0, anchor_x, anchor_y };
	t.scale(scale_factor, scale_factor);
	
	auto rotation_rad = text_object->getRotation();
	if (!qIsNull(rotation_rad))
	{
		rotation = -qRadiansToDegrees(rotation_rad);
		t.rotate(rotation);
	}
	
	extent = t.mapRect(path.controlPointRect());
}

PainterConfig TextRenderable::getPainterConfig(const QPainterPath* clip_path) const
{
	return { color_priority, PainterConfig::BrushOnly, 0.0, clip_path };
}

void TextRenderable::render(QPainter &painter, const RenderConfig &config) const
{
	painter.save();
	renderCommon(painter, config);
	painter.restore();
}

void TextRenderable::renderCommon(QPainter& painter, const RenderConfig& config) const
{
	bool disable_antialiasing = config.options.testFlag(RenderConfig::Screen) && !(Settings::getInstance().getSettingCached(Settings::MapDisplay_TextAntialiasing).toBool());
	if (disable_antialiasing)
	{
		painter.setRenderHint(QPainter::Antialiasing, false);
		painter.setRenderHint(QPainter::TextAntialiasing, false);
	}
	
	painter.translate(anchor_x, anchor_y);
	if (rotation != 0.0)
		painter.rotate(rotation);
	painter.scale(scale_factor, scale_factor);
	painter.drawPath(path);
}



// ### TextRenderable ###

TextFramingRenderable::TextFramingRenderable(const TextSymbol* symbol, const TextObject* text_object, const MapColor* color, double anchor_x, double anchor_y)
: TextRenderable     { symbol, text_object, color, anchor_x, anchor_y }
, framing_line_width { 2 * 0.001 * symbol->getFramingLineHalfWidth() / scale_factor }
{
	auto adjustment = 0.001 * symbol->getFramingLineHalfWidth() ;
	extent.adjust(-adjustment, -adjustment, +adjustment, +adjustment);
}

PainterConfig TextFramingRenderable::getPainterConfig(const QPainterPath* clip_path) const
{
	return { color_priority, PainterConfig::PenOnly, framing_line_width, clip_path };
}

void TextFramingRenderable::render(QPainter& painter, const RenderConfig& config) const
{
	painter.save();
	QPen pen(painter.pen());
	pen.setJoinStyle(Qt::MiterJoin);
	pen.setMiterLimit(0.5);
	fixPenForPdf(pen, painter);
	painter.setPen(pen);
	TextRenderable::renderCommon(painter, config);
	painter.restore();
}


}  // namespace OpenOrienteering
