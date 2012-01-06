/*
 *    Copyright 2011 Thomas Schöps
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

#include "symbol_point.h"
#include "map_color.h"

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
