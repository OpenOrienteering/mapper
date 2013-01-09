/*
 *    Copyright 2012, 2013 Thomas Schöps, Kai Pastor
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

#include <map>
#include <vector>

#include <QRectF>
#include <QSharedData>
#include <QExplicitlySharedDataPointer>

class QColor;
class QPainter;
class QPainterPath;

class Map;
class MapColor;
class Object;
class RenderStates;

/**
 * A Renderable is a graphical map item with a simple shape and a single color.
 * 
 * This is the abstract base class.
 */
class Renderable
{
public:
	Renderable();
	Renderable(const Renderable& other);
	virtual ~Renderable();
	
	inline const QRectF& getExtent() const {return extent;}
	
	/// Renders the renderable with the given painter
	virtual void render(QPainter& painter, bool force_min_size, float scaling, bool on_screen) const = 0;
	
	/// Creates the render state information which must be set when rendering this renderable
	virtual void getRenderStates(RenderStates& out) const = 0;
	
protected:
	QRectF extent;
	int color_priority;
};

/** 
 * RenderStates contains state information about the painter 
 * which must be set when rendering a Renderable. 
 * 
 * Used to group renderables by common render attributes.
 */
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
	const QPainterPath* clip_path;
	
	RenderStates(const Renderable* r, const QPainterPath* path = NULL) : clip_path(path) { r->getRenderStates(*this); };
	
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

/**
 * A low-level container for renderables.
 */
typedef std::vector<Renderable*> RenderableVector;

/**
 * A shared high-level container for renderables
 * grouped by common render attributes.
 * 
 * This shared container can be used in different collections. When the last
 * reference to this container is dropped, it will delete the renderables.
 */
class SharedRenderables : public QSharedData, public std::map< RenderStates, RenderableVector >
{
public:
	typedef QExplicitlySharedDataPointer<SharedRenderables> Pointer;
	~SharedRenderables();
	void deleteRenderables();
	void compact(); // release memory which is occupied by unused RenderStates, FIXME: maybe call this regularly...
};

/**
 * A high-level container for all renderables of a single object, 
 * grouped by color priority and common render attributes.
 */
class ObjectRenderables : protected std::map<int, SharedRenderables::Pointer>
{
friend class MapRenderables;
public:
	ObjectRenderables(Object* object, QRectF& extent);
	~ObjectRenderables();
	
	inline void insertRenderable(Renderable* r)
	{
		RenderStates state(r, clip_path);
		insertRenderable(r, state);
	}
	void insertRenderable(Renderable* r, RenderStates& state);
	
	void clear();
	void deleteRenderables();
	void takeRenderables();
	
	void setClipPath(QPainterPath* path);
	inline QPainterPath* getClipPath() const {return clip_path;}
	
	const QRectF& getExtent() const { return extent; }
	
private:
	Object* const object;
	QRectF& extent;
	QPainterPath* clip_path; // no memory management here!
};

/**
 * A low-level container for renderables of multiple objects
 * grouped by object and common render attributes.
 * 
 * This container uses a smart pointer to the renderable collection
 * of each single object.
 */
typedef std::map<const Object*, SharedRenderables::Pointer> ObjectRenderablesMap;

/** 
 * A high-level container for renderables of multiple objects
 * grouped by color priority, object and common render attributes.
 * 
 * This container is able to draw the renderables.
 */
class MapRenderables : protected std::map<int, ObjectRenderablesMap>
{
public:
	MapRenderables(Map* map);
	
	void draw(QPainter* painter, QRectF bounding_box, bool force_min_size, float scaling, bool on_screen, bool show_helper_symbols, float opacity_factor = 1.0f, bool highlighted = false) const;
	void drawOverprintingSimulation(QPainter* painter, QRectF bounding_box, bool force_min_size, float scaling, bool on_screen, bool show_helper_symbols, float opacity_factor = 1.0f, bool highlighted = false) const;
	void drawColorSeparation(QPainter* painter, MapColor* separation, QRectF bounding_box, bool force_min_size, float scaling, bool on_screen, bool show_helper_symbols, float opacity_factor = 1.0f, bool highlighted = false) const;
	
	void insertRenderablesOfObject(const Object* object);
	void removeRenderablesOfObject(const Object* object, bool mark_area_as_dirty);	// NOTE: does not delete the renderables, just removes them from display
	
	void clear(bool set_area_dirty = false);
	inline bool isEmpty() const {return empty();}
	
	static QColor getHighlightedColor(const QColor& original);
	
private:
	Map* const map;
};

#endif
