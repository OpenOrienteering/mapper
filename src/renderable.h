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


#ifndef _OPENORIENTEERING_RENDERABLE_H_
#define _OPENORIENTEERING_RENDERABLE_H_

#include <QPainter>

#include <map>

#include "map_coord.h"

QT_BEGIN_NAMESPACE
class QPainterPath;
QT_END_NAMESPACE

class Map;
class Symbol;
class PointSymbol;
class LineSymbol;
class AreaSymbol;
class Object;

/// Contains state information about the painter which must be set when rendering a Renderable. Used to order the Renderables by color and minimize state changes
struct RenderStates
{
	enum RenderMode
	{
		Reserved = -1,
		BrushOnly = 0,
		PenOnly = 1
	};
	
	int color_priority;
	RenderMode mode;
	float pen_width;
	QPainterPath* clip_path;
	
	inline bool operator!= (const RenderStates& other) const
	{
		return (color_priority != other.color_priority) ||
		       (mode != other.mode) ||
			   (mode == PenOnly && pen_width != other.pen_width) ||
			   (clip_path != other.clip_path);
	}
	
	inline bool operator< (const RenderStates& other) const
	{
		if (color_priority != other.color_priority)
			return color_priority > other.color_priority;
		else
		{
			// Same priority, decide by clip path
			if (clip_path != other.clip_path)
				return clip_path > other.clip_path;
			else
			{
				// Same clip path, decide by mode
				if ((int)mode != (int)other.mode)
					return (int)mode > (int)other.mode;
				else
				{
					// Same mode, decide by pen width
					return pen_width < other.pen_width;
				}
			}
		}
	}
};

/// Base class to represent small parts of map objects with a simple shape and just one color
class Renderable
{
public:
	Renderable();
	virtual ~Renderable();
	
	inline const QRectF& getExtent() const {return extent;}
	inline Symbol* getSymbol() const {return symbol;}
	
	/// Renders the renderable with the given painter
	virtual void render(QPainter& painter) = 0;
	
	/// Sets the clip path to use (NULL by default)
	//inline void setClipPath(QPainterPath* new_clip_path) {clip_path = new_clip_path;}
	
	/// Creates the render state information which must be set when rendering this renderable
	virtual void getRenderStates(RenderStates& out) = 0;
	
	/// Creates a clone of this renderable
	//virtual Renderable* clone() = 0;
	
	/// Set/Get the object which created this renderable
	inline void setCreator(Object* creator) {this->creator = creator;}
	inline Object* getCreator() const {return creator;}
	
protected:
	
	QRectF extent;
	Symbol* symbol;
	Object* creator;	// this is useful for the deletion of all renderables created by an object
	QPainterPath* clip_path;
};

typedef std::vector<Renderable*> RenderableVector;

/// Container for renderables which sorts them by color priority and renders them
class RenderableContainer
{
public:
	RenderableContainer(Map* map);
	
	void draw(QPainter* painter, QRectF bounding_box, float opacity_factor = 1.0f);
	
	void removeRenderablesOfObject(Object* object, bool mark_area_as_dirty);	// NOTE: does not delete the renderables, just removes them from display
	void insertRenderablesOfObject(Object* object);
	
private:
	typedef std::multimap<RenderStates, Renderable*> Renderables;
	
	Renderables renderables;
	Map* map;
};

class DotRenderable : public Renderable
{
public:
	DotRenderable(PointSymbol* symbol, MapCoordF coord);
	virtual void render(QPainter& painter);
	virtual void getRenderStates(RenderStates& out);
	//virtual Renderable* clone();
};

class CircleRenderable : public Renderable
{
public:
	CircleRenderable(PointSymbol* symbol, MapCoordF coord);
	virtual void render(QPainter& painter);
	virtual void getRenderStates(RenderStates& out);
	//virtual Renderable* clone();
	
protected:
	QRectF rect;
	float line_width;
};

class LineRenderable : public Renderable
{
public:
	LineRenderable(LineSymbol* symbol, const MapCoordVectorF& transformed_coords, const MapCoordVector& coords, bool closed);
	virtual void render(QPainter& painter);
	virtual void getRenderStates(RenderStates& out);
	//virtual Renderable* clone();
	
protected:
	QPainterPath path;
	float line_width;
	Qt::PenCapStyle cap_style;
	Qt::PenJoinStyle join_style;
};

class AreaRenderable : public Renderable
{
public:
	AreaRenderable(AreaSymbol* symbol, const MapCoordVectorF& transformed_coords, const MapCoordVector& coords);
	virtual void render(QPainter& painter);
	virtual void getRenderStates(RenderStates& out);
	//virtual Renderable* clone();
	
protected:
	QPainterPath path;
};

#endif
