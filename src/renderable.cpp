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

void MapRenderables::draw(QPainter* painter, QRectF bounding_box, bool force_min_size, float scaling, bool on_screen, bool show_helper_symbols, float opacity_factor, bool highlighted) const
{
	// TODO: improve performance by using some spatial acceleration structure?
	
	Map::ColorVector& colors = map->color_set->colors;
	
	QPainterPath initial_clip = painter->clipPath();
	bool no_initial_clip = initial_clip.isEmpty();
	const QPainterPath* current_clip = NULL;
	
	painter->save();
	const_reverse_iterator end_of_colors = rend();
	for (const_reverse_iterator color = rbegin(); color != end_of_colors; ++color)
	{
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
				MapColor* color;
				float pen_width;
				
				if (new_states.color_priority > MapColor::Reserved)
				{
					pen_width = new_states.pen_width;
					color = colors[new_states.color_priority];
				}
				else
				{
					painter->setRenderHint(QPainter::Antialiasing, true);	// this is not undone here anywhere as it should apply to all special symbols and these are always painted last
					pen_width = new_states.pen_width / scaling;
					
					if (new_states.color_priority == MapColor::CoveringWhite)
						color = Map::getCoveringWhite();
					else if (new_states.color_priority == MapColor::CoveringRed)
						color = Map::getCoveringRed();
					else if (new_states.color_priority == MapColor::Undefined)
						color = Map::getUndefinedColor();
					else if (new_states.color_priority == MapColor::Reserved)
						continue;
					else
						assert(!"Invalid special color!");
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

void MapRenderables::drawOverprintingSimulation(QPainter* painter, QRectF bounding_box, bool force_min_size, float scaling, bool on_screen, bool show_helper_symbols, float opacity_factor, bool highlighted) const
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
			drawColorSeparation(&p, *map_color, bounding_box, force_min_size, scaling, on_screen, true);
			p.end();
			
			// Add this separation to the composition
			painter->drawImage(0, 0, separation);
			image_fixup();
		}
	}
	
	painter->setWorldTransform(t, false);
	painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
	drawColorSeparation(painter, Map::getCoveringWhite(), bounding_box, force_min_size, scaling, on_screen, true);
	drawColorSeparation(painter, Map::getCoveringRed(), bounding_box, force_min_size, scaling, on_screen, true);
	
	painter->restore();
}

void MapRenderables::drawColorSeparation(QPainter* painter, MapColor* separation, QRectF bounding_box, bool force_min_size, float scaling, bool on_screen, bool show_helper_symbols, float opacity_factor, bool highlighted) const
{
	Map::ColorVector& colors = map->color_set->colors;
	
	QPainterPath initial_clip = painter->clipPath();
	bool no_initial_clip = initial_clip.isEmpty();
	const QPainterPath* current_clip = NULL;
	
	// As soon as the spot color is actually used for drawing (i.e. drawing_started = true),
	// we need to take care of out-of-sequence map colors.
	bool drawing_started = false;
	
	painter->save();
	
	const_reverse_iterator end_of_colors = rend();
	for (const_reverse_iterator color = rbegin(); color != end_of_colors; ++color)
	{
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
				MapColor* renderables_color;
				float pen_width;
				
				if (new_states.color_priority > MapColor::Reserved)
				{
					pen_width = new_states.pen_width;
					renderables_color = colors[new_states.color_priority];
				}
				else
				{
					painter->setRenderHint(QPainter::Antialiasing, true);	// this is not undone here anywhere as it should apply to all special symbols and these are always painted last
					pen_width = new_states.pen_width / scaling;
					
					if (new_states.color_priority == MapColor::CoveringWhite)
						renderables_color = Map::getCoveringWhite();
					else if (new_states.color_priority == MapColor::CoveringRed)
						renderables_color = Map::getCoveringRed();
					else if (new_states.color_priority == MapColor::Undefined)
						renderables_color = Map::getUndefinedColor();
					else if (new_states.color_priority == MapColor::Reserved)
						continue;
					else
						assert(!"Invalid special color!");
				}
				
				SpotColorComponent drawing_color(Map::getUndefinedColor(), 0.0f);
				switch(renderables_color->getSpotColorMethod())
				{
					case MapColor::SpotColor:
					{
						if (renderables_color == separation) 
						{
							// draw with spot color
							drawing_color = SpotColorComponent(renderables_color, 1.0f);
							if (!drawing_started)
								drawing_started = true;
						}
						else if (renderables_color->getKnockout() && renderables_color->getPriority() < separation->getPriority())
							// explicit knockout
							drawing_color = SpotColorComponent(separation, 0.0f);
						break;
					}
					case MapColor::CustomColor:
					{
						// First, check if the renderables draw color to this separation
						Q_FOREACH(SpotColorComponent component, renderables_color->getComponents())
						{
							if (component.spot_color == separation)
							{
								// The renderables do draw the current spot color
								drawing_color = component;
								if (!drawing_started)
									drawing_started = (component.factor > 0.0005f);
							}
						}
						if (drawing_color.spot_color->getPriority() == MapColor::Undefined)
						{
							// If the renderables do not draw color to this separation,
							// check if they need a knockout.
							if (renderables_color->getKnockout() && renderables_color->getPriority() < separation->getPriority())
								// explicit knockout
								drawing_color = SpotColorComponent(separation, 0.0f);
							else if (drawing_started)
								// knockout for out-of-sequence colors
								drawing_color = SpotColorComponent(separation, 0.0f);
						}
						break;
					}
					default:
						; // nothing
				}
				if (drawing_color.spot_color->getPriority() == MapColor::Undefined)
					continue;
				
				QColor color = *drawing_color.spot_color;
				if (drawing_color.factor < 0.0005f)
					color = Qt::white;
				else
				{
					qreal c, m, y, k;
					color.getCmykF(&c, &m, &y, &k);
					color.setCmykF(c*drawing_color.factor,m*drawing_color.factor,y*drawing_color.factor,k*drawing_color.factor,1.0f);
				}
				
				if (new_states.mode == RenderStates::PenOnly)
				{
					bool pen_too_small = (force_min_size && pen_width * scaling <= 1.0f);
					painter->setPen(QPen(highlighted ? getHighlightedColor(color) : color, pen_too_small ? 0 : pen_width));
					painter->setBrush(QBrush(Qt::NoBrush));
				}
				else if (new_states.mode == RenderStates::BrushOnly)
				{
					QBrush brush(highlighted ? getHighlightedColor(color) : color);
					painter->setPen(QPen(Qt::NoPen));
					painter->setBrush(brush);
				}
				
				// TODO: Check for removal
				painter->setOpacity(qMin(1.0f, opacity_factor * drawing_color.spot_color->getOpacity()));
				
				if (current_clip != new_states.clip_path)
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
