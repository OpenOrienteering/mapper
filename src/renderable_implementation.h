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


#ifndef _OPENORIENTEERING_RENDERABLE_IMPLENTATION_H_
#define _OPENORIENTEERING_RENDERABLE_IMPLENTATION_H_

#include <QPainter>

#include "object.h"
#include "renderable.h"

class QPainterPath;

class AreaSymbol;
class LineSymbol;
class Map;
class MapColor;
class MapCoordF;
class Object;
class PathCoordVector;
class PointSymbol;
class Symbol;
class TextObject;
struct TextObjectLineInfo;
class TextSymbol;

/** Renderable for displaying a filled dot. */
class DotRenderable : public Renderable
{
public:
	DotRenderable(const PointSymbol* symbol, MapCoordF coord);
	explicit DotRenderable(const DotRenderable& other);
	virtual void render(QPainter& painter, const RenderConfig& config) const override;
	virtual PainterConfig getPainterConfig(QPainterPath* clip_path = nullptr) const override;
};

/** Renderable for displaying a circle. */
class CircleRenderable : public Renderable
{
public:
	CircleRenderable(const PointSymbol* symbol, MapCoordF coord);
	explicit CircleRenderable(const CircleRenderable& other);
	virtual void render(QPainter& painter, const RenderConfig& config) const override;
	virtual PainterConfig getPainterConfig(QPainterPath* clip_path = nullptr) const override;
	
protected:
	const float line_width;
	QRectF rect;
};

/** Renderable for displaying a line. */
class LineRenderable : public Renderable
{
public:
	LineRenderable(const LineSymbol* symbol, const VirtualPath& virtual_path, bool closed);
	LineRenderable(const LineSymbol* symbol, QPointF first, QPointF second);
	explicit LineRenderable(const LineRenderable& other);
	virtual void render(QPainter& painter, const RenderConfig& config) const override;
	virtual PainterConfig getPainterConfig(QPainterPath* clip_path = nullptr) const override;
	
protected:
	void extentIncludeCap(quint32 i, float half_line_width, bool end_cap, const LineSymbol* symbol, const VirtualPath& path);
	
	void extentIncludeJoin(quint32 i, float half_line_width, const LineSymbol* symbol, const VirtualPath& path);
	
	const float line_width;
	QPainterPath path;
	Qt::PenCapStyle cap_style;
	Qt::PenJoinStyle join_style;
};

/** Renderable for displaying an area. */
class AreaRenderable : public Renderable
{
public:
	AreaRenderable(const AreaSymbol* symbol, const PathPartVector& path_parts);
	AreaRenderable(const AreaSymbol* symbol, const VirtualPath& path);
	explicit AreaRenderable(const AreaRenderable& other);
	virtual void render(QPainter& painter, const RenderConfig& config) const override;
	virtual PainterConfig getPainterConfig(QPainterPath* clip_path = nullptr) const override;
	
	inline QPainterPath* getPainterPath() {return &path;}
	
protected:
	void addSubpath(const VirtualPath& virtual_path);
	
	QPainterPath path;
};

/** Renderable for displaying text. */
class TextRenderable : public Renderable
{
public:
	TextRenderable(const TextSymbol* symbol, const TextObject* text_object, const MapColor* color, double anchor_x, double anchor_y, bool framing_line = false);
	explicit TextRenderable(const TextRenderable& other);
	virtual void render(QPainter& painter, const RenderConfig& config) const override;
	virtual PainterConfig getPainterConfig(QPainterPath* clip_path = nullptr) const override;
	
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
