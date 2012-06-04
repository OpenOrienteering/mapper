/*
 *    Copyright 2012 Thomas Schöps, Kai Pastor
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

#include <QPainter>
#include <qmath.h>

#include "map.h"
#include "map_color.h"
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
	for (iterator renderables = begin(); renderables != end(); ++renderables)
	{
		for (RenderableVector::const_iterator renderable = renderables->second.begin(); renderable != renderables->second.end(); ++renderable)
		{
			delete *renderable;
		}
		renderables->second.clear();
		if (renderables->first.clip_path != NULL)
			erase(renderables);
	}
}

void SharedRenderables::compact()
{
	for (iterator renderables = begin(); renderables != end(); ++renderables)
	{
		if (renderables->second.size() == 0)
		{
			erase(renderables);
		}
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

void ObjectRenderables::insertRenderable(Renderable* r)
{
	RenderStates state(r, clip_path);
	SharedRenderables::Pointer& container(operator[](state.color_priority));
	if (!container)
		container = new SharedRenderables();
	container->operator[](state).push_back(r);
	if (extent.isValid())
		rectInclude(extent, r->getExtent());
	else
		extent = r->getExtent();
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


// ### RenderableContainer ###

MapRenderables::MapRenderables(Map* map) : map(map)
{
}

void MapRenderables::draw(QPainter* painter, QRectF bounding_box, bool force_min_size, float scaling, bool show_helper_symbols, float opacity_factor, bool highlighted) const
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
					painter->setPen(QPen(highlighted ? getHighlightedColor(color->color) : color->color, pen_too_small ? 0 : pen_width));
					
					painter->setBrush(QBrush(Qt::NoBrush));
				}
				else if (new_states.mode == RenderStates::BrushOnly)
				{
					QBrush brush(highlighted ? getHighlightedColor(color->color) : color->color);
					
					painter->setPen(QPen(Qt::NoPen));
					painter->setBrush(brush);
				}
				
				painter->setOpacity(qMin(1.0f, opacity_factor * color->opacity));
				
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
					(*renderable)->render(*painter, force_min_size, scaling);
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
			
			for (SharedRenderables::const_iterator renderables = obj->second->begin(); renderables != obj->second->end(); ++renderables)
			{
				for (RenderableVector::const_iterator renderable = renderables->second.begin(); renderable != renderables->second.end(); ++renderable)
				{
					if (mark_area_as_dirty)
						map->setObjectAreaDirty((*renderable)->getExtent());
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
