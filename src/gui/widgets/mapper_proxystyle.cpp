/*
 *    Copyright 2013 Kai Pastor
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


#include "mapper_proxystyle.h"

#include <QtWidgets>

#include "segmented_button_layout.h"

MapperProxyStyle::MapperProxyStyle(QStyle* base_style)
 : QProxyStyle(base_style)
{
	; // Nothing
}

MapperProxyStyle::~MapperProxyStyle()
{
	; // Nothing, not inlined
}

void MapperProxyStyle::drawPrimitive(QStyle::PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const
{
	int segment = widget ? widget->property("segment").toInt() : 0;
	if (segment)
	{
		// Segmented Buttons implementation
		switch (element)
		{
			case PE_IndicatorButtonDropDown:
				element = PE_PanelButtonTool; // Enforce same appearence. Workaround QWindowsStyle quirk.
				/* 
				 * fall through ...
				 */
			case PE_PanelButtonCommand:
			case PE_PanelButtonBevel:
			case PE_PanelButtonTool:
			{
				if (option->rect.left())
				{
					// Subcomponent clipping
					painter->save();
					painter->setClipRect(option->rect, Qt::IntersectClip);
				}
				
				// Background (to be clipped by the widget)
				int left_adj = ((segment & SegmentedButtonLayout::LeftNeighbor) || option->rect.left()) ? 4 : 0;
				int right_adj = (segment & SegmentedButtonLayout::RightNeighbor) ? 4 : 0;
				
				QStyleOption mod_option(*option);
				mod_option.rect.adjust(-left_adj, 0, right_adj, 0);
				QProxyStyle::drawPrimitive(element, &mod_option, painter, widget);
				
				// Segment separators
				qreal saved_opacity = painter->opacity();
				painter->setOpacity((option->state & QStyle::State_Enabled) ? 0.5 : 0.2);
				int frame_width = proxy()->pixelMetric(PM_DefaultFrameWidth, option, widget);
				mod_option.rect = option->rect.adjusted(0, frame_width, 0, -frame_width);
				if (left_adj)
				{
					drawLeftSeparatorLine(painter, &mod_option);
				}
				if (right_adj)
				{
					drawRightSeparatorLine(painter, &mod_option);
				}
				painter->setOpacity(saved_opacity);
				
				if (option->rect.left())
				{
					painter->restore();
				}
				
				return;
			}
			
			default:
				; // Nothing
		}
	}
	
	QProxyStyle::drawPrimitive(element, option, painter, widget);
}

void MapperProxyStyle::drawLeftSeparatorLine(QPainter* painter, const QStyleOption* option)
{
	QPen old_pen = painter->pen();
	if (option->state & QStyle::State_Sunken)
	    painter->setPen(option->palette.dark().color());
	else
	    painter->setPen(option->palette.light().color());
	painter->drawLine(option->rect.left(), option->rect.top(), option->rect.left(), option->rect.bottom());
	painter->setPen(old_pen);
}

void MapperProxyStyle::drawRightSeparatorLine(QPainter* painter, const QStyleOption* option)
{
	QPen old_pen = painter->pen();
	if (option->state & QStyle::State_Sunken)
	    painter->setPen(option->palette.light().color());
	else
	    painter->setPen(option->palette.dark().color());
	painter->drawLine(option->rect.right(), option->rect.top(), option->rect.right(), option->rect.bottom());
	painter->setPen(old_pen);
}

#ifdef Q_OS_MAC
int MapperProxyStyle::pixelMetric(PixelMetric metric, const QStyleOption* option, const QWidget* widget) const
{
	if (metric == QStyle::PM_ToolBarIconSize)
	{
		static int s = (QProxyStyle::pixelMetric(QStyle::PM_SmallIconSize) + QProxyStyle::pixelMetric(QStyle::PM_ToolBarIconSize)) / 2;
		return s;
	}
	return QProxyStyle::pixelMetric(metric, option, widget);
}
#endif
