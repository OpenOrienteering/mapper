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

#ifndef OPENORIENTEERING_RENDERABLE_H
#define OPENORIENTEERING_RENDERABLE_H

#include <map>
#include <vector>

#include <QtGlobal>
#include <QFlags>
#include <QRectF>
#include <QSharedData>
#include <QExplicitlySharedDataPointer>

#include "core/map_color.h"

class QColor;
class QPainter;
class QPainterPath;
// IWYU pragma: no_forward_declare QRectF

namespace OpenOrienteering {

class Map;
class Object;
class PainterConfig;


/**
 * This class contains rendering configuration values.
 * 
 * A reference to an object of this class replaces what used to be part of the
 * parameter list of various draw()/render() methods. With that old approach,
 * each new rendering configuration option required changes in many signatures
 * and function calls. In addition, the boolean options were not verbose at all.
 * 
 * Objects are meant to be initialized by initializer lists (C++11).
 */
class RenderConfig
{
public:
	/**
	 * Flags indicating particular rendering configuration options.
	 */
	enum Option
	{
		Screen              = 1<<0, ///< Indicates that the drawing is for the screen.
		                            ///  Can turn on optimizations which result in slightly
		                            ///  lower display quality (e.g. disable antialiasing
		                            ///  for texts) for the benefit of speed.
		DisableAntialiasing = 1<<1, ///< Forces disabling of Antialiasing.
		ForceMinSize        = 1<<2, ///< Forces a minimum size of app. 1 pixel for objects. 
		                            ///  Makes maps look better at small zoom levels without antialiasing.
		HelperSymbols       = 1<<3, ///< Activates display of symbols with the "helper symbol" flag.
		Highlighted         = 1<<4, ///< Makes the color appear highlighted.
		RequireSpotColor    = 1<<5, ///< Skips colors which do not have a spot color definition.
		Tool                = Screen | ForceMinSize | HelperSymbols, ///< The recommended flags for tools.
		NoOptions           = 0     ///< No option activated.
	};
	
	/**
	 * \class RenderConfig::Options
	 * 
	 * A combination of flags for rendering configuration options.
	 * 
	 * \see QFlags::testFlag(), RenderConfig::Option
	 */
	Q_DECLARE_FLAGS(Options, Option)
	
	const Map& map;       ///< The map.
	
	QRectF  bounding_box; ///< The bounding box of the area to be drawn.
	                      ///  Given in map coordinates.
	
	qreal   scaling;      ///< The scaling expressed as pixels per mm.
	                      ///  Used to calculate the final object sizes when
                          ///  ForceMinSize is set.
    
	Options options;      ///< The rendering options.
	
	qreal   opacity;      ///< The opacity.
	
	/**
	 * A convenience method for testing flags in the options value.
	 * 
	 * \see QFlags::testFlag()
	 */
	bool testFlag(const Option flag) const;
};



/**
 * A Renderable is a graphical item with a simple shape and a single color.
 * 
 * This is the abstract base class. Inheriting classes must implement the
 * abstract methods, and they must set the extent during construction.
 */
class Renderable  // clazy:exclude=copyable-polymorphic
{
protected:
	/** The constructor for new renderables. */
	explicit Renderable(const MapColor* color);
	
public:
	Renderable(const Renderable&) = delete;
	Renderable(Renderable&&) = delete;
	
	/**
	 * The destructor.
	 */
	virtual ~Renderable();
	
	Renderable& operator=(const Renderable&) = delete;
	Renderable& operator=(Renderable&&) = delete;
	
	/**
	 * Returns the extent (bounding box).
	 */
	const QRectF& getExtent() const;
	
	/**
	 * Tests whether the renderable's extent intersects the given rect.
	 */
	bool intersects(const QRectF& rect) const;
	
	/**
	 * Returns the painter configuration information.
	 * 
	 * This configuration must be set when rendering this renderable.
	 */
	virtual PainterConfig getPainterConfig(const QPainterPath* clip_path = nullptr) const = 0;
	
	/**
	 * Renders the renderable with the given painter and rendering configuration.
	 */
	virtual void render(QPainter& painter, const RenderConfig& config) const = 0;
	
protected:
	/** The color priority is a major attribute and cannot be modified. */
	const int color_priority;
	
	/** The extent must be set by inheriting classes. */
	QRectF extent;
};



/** 
 * PainterConfig contains painter configuration information.
 * 
 * When painting a renderable item, the QPainter shall be configured according
 * to this information.
 * 
 * A PainterConfig is an immutable values, constructed with initializer lists.
 */
class PainterConfig
{
public:
	enum PainterMode
	{
		BrushOnly = 0,  ///< Render using the brush only.
		PenOnly   = 1,  ///< Render using the pen only.
		Reserved  = -1	///< Not used.
	};
	
	const int color_priority;       ///< The color priority which determines rendering order
	const PainterMode mode;         ///< The mode of painting
	const qreal pen_width;          ///< The width of the pen
	const QPainterPath* clip_path;  ///< A clip_path which may be shared by several Renderables
	
	/**
	 * Activates the configuration on the given painter.
	 * 
	 * If this method returns false, the corresponding renderables shall not be drawn.
	 * 
	 * @param painter      The painter to be configured.
	 * @param current_clip A pointer which will be set to the address of the current clip,
	 *                     in order to avoid switching the clip area unneccessarily.
	 * @param config       The rendering configurations.
	 * @param color        The QColor to be used for the pen or brush.
	 * @param initial_clip The clip which was set initially for this painter.
	 * @return True if the configuration was activated, false if the corresponding renderables shall not be drawn.
	 */
	bool activate(QPainter* painter, const QPainterPath*& current_clip, const RenderConfig& config, const QColor& color, const QPainterPath& initial_clip) const;
	
	friend bool operator==(const PainterConfig& lhs, const PainterConfig& rhs);
	friend bool operator<(const PainterConfig& lhs, const PainterConfig& rhs);
};

/**
 * Returns true if the configurations are equal.
 */
bool operator==(const PainterConfig& lhs, const PainterConfig& rhs);

/**
 * Returns true if the configurations are not equal.
 */
bool operator!=(const PainterConfig& lhs, const PainterConfig& rhs);

/**
 * Defines an order over values which are not equal.
 */
bool operator<(const PainterConfig& lhs, const PainterConfig& rhs);



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
class SharedRenderables : public QSharedData, public std::map< PainterConfig, RenderableVector >
{
public:
	typedef QExplicitlySharedDataPointer<SharedRenderables> Pointer;
	SharedRenderables() = default;
	SharedRenderables(const SharedRenderables&) = delete;
	SharedRenderables& operator=(const SharedRenderables&) = delete;
	~SharedRenderables();
	void deleteRenderables();
	void compact(); // release memory which is occupied by unused PainterConfig, FIXME: maybe call this regularly...
};



/**
 * A high-level container for all renderables of a single object, 
 * grouped by color priority and common render attributes.
 */
class ObjectRenderables : protected std::map<int, SharedRenderables::Pointer>
{
friend class MapRenderables;
public:
	ObjectRenderables(Object& object);
	ObjectRenderables(const ObjectRenderables&) = delete;
	ObjectRenderables& operator=(const ObjectRenderables&) = delete;
	~ObjectRenderables();
	
	inline void insertRenderable(Renderable* r);
	void insertRenderable(Renderable* r, const PainterConfig& state);
	
	void clear();
	void deleteRenderables();
	void takeRenderables();
	
	/**
	 * Draws all renderables matching the given map color with the given color.
	 * 
	 * If map_color is -1, this functions draws all renderables of color which
	 * are not in the list of map colors (i.e. objects with undefined symbol).
	 * Used by FillTool to encode object IDs as colors.
	 */
	void draw(int map_color, const QColor& color, QPainter* painter, const RenderConfig& config) const;
	
	void setClipPath(const QPainterPath* path);
	const QPainterPath* getClipPath() const;
	
	const QRectF& getExtent() const;
	
private:
	QRectF& extent;
	const QPainterPath* clip_path = nullptr; // no memory management here!
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
	/**
	 * An Object deleter which takes care of removing the renderables of the object.
	 * 
	 * Synopsis:
	 *   std::unique_ptr<Object, MapRenderables::ObjectDeleter> object { nullptr, { renderables } };
	 */
	class ObjectDeleter
	{
	public:
		MapRenderables& renderables;
		void operator()(Object* object) const;
	};
	
	
	MapRenderables(Map* map);
	
	/**
	 * Draws the renderables normally (one opaque over the other).
	 * 
	 * @param painter The QPainter used for drawing.
	 * @param config  The rendering configuration
	 */
	void draw(QPainter* painter, const RenderConfig& config) const;
	
	/**
	 * Draws the renderables in a spot color overprinting simulation.
	 * 
	 * @param painter Must be a QPainter on a QImage of Format_ARGB32_Premultiplied.
	 * @param config  The rendering configuration
	 */
	void drawOverprintingSimulation(QPainter* painter, const RenderConfig& config) const;
	
	/**
	 * Draws only the renderables which belong to a particular spot color.
	 * 
	 * Separations are normally drawn in levels of gray where black means
	 * full tone of the spot color. The parameter use_color can be used to
	 * draw in the actual spot color instead.
	 * 
	 * @param painter The QPainter used for drawing.
	 * @param config  The rendering configuration
	 * @param separation The spot color to draw the separation for.
	 * @param use_color  If true, forces the separation to be drawn in its actual color.
	 */
	void drawColorSeparation(QPainter* painter, const RenderConfig& config,
		const MapColor* separation, bool use_color = false) const;
	
	void insertRenderablesOfObject(const Object* object);
	
	/* NOTE: does not delete the renderables, just removes them from display */
	void removeRenderablesOfObject(const Object* object, bool mark_area_as_dirty);
	
	void clear(bool mark_area_as_dirty = false);
	
	inline bool empty() const;
	
private:
	Map* const map;
};



// ### RenderConfig ###

inline
bool RenderConfig::testFlag(const RenderConfig::Option flag) const
{
	return options.testFlag(flag);
}



// ### Renderable ###

inline
Renderable::Renderable(const MapColor* color)
 : color_priority(color ? color->getPriority() : MapColor::Reserved)
{
	; // nothing
}

inline
const QRectF&Renderable::getExtent() const
{
	return extent;
}

inline
bool Renderable::intersects(const QRectF& rect) const
{
	return extent.intersects(rect);
}



// ### PainterConfig ###

inline
bool operator==(const PainterConfig& lhs, const PainterConfig& rhs)
{
	return (lhs.color_priority == rhs.color_priority) &&
	       (lhs.mode == rhs.mode) &&
	       (lhs.pen_width == rhs.pen_width || lhs.mode == PainterConfig::BrushOnly) &&
	       (lhs.clip_path != rhs.clip_path);
}

inline
bool operator!=(const PainterConfig& lhs, const PainterConfig& rhs)
{
	return !(lhs == rhs);
}

inline
bool operator<(const PainterConfig& lhs, const PainterConfig& rhs)
{
	// First, decide by priority
	if (lhs.color_priority != rhs.color_priority)
		return lhs.color_priority > rhs.color_priority;
	
	// Same priority, decide by clip path
	else if (lhs.clip_path != rhs.clip_path)
		return lhs.clip_path > rhs.clip_path;
	
	// Same clip path, decide by mode
	else if ((int)lhs.mode != (int)rhs.mode)
		return (int)lhs.mode > (int)rhs.mode;
	
	// Same mode, decide by pen width
	else
		return lhs.pen_width < rhs.pen_width;
}



// ### ObjectRenderables ###

inline
void ObjectRenderables::insertRenderable(Renderable* r)
{
	insertRenderable(r, r->getPainterConfig(clip_path));
}

inline
const QPainterPath* ObjectRenderables::getClipPath() const
{
	return clip_path;
}

inline
const QRectF &ObjectRenderables::getExtent() const
{
	return extent;
}



// ### MapRenderables ###

inline
bool MapRenderables::empty() const
{
	return std::map<int, ObjectRenderablesMap>::empty();
}


}  // namespace OpenOrienteering


Q_DECLARE_OPERATORS_FOR_FLAGS(OpenOrienteering::RenderConfig::Options)


#endif
