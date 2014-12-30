/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012, 2013, 2014 Kai Pastor
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
		Screen            = 0x01, ///< Indicates that the drawing is for the screen.
		                          ///  Can turn on optimizations which result in slightly lower
		                          ///  display quality (e.g. disable antialiasing for texts)
                                  ///  for the benefit of speed.
		ForceMinSize      = 0x02, ///< Forces a minimum size of app. 1 pixel for objects. 
		                          ///  Makes maps look better at small zoom levels without antialiasing.
		HelperSymbols     = 0x04, ///< Activates display of symbols with the "helper symbol" flag.
		Highlighted       = 0x08, ///< Makes the color appear highlighted.
		RequireSpotColor  = 0x10, ///< Skips colors which do not have a spot color definition.
		Tool              = Screen | ForceMinSize | HelperSymbols, ///< The recommended flags for tools.
		NoOptions         = 0x00  ///< No option activated.
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
	
	qreal   scaling;      ///< The scaling.
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
class Renderable
{
protected:
	/** The constructor for new renderables. */
	explicit Renderable(int color_priority);
	
	/** The copy constructor is needed by containers. */
	Renderable(const Renderable& other);
	
public:
	/**
	 * The destructor.
	 */
	virtual ~Renderable();
	
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
	virtual PainterConfig getPainterConfig(QPainterPath* clip_path = nullptr) const = 0;
	
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
	ObjectRenderables(Object* object, QRectF& extent);
	~ObjectRenderables();
	
	inline void insertRenderable(Renderable* r);
	void insertRenderable(Renderable* r, PainterConfig state);
	
	void clear();
	void deleteRenderables();
	void takeRenderables();
	
	/**
	 * Draws all renderables in this container directly with the given color.
	 * May e.g. be used to encode object ids as colors.
	 */
	void draw(const QColor& color, QPainter* painter, const RenderConfig& config) const;
	
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

Q_DECLARE_OPERATORS_FOR_FLAGS(RenderConfig::Options)

inline
bool RenderConfig::testFlag(const RenderConfig::Option flag) const
{
	return options.testFlag(flag);
}



// ### Renderable ###

inline
Renderable::Renderable(int color_priority)
 : color_priority(color_priority)
{
	; // nothing
}

inline
Renderable::Renderable(const Renderable& other)
 : color_priority(other.color_priority)
 , extent(other.extent)
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
	// NOTE: !bounding_box.intersects(extent) should be logical equivalent to the following
	if (extent.right() < rect.x())	return false;
	if (extent.bottom() < rect.y())	return false;
	if (extent.x() > rect.right())	return false;
	if (extent.y() > rect.bottom())	return false;
	return true;
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



// ### MapRenderables ###

inline
bool MapRenderables::empty() const
{
	return std::map<int, ObjectRenderablesMap>::empty();
}



#endif
