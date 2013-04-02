/*
 *    Copyright 2012, 2013 Thomas Schöps
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


#ifndef _OPENORIENTEERING_RENDERABLE_IMPLENTATION_H_
#define _OPENORIENTEERING_RENDERABLE_IMPLENTATION_H_

#include <vector>

#include <QPainter>

#include <map>

#include "renderable.h"
#include "map_coord.h"
#include "path_coord.h"

class QPainterPath;

class Map;
class MapColor;
class Object;
class Symbol;
class PointSymbol;
class LineSymbol;
class AreaSymbol;
class TextSymbol;
class TextObject;
struct TextObjectLineInfo;

/** Renderable for displaying a filled dot. */
class DotRenderable : public Renderable
{
public:
	DotRenderable(PointSymbol* symbol, MapCoordF coord);
	DotRenderable(const DotRenderable& other);
	virtual void render(QPainter& painter, QRectF& bounding_box, bool force_min_size, float scaling, bool on_screen) const;
	virtual void getRenderStates(RenderStates& out) const;
	//virtual Renderable* duplicate() {return new DotRenderable(*this);}
};

/** Renderable for displaying a circle. */
class CircleRenderable : public Renderable
{
public:
	CircleRenderable(PointSymbol* symbol, MapCoordF coord);
	CircleRenderable(const CircleRenderable& other);
	virtual void render(QPainter& painter, QRectF& bounding_box, bool force_min_size, float scaling, bool on_screen) const;
	virtual void getRenderStates(RenderStates& out) const;
	//virtual Renderable* duplicate() {return new CircleRenderable(*this);}
	
protected:
	QRectF rect;
	float line_width;
};

/** Renderable for displaying a line. */
class LineRenderable : public Renderable
{
public:
	LineRenderable(LineSymbol* symbol, const MapCoordVectorF& transformed_coords, const MapCoordVector& coords, const PathCoordVector& path_coords, bool closed);
	LineRenderable(const LineRenderable& other);
	virtual void render(QPainter& painter, QRectF& bounding_box, bool force_min_size, float scaling, bool on_screen) const;
	virtual void getRenderStates(RenderStates& out) const;
	//virtual Renderable* duplicate() {return new LineRenderable(*this);}
	
protected:
	void extentIncludeCap(int i, float half_line_width, bool end_cap, LineSymbol* symbol, const MapCoordVectorF& transformed_coords, const MapCoordVector& coords, bool closed);
	void extentIncludeJoin(int i, float half_line_width, LineSymbol* symbol, const MapCoordVectorF& transformed_coords, const MapCoordVector& coords, bool closed);
	
	QPainterPath path;
	float line_width;
	Qt::PenCapStyle cap_style;
	Qt::PenJoinStyle join_style;
};

/** Renderable for displaying an area. */
class AreaRenderable : public Renderable
{
public:
	AreaRenderable(AreaSymbol* symbol, const MapCoordVectorF& transformed_coords, const MapCoordVector& coords, const PathCoordVector* path_coords);
	AreaRenderable(const AreaRenderable& other);
	virtual void render(QPainter& painter, QRectF& bounding_box, bool force_min_size, float scaling, bool on_screen) const;
	virtual void getRenderStates(RenderStates& out) const;
	//virtual Renderable* duplicate() {return new AreaRenderable(*this);}
	
	inline QPainterPath* getPainterPath() {return &path;}
	
protected:
	QPainterPath path;
};

/** Renderable for displaying text. */
class TextRenderable : public Renderable
{
public:
	TextRenderable(TextSymbol* symbol, TextObject* text_object, const MapColor* color, double anchor_x, double anchor_y, bool framing_line = false);
	TextRenderable(const TextRenderable& other);
	virtual void render(QPainter& painter, QRectF& bounding_box, bool force_min_size, float scaling, bool on_screen) const;
	virtual void getRenderStates(RenderStates& out) const;
	//virtual Renderable* duplicate() {return new TextRenderable(*this);}
	
protected:
	QPainterPath path;
	double anchor_x;
	double anchor_y;
	double rotation;
	double scale_factor;
	bool framing_line;
	float framing_line_width;
};

#endif
