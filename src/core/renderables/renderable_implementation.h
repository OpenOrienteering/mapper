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


#ifndef OPENORIENTEERING_RENDERABLE_IMPLENTATION_H
#define OPENORIENTEERING_RENDERABLE_IMPLENTATION_H

#include <Qt>
#include <QtGlobal>
#include <QPainterPath>
#include <QPointF>
#include <QRectF>

#include "renderable.h"

class QPainter;
class QPointF;

class AreaSymbol;
class LineSymbol;
class MapColor;
class MapCoordF;
class PathPartVector;
class PointSymbol;
class TextObject;
class TextSymbol;
class VirtualPath;


/** Renderable for displaying a filled dot. */
class DotRenderable : public Renderable
{
public:
	DotRenderable(const PointSymbol* symbol, MapCoordF coord);
	void render(QPainter& painter, const RenderConfig& config) const override;
	PainterConfig getPainterConfig(const QPainterPath* clip_path = nullptr) const override;
};

/** Renderable for displaying a circle. */
class CircleRenderable : public Renderable
{
public:
	CircleRenderable(const PointSymbol* symbol, MapCoordF coord);
	void render(QPainter& painter, const RenderConfig& config) const override;
	PainterConfig getPainterConfig(const QPainterPath* clip_path = nullptr) const override;
	
protected:
	const qreal line_width;
	QRectF rect;
};

/** Renderable for displaying a line. */
class LineRenderable : public Renderable
{
public:
	LineRenderable(const LineSymbol* symbol, const VirtualPath& virtual_path, bool closed);
	LineRenderable(const LineSymbol* symbol, QPointF first, QPointF second);
	void render(QPainter& painter, const RenderConfig& config) const override;
	PainterConfig getPainterConfig(const QPainterPath* clip_path = nullptr) const override;
	
protected:
	void extentIncludeCap(quint32 i, qreal half_line_width, bool end_cap, const LineSymbol* symbol, const VirtualPath& path);
	
	void extentIncludeJoin(quint32 i, qreal half_line_width, const LineSymbol* symbol, const VirtualPath& path);
	
	const qreal line_width;
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
	void render(QPainter& painter, const RenderConfig& config) const override;
	PainterConfig getPainterConfig(const QPainterPath* clip_path = nullptr) const override;
	
	inline const QPainterPath* painterPath() const;
	
protected:
	void addSubpath(const VirtualPath& virtual_path);
	
	QPainterPath path;
};

/** Renderable for displaying text. */
class TextRenderable : public Renderable
{
public:
	TextRenderable(const TextSymbol* symbol, const TextObject* text_object, const MapColor* color, double anchor_x, double anchor_y);
	PainterConfig getPainterConfig(const QPainterPath* clip_path = nullptr) const override;
	void render(QPainter& painter, const RenderConfig& config) const override;
	
protected:
	void renderCommon(QPainter& painter, const RenderConfig& config) const;
	
	QPainterPath path;
	qreal anchor_x;
	qreal anchor_y;
	qreal rotation;
	qreal scale_factor;
};

/** Renderable for displaying framing line for text. */
class TextFramingRenderable : public TextRenderable
{
public:
	TextFramingRenderable(const TextSymbol* symbol, const TextObject* text_object, const MapColor* color, double anchor_x, double anchor_y);
	PainterConfig getPainterConfig(const QPainterPath* clip_path = nullptr) const override;
	void render(QPainter& painter, const RenderConfig& config) const override;
	
protected:
	qreal framing_line_width;
};



// ### AreaRenderable inline code ###

const QPainterPath* AreaRenderable::painterPath() const
{
	return &path;
}



#endif
