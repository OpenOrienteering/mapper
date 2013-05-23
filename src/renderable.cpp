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

#include "renderable.h"

#include <QDebug>
#include <QPainter>
#include <qmath.h>

#include "core/image_transparency_fixup.h"
#include "core/map_color.h"
#include "map.h"
#include "object.h"
#include "symbol.h"
#include "util.h"

/* 
 * The macro MAPPER_OVERPRINTING_CORRECTION allows to select different
 * implementations of spot color overprinting simulation correction towards
 * the appearance of colors in normal (non-spot color) output.
 * 
 *         -1:  Mapper 0.5.0 correction.
 *              Results in undesired brightening when overprinting halftones.
 * 
 *          0:  No correction. Plain multiply spot color composition.
 *              Results in undesired green from 100% blue on 100% yellow.
 * 
 *          1:  Weak correction. Blends the normal output over the 
 *              overprinting simulation with an alpha of 0.125.
 * 
 *          2:  Middle correction. Blends the normal output over the 
 *              overprinting simulation with an alpha of 0.25.
 * 
 *          3:  Strong correction. Blends the normal output over the 
 *              overprinting simulation with an alpha of 0.5.
 * 
 * Options 3 and 2 seem to give the best results. Output from option 3 is quite
 * similar to option -1 (Mapper 0.5.0), but without the undesired brightening.
 * 
 * Options 1..3 work only as long as the color set and the symbol set are
 * defined in a way that the raw overprinting simulation output and the normal
 * output do not differ significantly.
 */
#ifndef MAPPER_OVERPRINTING_CORRECTION
	
	// Default: [new] middle correction
	#define MAPPER_OVERPRINTING_CORRECTION 2
	
#endif


// define DEBUG_OPENORIENTEERING_RENDERABLE to find leaking Renderables

#ifdef DEBUG_OPENORIENTEERING_RENDERABLE

#include <QDebug>

qint64 renderable_instance_count = 0;

void debugRenderableCount(qint64 counter)
{
	if (counter % 1000 == 0)
		qDebug() << "Renderable:" << counter << "instances";
}

#endif


Renderable::Renderable()
{
#ifdef DEBUG_OPENORIENTEERING_RENDERABLE
	debugRenderableCount(++renderable_instance_count);
#endif
}

Renderable::Renderable(const Renderable& other)
{
	extent = other.extent;
	color_priority = other.color_priority;
#ifdef DEBUG_OPENORIENTEERING_RENDERABLE
	debugRenderableCount(++renderable_instance_count);
#endif
}

Renderable::~Renderable()
{
#ifdef DEBUG_OPENORIENTEERING_RENDERABLE
	debugRenderableCount(--renderable_instance_count);
#endif
}


// ### RenderableContainerVector ###

SharedRenderables::~SharedRenderables()
{
	deleteRenderables();
}

void SharedRenderables::deleteRenderables()
{
	for (iterator renderables = begin(); renderables != end(); )
	{
		for (RenderableVector::const_iterator renderable = renderables->second.begin(); renderable != renderables->second.end(); ++renderable)
		{
			delete *renderable;
		}
		renderables->second.clear();
		if (renderables->first.clip_path != NULL)
		{
			iterator it = renderables;
			++renderables;
			erase(it);
		}
		else
			++renderables;
	}
}

void SharedRenderables::compact()
{
	for (iterator renderables = begin(); renderables != end(); )
	{
		if (renderables->second.size() == 0)
		{
			iterator it = renderables;
			++renderables;
			erase(it);
		}
		else
			++renderables;
	}
}


// ### ObjectRenderables ###

ObjectRenderables::ObjectRenderables(Object* object, QRectF& extent)
: object(object),
  extent(extent),
  clip_path(NULL)
{
}

ObjectRenderables::~ObjectRenderables()
{
}

void ObjectRenderables::setClipPath(QPainterPath* path)
{
	clip_path = path;
}

void ObjectRenderables::insertRenderable(Renderable* r, RenderStates& state)
{
	SharedRenderables::Pointer& container(operator[](state.color_priority));
	if (!container)
		container = new SharedRenderables();
	container->operator[](state).push_back(r);
	if (clip_path == NULL)
	{
		if (extent.isValid())
			rectInclude(extent, r->getExtent());
		else
			extent = r->getExtent();
	}
}

void ObjectRenderables::clear()
{
	for (const_iterator color = begin(); color != end(); ++color)
	{
		color->second->clear();
	}
}

void ObjectRenderables::takeRenderables()
{
	for (iterator color = begin(); color != end(); ++color)
	{
		SharedRenderables* new_container = new SharedRenderables();
		
		// Pre-allocate as much space as in the original container
		for (SharedRenderables::const_iterator it = color->second->begin(); it != color->second->end(); ++it)
		{
			new_container->operator[](it->first).reserve(it->second.size());
		}
		color->second = new_container;
	}
}

void ObjectRenderables::deleteRenderables()
{
	for (const_iterator color = begin(); color != end(); ++color)
	{
		color->second->deleteRenderables();
	}
}



// ### MapRenderables ###

MapRenderables::MapRenderables(Map* map) : map(map)
{
}

void MapRenderables::draw(QPainter* painter, QRectF bounding_box, bool force_min_size, float scaling, bool on_screen, bool show_helper_symbols, float opacity_factor, bool highlighted, bool require_spot_color) const
{
	// TODO: improve performance by using some spatial acceleration structure?
	
	Map::ColorVector& colors = map->color_set->colors;
	
	QPainterPath initial_clip = painter->clipPath();
	bool no_initial_clip = initial_clip.isEmpty();
	const QPainterPath* current_clip = NULL;
	
	painter->save();
	const_reverse_iterator end_of_colors = rend();
	const_reverse_iterator color = rbegin();
	while (color != end_of_colors && color->first >= map->getNumColors())
	{
		++color;
	}
	for (; color != end_of_colors; ++color)
	{
		if ( require_spot_color &&
		     (color->first < 0 || map->getColor(color->first)->getSpotColorMethod() == MapColor::UndefinedMethod) )
		{
			continue;
		}
		
		ObjectRenderablesMap::const_iterator end_of_objects = color->second.end();
		for (ObjectRenderablesMap::const_iterator object = color->second.begin(); object != end_of_objects; ++object)
		{
			// Settings check
			Symbol* symbol = object->first->getSymbol();
			if (!show_helper_symbols && symbol->isHelperSymbol())
				continue;
			if (symbol->isHidden())
				continue;
			
			if (!object->first->getExtent().intersects(bounding_box))
				continue;
			
			SharedRenderables::const_iterator it_end = object->second->end();
			for (SharedRenderables::const_iterator it = object->second->begin(); it != it_end; ++it)
			{
				const RenderStates& new_states = it->first;
				const MapColor* color;
				float pen_width = new_states.pen_width;
				
				if (new_states.color_priority > MapColor::Reserved)
				{
					color = colors[new_states.color_priority];
				}
				else if (new_states.color_priority == MapColor::Registration)
				{
					color = Map::getRegistrationColor();
				}
				else
				{
					if (new_states.color_priority == MapColor::CoveringWhite)
						color = Map::getCoveringWhite();
					else if (new_states.color_priority == MapColor::CoveringRed)
						color = Map::getCoveringRed();
					else if (new_states.color_priority == MapColor::Undefined)
						color = Map::getUndefinedColor();
					else if (new_states.color_priority == MapColor::Reserved)
						continue;
					else
					{
						Q_ASSERT(!"Invalid special color!");
						continue; // in release build
					}
					
					// this is not undone here anywhere as it should apply to 
					// all special symbols and these are always painted last
					painter->setRenderHint(QPainter::Antialiasing, true);
					pen_width = new_states.pen_width / scaling;
				}
				
				if (new_states.mode == RenderStates::PenOnly)
				{
					bool pen_too_small = (force_min_size && pen_width * scaling <= 1.0f);
					painter->setPen(QPen(highlighted ? getHighlightedColor(*color) : *color, pen_too_small ? 0 : pen_width));
					
					painter->setBrush(QBrush(Qt::NoBrush));
				}
				else if (new_states.mode == RenderStates::BrushOnly)
				{
					QBrush brush(highlighted ? getHighlightedColor(*color) : *color);
					
					painter->setPen(QPen(Qt::NoPen));
					painter->setBrush(brush);
				}
				/*else if (new_states.mode == RenderStates::PenAndHatch)	// NOTE: does not work well with printing
				{
					bool pen_too_small = (force_min_size && pen_width * scaling <= 1.0f);
					painter->setPen(QPen(highlighted ? getHighlightedColor(color->color) : color->color, pen_too_small ? 0 : pen_width));
					QBrush brush(highlighted ? getHighlightedColor(color->color) : color->color);
					brush.setStyle(Qt::BDiagPattern);
					brush.setTransform(QTransform().scale(1 / scaling, 1 / scaling));
					painter->setBrush(brush);
				}*/
				
				painter->setOpacity(qMin(1.0f, opacity_factor * color->getOpacity()));
				
				if (current_clip != new_states.clip_path)
				{
					if (no_initial_clip)
					{
						if (new_states.clip_path)
							painter->setClipPath(*new_states.clip_path, Qt::ReplaceClip);
						else
							painter->setClipPath(initial_clip, Qt::NoClip);
					}
					// Workaround for Qt::IntersectClip problem
					// with Windows and Mac printers (cf. [tickets:#196]), and 
					// with Linux PDF export (cf. [tickets:#225]).
					else if (!on_screen && new_states.clip_path)
					{
						painter->setClipPath(initial_clip.intersected(*new_states.clip_path), Qt::ReplaceClip);
						if (painter->clipPath().isEmpty())
							continue; // outside of initial clip
					}
					else
					{
						painter->setClipPath(initial_clip, Qt::ReplaceClip);
						if (new_states.clip_path)
							painter->setClipPath(*new_states.clip_path, Qt::IntersectClip);
					}
					current_clip = new_states.clip_path;
				}
					
				RenderableVector::const_iterator r_end = it->second.end();
				for (RenderableVector::const_iterator renderable = it->second.begin(); renderable != r_end; ++renderable)
				{
					// Bounds check
					const QRectF& extent = (*renderable)->getExtent();
					// NOTE: !bounding_box.intersects(extent) should be logical equivalent to the following
					if (extent.right() < bounding_box.x())	continue;
					if (extent.bottom() < bounding_box.y())	continue;
					if (extent.x() > bounding_box.right())	continue;
					if (extent.y() > bounding_box.bottom())	continue;
					
					// Render the renderable
					(*renderable)->render(*painter, bounding_box, force_min_size, scaling, on_screen);
				}
			}
		}
	}
	painter->restore();
}

void MapRenderables::drawOverprintingSimulation(QPainter* painter, QRectF bounding_box, bool force_min_size, float scaling, bool on_screen, bool show_helper_symbols) const
{
	// NOTE: painter must be a QPainter on a QImage of Format_ARGB32_Premultiplied.
	QImage* image = static_cast<QImage*>(painter->device());
	ImageTransparencyFixup image_fixup(image);
	
	QPainter::RenderHints hints = painter->renderHints();
	QTransform t = painter->worldTransform();
	painter->save();
	
	painter->resetTransform();
	painter->setCompositionMode(QPainter::CompositionMode_Multiply); // Alternative: CompositionMode_Darken
	
	QImage separation(image->size(), QImage::Format_ARGB32_Premultiplied);
	
	for (Map::ColorVector::reverse_iterator map_color = map->color_set->colors.rbegin();
	     map_color != map->color_set->colors.rend();
	     map_color++)
	{
		if ((*map_color)->getSpotColorMethod() == MapColor::SpotColor)
		{
			separation.fill((Qt::GlobalColor)Qt::transparent);
			
			// Collect all halftones and knockouts of a single color
			QPainter p(&separation);
			p.setRenderHints(hints);
			p.setWorldTransform(t, false);
			drawColorSeparation(&p, bounding_box, force_min_size, scaling, on_screen, show_helper_symbols, *map_color, true);
			p.end();
			
			// Add this separation to the composition with multiplication.
			painter->setCompositionMode(QPainter::CompositionMode_Multiply);
			painter->drawImage(0, 0, separation);
			image_fixup();
			
#if MAPPER_OVERPRINTING_CORRECTION == -1
			// Add some opacity to the multiplication, but not for black,
			// since halftones (i.e. grey) might unduly lighten the composition.
			if (static_cast<QRgb>(**map_color) != 0xff000000)
			{
				// FIXME: Implement this for Format_ARGB32_Premultiplied,
				//        if efficiently possible.
				QImage copy = separation.convertToFormat(QImage::Format_ARGB32);
				QRgb* dest = (QRgb*)copy.bits();
				const QRgb* dest_end = dest + copy.byteCount() / sizeof(QRgb);
				for (QRgb* px = dest; px < dest_end; ++px)
				{
					const unsigned int alpha = qAlpha(*px) * ((255-qGray(*px)) << 16) & 0xff000000;
					*px = alpha | (*px & 0xffffff);
				}
				painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
				painter->drawImage(0, 0, copy);
			}
#endif
		}
	}
	
	painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
	
#if MAPPER_OVERPRINTING_CORRECTION > 0
	separation.fill((Qt::GlobalColor)Qt::transparent);
	QPainter p(&separation);
	p.setRenderHints(hints);
	p.setWorldTransform(t, false);
	draw(&p, bounding_box, force_min_size, scaling, on_screen, show_helper_symbols, 1.0f, false, true);
	p.end();
	QRgb* dest = (QRgb*)separation.bits();
	const QRgb* dest_end = dest + separation.byteCount() / sizeof(QRgb);
	for (QRgb* px = dest; px < dest_end; ++px)
	{
		/* Each pixel is a premultipled RGBA, so the alpha value is adjusted
		 * by applying the same factor to all 4 channels (bytes).
		 * Implemented by bitwise operators for efficiency.
		 */
#if MAPPER_OVERPRINTING_CORRECTION == 1
		*px = (*px >> 3) & 0x1f1f1f1f;
#elif MAPPER_OVERPRINTING_CORRECTION == 2
		*px = (*px >> 2) & 0x3f3f3f3f;
#else /* MAPPER_OVERPRINTING_CORRECTION == 3 or stronger */
		*px = (*px >> 1) & 0x7f7f7f7f;
#endif
	}
	painter->drawImage(0, 0, separation);
#endif
	
	if (on_screen)
	{
		static MapColor reserved_color(MapColor::Reserved);
		drawColorSeparation(painter, bounding_box, force_min_size, scaling, on_screen, show_helper_symbols, &reserved_color, true);
	}
	
	painter->restore();
}

void MapRenderables::drawColorSeparation(QPainter* painter, QRectF bounding_box, bool force_min_size, float scaling, bool on_screen, bool show_helper_symbols, const MapColor* separation, bool use_color) const
{
	painter->save();
	
	const QPainterPath initial_clip(painter->clipPath());
	bool no_initial_clip = initial_clip.isEmpty();
	const QPainterPath* current_clip = NULL;
	
	// As soon as the spot color is actually used for drawing (i.e. drawing_started = true),
	// we need to take care of knockouts.
	bool drawing_started = false;
	
	// For each pair of color priority and its renderables collection...
	const_reverse_iterator end_of_colors = rend();
	const_reverse_iterator color = rbegin();
	while (color != end_of_colors && color->first >= map->getNumColors())
	{
		++color;
	}
	for (; color != end_of_colors; ++color)
	{
		SpotColorComponent drawing_color(const_cast<const Map*>(map)->getColor(color->first), 1.0f);
		
		// Check whether the current color [priority] applies to the current separation.
		if (color->first > MapColor::Reserved)
		{
			if (separation->getPriority() == MapColor::Reserved)
			{
				// Don't process regular colors for the "Reserved" separation.
				continue;
			}
			
			switch (drawing_color.spot_color->getSpotColorMethod())
			{
				case MapColor::UndefinedMethod:
					continue;
				
				case MapColor::SpotColor:
					if (drawing_color.spot_color == separation)
					{
						; // okay
					}
					else if (drawing_started && drawing_color.spot_color->getKnockout())
					{
						drawing_color.factor = 0.0f;
					}
					else
					{
						continue;
					}
					break;
				
				case MapColor::CustomColor:
				{
					// First, check if the renderables draw color to this separation
					// TODO: Use an efficient data structure to avoid reiterating each time a separation is drawn
					const SpotColorComponents& components = drawing_color.spot_color->getComponents();
					for ( SpotColorComponents::const_iterator component = components.begin(), c_end = components.end();
						component != c_end;
						++component )
					{
						if (component->spot_color == separation)
						{
							// The renderables do draw the current spot color
							drawing_color = *component;
							break;
						}
					}
					if (drawing_color.spot_color != separation)
					{
						// If the renderables do not explicitly draw color to this separation,
						// check if they need a knockout.
						if (drawing_started && drawing_color.spot_color->getKnockout())
						{
							drawing_color = SpotColorComponent(separation, 0.0f);
						}
						else
						{
							continue;
						}
					}
					break;
				}
				
				default:
					Q_ASSERT(false); // in development builds
					continue;        // in release build
			}
		}
		else if (separation->getPriority() == MapColor::Reserved)
		{
			if (color->first == MapColor::Registration)
				continue; // treated per spot color
			else if (color->first == MapColor::Reserved)
				continue; // never drawn
			else if (drawing_color.spot_color == NULL)
			{
				Q_ASSERT(!"Invalid reserved color!");                // in development build
				drawing_color.spot_color = Map::getUndefinedColor(); // in release build
			}
			painter->setRenderHint(QPainter::Antialiasing, true);
		}
		else if (color->first == MapColor::Registration)
		{
			// Draw Registration Black as fulltone of regular spot color
			drawing_color.spot_color = separation;
		}
		else
		{
			// Don't draw reserved color in regular separation.
			continue;
		}
		
		// For each pair of object and its renderables [states] for a particular map color...
		ObjectRenderablesMap::const_iterator end_of_objects = color->second.end();
		for (ObjectRenderablesMap::const_iterator object = color->second.begin(); object != end_of_objects; ++object)
		{
			// Check whether the symbol and object is to be drawn at all.
			Symbol* symbol = object->first->getSymbol();
			if (!show_helper_symbols && symbol->isHelperSymbol())
				continue;
			if (symbol->isHidden())
				continue;
			
			if (!object->first->getExtent().intersects(bounding_box))
				continue;
			
			// For each pair of common rendering attributes and collection of renderables...
			SharedRenderables::const_iterator it_end = object->second->end();
			for (SharedRenderables::const_iterator it = object->second->begin(); it != it_end; ++it)
			{
				const RenderStates& new_states = it->first;
				
				float pen_width = new_states.pen_width;
				if (separation->getPriority() == MapColor::Reserved)
				{
					pen_width = new_states.pen_width / scaling;
				}
				
				QColor color = *drawing_color.spot_color;
				if (drawing_color.factor < 0.0005f)
				{
					color = Qt::white;
				}
				else if (use_color)
				{
					qreal c, m, y, k;
					color.getCmykF(&c, &m, &y, &k);
					color.setCmykF(c*drawing_color.factor, m*drawing_color.factor, y*drawing_color.factor, k*drawing_color.factor,1.0f);
				}
				else
				{
					color.setCmykF(0.0f, 0.0f, 0.0f, drawing_color.factor, 1.0f);
				}
				
				if (new_states.mode == RenderStates::PenOnly)
				{
					bool pen_too_small = (force_min_size && pen_width * scaling <= 1.0f);
					painter->setPen(QPen(color, pen_too_small ? 0 : pen_width));
					painter->setBrush(QBrush(Qt::NoBrush));
				}
				else if (new_states.mode == RenderStates::BrushOnly)
				{
					painter->setPen(QPen(Qt::NoPen));
					painter->setBrush(QBrush(color));
				}
				
				if (current_clip != new_states.clip_path)
				{
					if (no_initial_clip)
					{
						if (new_states.clip_path)
							painter->setClipPath(*new_states.clip_path, Qt::ReplaceClip);
						else
							painter->setClipPath(initial_clip, Qt::NoClip);
					}
					// Workaround for Qt::IntersectClip problem
					// with Windows and Mac printers (cf. [tickets:#196]), and 
					// with Linux PDF export (cf. [tickets:#225]).
					else if (!on_screen && new_states.clip_path)
					{
						painter->setClipPath(initial_clip.intersected(*new_states.clip_path), Qt::ReplaceClip);
						if (painter->clipPath().isEmpty())
							continue; // outside of initial clip
					}
					else
					{
						painter->setClipPath(initial_clip, Qt::ReplaceClip);
						if (new_states.clip_path)
							painter->setClipPath(*new_states.clip_path, Qt::IntersectClip);
					}
					current_clip = new_states.clip_path;
				}
				
				bool drawing = (drawing_color.factor > 0.0005f);
				
				// For each renderable that uses the current painter configuration...
				RenderableVector::const_iterator r_end = it->second.end();
				for (RenderableVector::const_iterator renderable = it->second.begin(); renderable != r_end; ++renderable)
				{
					// Bounds check
					const QRectF& extent = (*renderable)->getExtent();
					if (extent.right() < bounding_box.left()) continue;
					if (extent.bottom() < bounding_box.top()) continue;
					if (extent.left() > bounding_box.right()) continue;
					if (extent.top() > bounding_box.bottom()) continue;
					
					// Render the renderable
					drawing_started |= drawing;
					(*renderable)->render(*painter, bounding_box, force_min_size, scaling, on_screen);
				} // each renderable
				
			} // each common render attributes
			
		} // each object
		
	} // each map color
	
	painter->restore();
}

void MapRenderables::insertRenderablesOfObject(const Object* object)
{
	ObjectRenderables::const_iterator end_of_colors = object->renderables().end();
	ObjectRenderables::const_iterator color = object->renderables().begin();
	for (; color != end_of_colors; ++color)
	{
		operator[](color->first)[object] = color->second;
	}
}

void MapRenderables::removeRenderablesOfObject(const Object* object, bool mark_area_as_dirty)
{
	const_iterator end_of_colors = end();
	for (iterator color = begin(); color != end_of_colors; ++color)
	{
		ObjectRenderablesMap::const_iterator end_of_objects = color->second.end();
		for (ObjectRenderablesMap::iterator obj = color->second.begin(); obj != end_of_objects; ++obj)
		{
			if (obj->first != object)
				continue;
			
			if (mark_area_as_dirty)
			{
				for (SharedRenderables::const_iterator renderables = obj->second->begin(); renderables != obj->second->end(); ++renderables)
				{
					for (RenderableVector::const_iterator renderable = renderables->second.begin(); renderable != renderables->second.end(); ++renderable)
					{
						map->setObjectAreaDirty((*renderable)->getExtent());
					}
				}
			}
			color->second.erase(obj);
			
			break;
		}
	}
}

void MapRenderables::clear(bool set_area_dirty)
{
	if (set_area_dirty)
	{
		const_iterator end_of_colors = end();
		for (const_iterator color = begin(); color != end_of_colors; ++color)
		{
			ObjectRenderablesMap::const_iterator end_of_objects = color->second.end();
			for (ObjectRenderablesMap::const_iterator object = color->second.begin(); object != end_of_objects; ++object)
			{
				for (SharedRenderables::const_iterator renderables = object->second->begin(); renderables != object->second->end(); ++renderables)
				{
					for (RenderableVector::const_iterator renderable = renderables->second.begin(); renderable != renderables->second.end(); ++renderable)
					{
						map->setObjectAreaDirty((*renderable)->getExtent());
					}
				}
			}
		}
	}
	std::map<int, ObjectRenderablesMap>::clear();
}

QColor MapRenderables::getHighlightedColor(const QColor& original)
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
