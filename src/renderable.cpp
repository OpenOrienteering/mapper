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

#include "map.h"
#include "map_color.h"
#include "object.h"
#include "symbol_point.h"
#include "symbol_line.h"
#include "symbol_area.h"
#include "symbol_text.h"
#include "util.h"

Renderable::Renderable()
{
	clip_path = NULL;
}
Renderable::~Renderable()
{
}

// ### RenderableContainer ###

RenderableContainer::RenderableContainer(Map* map) : map(map)
{
}

void RenderableContainer::draw(QPainter* painter, QRectF bounding_box, float opacity_factor)
{
	Map::ColorVector& colors = map->color_set->colors;
	
	// TODO: improve performance by using some spatial acceleration structure?
	RenderStates states;
	states.color_priority = -1;
	states.mode = RenderStates::Reserved;
	states.clip_path = NULL;
	
	QPainterPath initial_clip = painter->clipPath();
	bool no_initial_clip = initial_clip.isEmpty();
	
	painter->save();
	for (Renderables::const_iterator it = renderables.begin(); it != renderables.end(); ++it)
	{
		const RenderStates& new_states = (*it).first;
		Renderable* renderable = (*it).second;
		
		// Bounds check
		const QRectF& extent = renderable->getExtent();
		if (extent.right() < bounding_box.x())	continue;
		if (extent.bottom() < bounding_box.y())	continue;
		if (extent.x() > bounding_box.right())	continue;
		if (extent.y() > bounding_box.bottom())	continue;
		
		// Change render states?
		if (states != new_states)
		{
			if (new_states.mode == RenderStates::PenOnly)
			{
				//if (forceMinSize && new_states.pen_width * scale <= 1.0f)
				//	painter->setPen(QPen(r->getSymbol()->getColor()->color, 0));
				//else
					painter->setPen(QPen(colors[new_states.color_priority]->color, new_states.pen_width));
				
				painter->setBrush(QBrush(Qt::NoBrush));
				painter->setOpacity(opacity_factor * colors[new_states.color_priority]->opacity);
			}
			else if (new_states.mode == RenderStates::BrushOnly)
			{
				QBrush brush(colors[new_states.color_priority]->color);
				
				painter->setPen(QPen(Qt::NoPen));
				painter->setBrush(brush);
				painter->setOpacity(opacity_factor * colors[new_states.color_priority]->opacity);
			}
			
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
		renderable->render(*painter); //, scale, forceMinSize);
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
			renderables.insert(Renderables::value_type(render_states, renderable));	
	}
}
void RenderableContainer::removeRenderablesOfObject(Object* object, bool mark_area_as_dirty)
{
	Renderables::iterator itend = renderables.end();
	for (Renderables::iterator it = renderables.begin(); it != itend; )
	{
		if ((*it).second->getCreator() == object)
		{
			if (mark_area_as_dirty)
				map->setObjectAreaDirty((*it).second->getExtent());
			Renderables::iterator todelete = it;
			++it;
			renderables.erase(todelete);
		}
		else
			++it;
	}
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
	PointSymbol* point = reinterpret_cast<PointSymbol*>(symbol);	// TODO: Check: do all renderables keep the symbol pointer just to fetch their color priority? Isn't that a bit strange?
	
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

// ### TextRenderable ###

TextRenderable::TextRenderable(TextSymbol* symbol, double line_x, double line_y, double anchor_x, double anchor_y, double rotation, const QString& line, const QFont& font) : Renderable()
{
	this->symbol = symbol;
	this->anchor_x = anchor_x;
	this->anchor_y = anchor_y;
	this->rotation = rotation;
	scale_factor = symbol->getFontSize() / TextSymbol::internal_point_size;
	
	path.setFillRule(Qt::WindingFill);	// Otherwise, when text and an underline intersect, holes appear
	/*if (rotation == 0)
		path.addText(line_x + anchor_x, line_y + anchor_y, font, line);
	else*/
		path.addText(line_x, line_y, font, line);
	
	// Get extent
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
void TextRenderable::getRenderStates(RenderStates& out)
{
	TextSymbol* text_symbol = reinterpret_cast<TextSymbol*>(symbol);
	
	out.color_priority = text_symbol->getColor()->priority;
	out.mode = RenderStates::BrushOnly;
	out.pen_width = 0;
	out.clip_path = clip_path;
}
void TextRenderable::render(QPainter& painter)
{
	// NOTE: mini-optimization to prevent the save-restore for un-rotated texts which could be used when the scale-hack is no longer necessary
	/*if (rotation == 0)
		painter.drawPath(path);
	else
	{*/
		painter.save();
		painter.translate(anchor_x, anchor_y);
		if (rotation != 0)
			painter.rotate(rotation);
		painter.scale(scale_factor, scale_factor);
		painter.drawPath(path);
		painter.restore();
	//}
}
