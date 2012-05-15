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

#include "renderable.h"

#include <QPainter>
#include <qmath.h>

#include "map.h"
#include "map_color.h"
#include "object.h"
#include "object_text.h"
#include "symbol_point.h"
#include "symbol_line.h"
#include "symbol_area.h"
#include "symbol_text.h"
#include "util.h"

Renderable::Renderable()
{
	clip_path = NULL;
}
Renderable::Renderable(const Renderable& other)
{
	extent = other.extent;
	creator = other.creator;
	color_priority = other.color_priority;
	clip_path = other.clip_path;
}
Renderable::~Renderable()
{
}

// ### RenderableContainer ###

RenderableContainer::RenderableContainer(Map* map) : map(map)
{
}

void RenderableContainer::draw(QPainter* painter, QRectF bounding_box, bool force_min_size, float scaling, bool show_helper_symbols, float opacity_factor, bool highlighted)
{
	Map::ColorVector& colors = map->color_set->colors;
	
	// TODO: improve performance by using some spatial acceleration structure?
	RenderStates states;
	states.color_priority = -1;
	states.mode = RenderStates::Reserved;
	states.pen_width = -1;
	states.clip_path = NULL;
	
	QPainterPath initial_clip = painter->clipPath();
	bool no_initial_clip = initial_clip.isEmpty();
	
	painter->save();
	Renderables::const_reverse_iterator outer_it_end = renderables.rend();
	for (Renderables::const_reverse_iterator outer_it = renderables.rbegin(); outer_it != outer_it_end; ++outer_it)
	{
		RenderableContainerVector::const_iterator it_end = outer_it->second.end();
		for (RenderableContainerVector::const_iterator it = outer_it->second.begin(); it != it_end; ++it)
		{
			const RenderStates& new_states = (*it).first;
			Renderable* renderable = (*it).second;
			
			// Bounds check
			const QRectF& extent = renderable->getExtent();
			if (extent.right() < bounding_box.x())	continue;
			if (extent.bottom() < bounding_box.y())	continue;
			if (extent.x() > bounding_box.right())	continue;
			if (extent.y() > bounding_box.bottom())	continue;
			
			// Settings check
			Symbol* symbol = renderable->getCreator()->getSymbol();
			if (!show_helper_symbols && symbol->isHelperSymbol())
				continue;
			if (symbol->isHidden())
				continue;
			
			// Change render states?
			if (states != new_states)
			{
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
				
				if (states.clip_path != new_states.clip_path)
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
				}
				
				states = new_states;
			}
			
			// Render the renderable
			renderable->render(*painter, force_min_size, scaling);
		}
	}
	painter->restore();
}

void RenderableContainer::insertRenderablesOfObject(Object* object)
{
	RenderableVector::const_iterator it_end = object->endRenderables();
	for (RenderableVector::const_iterator it = object->beginRenderables(); it != it_end; ++it)
	{
		Renderable* renderable = *it;
		RenderStates render_states;
		renderable->getRenderStates(render_states);
		
		if (render_states.color_priority != MapColor::Reserved)
		{
			renderables[render_states.color_priority].push_back(std::make_pair(render_states, renderable));
			//renderables.insert(Renderables::value_type(render_states, renderable));
		}
	}
}
void RenderableContainer::removeRenderablesOfObject(Object* object, bool mark_area_as_dirty)
{
	Renderables::iterator itend = renderables.end();
	for (Renderables::iterator it = renderables.begin(); it != itend; ++it)
	{
		for (int i = (int)it->second.size() - 1; i >= 0; --i)
		{
			if (it->second.at(i).second->getCreator() != object)
				continue;
			
			// NOTE: this assumes that renderables by one object are at continuous indices
			int k = i;
			if (mark_area_as_dirty)
				map->setObjectAreaDirty(it->second.at(i).second->getExtent());
			while (k > 0 && it->second.at(k - 1).second->getCreator() == object)
			{
				--k;
				if (mark_area_as_dirty)
					map->setObjectAreaDirty(it->second.at(k).second->getExtent());
			}
			
			it->second.erase(it->second.begin() + k, it->second.begin() + (i + 1));
			break;
		}
		
		/*if ((*it).second->getCreator() == object)
		{
			if (mark_area_as_dirty)
				map->setObjectAreaDirty((*it).second->getExtent());
			Renderables::iterator todelete = it;
			++it;
			renderables.erase(todelete);
		}
		else
			++it;*/
	}
}

void RenderableContainer::clear()
{
	renderables.clear();
}

QColor RenderableContainer::getHighlightedColor(const QColor& original)
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
