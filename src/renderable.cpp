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

Renderable::Renderable()
{
	clip_path = NULL;
}
Renderable::Renderable(const Renderable& other)
{
	extent = other.extent;
	creator = other.creator;
	color_priority = other.color_priority;
	clip_path = other.clip_path;
}
Renderable::~Renderable()
{
}

// ### RenderableContainer ###

RenderableContainer::RenderableContainer(Map* map) : map(map)
{
}

void RenderableContainer::draw(QPainter* painter, QRectF bounding_box, bool force_min_size, float scaling, bool show_helper_symbols, float opacity_factor, bool highlighted)
{
	Map::ColorVector& colors = map->color_set->colors;
	
	// TODO: improve performance by using some spatial acceleration structure?
	RenderStates states;
	states.color_priority = -1;
	states.mode = RenderStates::Reserved;
	states.pen_width = -1;
	states.clip_path = NULL;
	
	QPainterPath initial_clip = painter->clipPath();
	bool no_initial_clip = initial_clip.isEmpty();
	
	painter->save();
	Renderables::const_reverse_iterator outer_it_end = renderables.rend();
	for (Renderables::const_reverse_iterator outer_it = renderables.rbegin(); outer_it != outer_it_end; ++outer_it)
	{
		RenderableContainerVector::const_iterator it_end = outer_it->second.end();
		for (RenderableContainerVector::const_iterator it = outer_it->second.begin(); it != it_end; ++it)
		{
			const RenderStates& new_states = (*it).first;
			Renderable* renderable = (*it).second;
			
			// Bounds check
			const QRectF& extent = renderable->getExtent();
			if (extent.right() < bounding_box.x())	continue;
			if (extent.bottom() < bounding_box.y())	continue;
			if (extent.x() > bounding_box.right())	continue;
			if (extent.y() > bounding_box.bottom())	continue;
			
			// Settings check
			Symbol* symbol = renderable->getCreator()->getSymbol();
			if (!show_helper_symbols && symbol->isHelperSymbol())
				continue;
			if (symbol->isHidden())
				continue;
			
			// Change render states?
			if (states != new_states)
			{
				MapColor* color;
				float pen_width;
				
				if (new_states.color_priority > MapColor::Reserved)
				{
					pen_width = new_states.pen_width;
					color = colors[new_states.color_priority];
				}
				else
				{
					painter->setRenderHint(QPainter::Antialiasing, true);	// this is not undone here anywhere as it should apply to all special symbols and these are always painted last
					pen_width = new_states.pen_width / scaling;
					
					if (new_states.color_priority == MapColor::CoveringWhite)
						color = Map::getCoveringWhite();
					else if (new_states.color_priority == MapColor::CoveringRed)
						color = Map::getCoveringRed();
					else if (new_states.color_priority == MapColor::Undefined)
						color = Map::getUndefinedColor();
					else
						assert(!"Invalid special color!");
				}
				
				if (new_states.mode == RenderStates::PenOnly)
				{
					bool pen_too_small = (force_min_size && pen_width * scaling <= 1.0f);
					painter->setPen(QPen(highlighted ? getHighlightedColor(color->color) : color->color, pen_too_small ? 0 : pen_width));
					
					painter->setBrush(QBrush(Qt::NoBrush));
				}
				else if (new_states.mode == RenderStates::BrushOnly)
				{
					QBrush brush(highlighted ? getHighlightedColor(color->color) : color->color);
					
					painter->setPen(QPen(Qt::NoPen));
					painter->setBrush(brush);
				}
				
				painter->setOpacity(qMin(1.0f, opacity_factor * color->opacity));
				
				if (states.clip_path != new_states.clip_path)
				{
					if (no_initial_clip)
					{
						if (new_states.clip_path)
							painter->setClipPath(*new_states.clip_path, Qt::ReplaceClip);
						else
							painter->setClipPath(initial_clip, Qt::NoClip);
					}
					else
					{
						painter->setClipPath(initial_clip, Qt::ReplaceClip);
						if (new_states.clip_path)
							painter->setClipPath(*new_states.clip_path, Qt::IntersectClip);
					}
				}
				
				states = new_states;
			}
			
			// Render the renderable
			renderable->render(*painter, force_min_size, scaling);
		}
	}
	painter->restore();
}

void RenderableContainer::insertRenderablesOfObject(Object* object)
{
	RenderableVector::const_iterator it_end = object->endRenderables();
	for (RenderableVector::const_iterator it = object->beginRenderables(); it != it_end; ++it)
	{
		Renderable* renderable = *it;
		RenderStates render_states;
		renderable->getRenderStates(render_states);
		
		if (render_states.color_priority != MapColor::Reserved)
		{
			renderables[render_states.color_priority].push_back(std::make_pair(render_states, renderable));
			//renderables.insert(Renderables::value_type(render_states, renderable));
		}
	}
}
void RenderableContainer::removeRenderablesOfObject(Object* object, bool mark_area_as_dirty)
{
	Renderables::iterator itend = renderables.end();
	for (Renderables::iterator it = renderables.begin(); it != itend; ++it)
	{
		for (int i = (int)it->second.size() - 1; i >= 0; --i)
		{
			if (it->second.at(i).second->getCreator() != object)
				continue;
			
			// NOTE: this assumes that renderables by one object are at continuous indices
			int k = i;
			if (mark_area_as_dirty)
				map->setObjectAreaDirty(it->second.at(i).second->getExtent());
			while (k > 0 && it->second.at(k - 1).second->getCreator() == object)
			{
				--k;
				if (mark_area_as_dirty)
					map->setObjectAreaDirty(it->second.at(k).second->getExtent());
			}
			
			it->second.erase(it->second.begin() + k, it->second.begin() + (i + 1));
			break;
		}
		
		/*if ((*it).second->getCreator() == object)
		{
			if (mark_area_as_dirty)
				map->setObjectAreaDirty((*it).second->getExtent());
			Renderables::iterator todelete = it;
			++it;
			renderables.erase(todelete);
		}
		else
			++it;*/
	}
}

void RenderableContainer::clear()
{
	renderables.clear();
}

QColor RenderableContainer::getHighlightedColor(const QColor& original)
{
	const int highlight_alpha = 255;
	
	if (original.value() > 127)
	{
		const float factor = 0.35f;
		return QColor(factor * original.red(), factor * original.green(), factor * original.blue(), highlight_alpha);
	}
	else
	{
		const float factor = 0.15f;
		return QColor(255 - factor * (255 - original.red()), 255 - factor * (255 - original.green()), 255 - factor * (255 - original.blue()), highlight_alpha);
	}
}

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
void DotRenderable::getRenderStates(RenderStates& out)
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
void CircleRenderable::getRenderStates(RenderStates& out)
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
void LineRenderable::getRenderStates(RenderStates& out)
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
void AreaRenderable::getRenderStates(RenderStates& out)
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

TextRenderable::TextRenderable(TextSymbol* symbol, double line_x, double line_y, double anchor_x, double anchor_y, double rotation, const QString& line, const QFont& font) : Renderable()
{
	color_priority = symbol->getColor()->priority;
	this->anchor_x = anchor_x;
	this->anchor_y = anchor_y;
	this->rotation = rotation;
	scale_factor = symbol->getFontSize() / TextSymbol::internal_point_size;
	
	path.setFillRule(Qt::WindingFill);	// Otherwise, when text and an underline intersect, holes appear
	/*if (rotation == 0)
		path.addText(line_x + anchor_x, line_y + anchor_y, font, line);
	else*/
		path.addText(line_x, line_y, font, line);
	
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

void TextRenderable::getRenderStates(RenderStates& out)
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
