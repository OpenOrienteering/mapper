/*
 *    Copyright 2013-2016 Kai Pastor
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

#include <Qt>
#include <QBrush>
#include <QFlags>
#include <QFormLayout>  // IWYU pragma: keep
#include <QPainter>
#include <QPalette>
#include <QRect>
#include <QStyleOption>
#include <QVariant>
#include <QWidget>

#include "segmented_button_layout.h"


namespace OpenOrienteering {

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
	auto overridden_element = element;
	switch (element)
	{
	case PE_IndicatorButtonDropDown:
		overridden_element = PE_PanelButtonTool; // Enforce same appearance. Workaround QWindowsStyle quirk.
		Q_FALLTHROUGH();
	case PE_PanelButtonCommand:
	case PE_PanelButtonBevel:
	case PE_PanelButtonTool:
		if (int segment = widget ? widget->property("segment").toInt() : 0)
		{
			drawSegmentedButton(segment, overridden_element, option, painter, widget);
			return;
		}
		break;
		
#ifdef Q_OS_ANDROID
	case QStyle::PE_IndicatorItemViewItemCheck:
		if (option->state.testFlag(QStyle::State_NoChange)
		    || !option->state.testFlag(QStyle::State_Enabled))
		{
			auto item = qstyleoption_cast<const QStyleOptionViewItem*>(option);
			auto o = QStyleOptionViewItem{ *item };
			o.state |= QStyle::State_Enabled;
			if (option->state.testFlag(QStyle::State_NoChange))
			{
				o.state &= ~QStyle::State_NoChange;
				o.state |= QStyle::State_On;
			}
			auto opacity = painter->opacity();
			painter->setOpacity(0.4);
			QProxyStyle::drawPrimitive(element, &o, painter, widget);
			painter->setOpacity(opacity);
			return;
		}
		break;
#endif
		
	default:
		; // Nothing
	}
	
	QProxyStyle::drawPrimitive(element, option, painter, widget);
}

void MapperProxyStyle::drawSegmentedButton(int segment, QStyle::PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const
{
	painter->save();
	
	// Background (to be clipped by the widget)
	int left_adj  = (segment & SegmentedButtonLayout::LeftNeighbor)  ? 4 : 0;
	int right_adj = (segment & SegmentedButtonLayout::RightNeighbor) ? 4 : 0;
	
	if (option->rect.left())
	{
		// Subcomponent clipping
		painter->setClipRect(option->rect, Qt::IntersectClip);
		left_adj = 4;
	}
	
	QStyleOption mod_option(*option);
	mod_option.rect.adjust(-left_adj, 0, right_adj, 0);
	QProxyStyle::drawPrimitive(element, &mod_option, painter, widget);
	
	// Segment separators
	painter->setOpacity((option->state & QStyle::State_Enabled) ? 0.5 : 0.2);
	int frame_width = proxy()->pixelMetric(PM_DefaultFrameWidth, option, widget);
	mod_option.rect = option->rect.adjusted(0, frame_width, 0, -frame_width);
	
	if (left_adj)
	{
		if (option->state & QStyle::State_Sunken)
		    painter->setPen(option->palette.dark().color());
		else
		    painter->setPen(option->palette.light().color());
		painter->drawLine(option->rect.left(), option->rect.top(), option->rect.left(), option->rect.bottom());
	}
	
	if (right_adj)
	{
		if (option->state & QStyle::State_Sunken)
		    painter->setPen(option->palette.light().color());
		else
		    painter->setPen(option->palette.dark().color());
		painter->drawLine(option->rect.right(), option->rect.top(), option->rect.right(), option->rect.bottom());
	}
	
	painter->restore();
}

int MapperProxyStyle::pixelMetric(PixelMetric metric, const QStyleOption* option, const QWidget* widget) const
{
	switch (metric)
	{
#ifdef Q_OS_ANDROID
	case QStyle::PM_ButtonIconSize:
		{
			static int s = qMax(QProxyStyle::pixelMetric(metric), QProxyStyle::pixelMetric(QStyle::PM_IndicatorWidth));
			return s;
		}
	case QStyle::PM_SplitterWidth:
		{
			static int s = (QProxyStyle::pixelMetric(metric), QProxyStyle::pixelMetric(QStyle::PM_IndicatorWidth)) / 2;
			return s;
		}
#endif
#ifdef Q_OS_MACOS
	case QStyle::PM_ToolBarIconSize:
		{
			static int s = (QProxyStyle::pixelMetric(metric) + QProxyStyle::pixelMetric(QStyle::PM_SmallIconSize)) / 2;
			return s;
		}
#endif
	default:
		break;
	}
	
	return QProxyStyle::pixelMetric(metric, option, widget);
}

int MapperProxyStyle::styleHint(QStyle::StyleHint hint, const QStyleOption* option, const QWidget* widget, QStyleHintReturn* return_data) const
{
	switch (hint)
	{
#ifdef Q_OS_ANDROID
	case QStyle::SH_FormLayoutWrapPolicy:
		return QFormLayout::WrapLongRows;
#endif		
	default:
		break;
	}
	
	return QProxyStyle::styleHint(hint, option, widget, return_data);
}


}  // namespace OpenOrienteering
