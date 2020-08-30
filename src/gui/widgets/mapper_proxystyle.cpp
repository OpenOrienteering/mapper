/*
 *    Copyright 2013-2020 Kai Pastor
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

#include <algorithm>

#include <Qt>
#include <QAbstractSpinBox>
#include <QApplication>
#include <QBrush>
#include <QByteArray>
#include <QColor>
#include <QCommonStyle> // IWYU pragma: keep
#include <QCoreApplication>
#include <qdrawutil.h>  // IWYU pragma: keep
#include <QFlags>
#include <QFormLayout>  // IWYU pragma: keep
#include <QGuiApplication>
#include <QMetaObject>
#include <QPainter>
#include <QPalette>
#include <QRect>
#include <QRgb>
#include <QScreen>
#include <QSize>
#include <QStyleOption>
#include <QStyleOptionComplex>
#include <QStyleOptionMenuItem>
#include <QStyleOptionSpinBox>
#include <QVariant>
#include <QWidget>

#include "settings.h"
#include "gui/scaling_icon_engine.h"
#include "gui/widgets/segmented_button_layout.h"
#include "gui/util_gui.h"


namespace OpenOrienteering {

namespace  {

/**
 * This function recognizes dock widget related widgets by the class name
 * matching "DockWidget".
 * 
 * This function helps to customize the style for classes like
 * "QDockWidgetTitleButton" or "MapEditorDockWidget".
 */
bool Q_DECL_UNUSED isDockWidgetRelated(const QWidget* widget)
{
	if (widget == nullptr)
		return false;
	
	auto* raw_class_name = widget->metaObject()->className();
	auto const class_name = QByteArray::fromRawData(raw_class_name, static_cast<int>(qstrlen(raw_class_name)));
	return class_name.contains("DockWidget");
}

int buttonSizePixel(const Settings& settings)
{
	auto const size_mm = settings.getSetting(Settings::ActionGridBar_ButtonSizeMM).toReal();
	return qRound(Util::mmToPixelPhysical(size_mm));
}

// Cf. qt_defaultDpiX in qfont.cpp
int defaultDpi()
{
	if (auto* screen = QGuiApplication::primaryScreen())
		return qRound(screen->logicalDotsPerInchX());
	return 100;
}

// Cf. dpiScaled in qstylehelper.cpp, qt_defaultDpiX in qfont.cpp
qreal dpiScaled(qreal value)
{
#ifdef Q_OS_MAC
	return value;
#else
	if (QCoreApplication::instance()->testAttribute(Qt::AA_Use96Dpi))
		return value;
	static const qreal scale = defaultDpi() / 96.0;
	return value * scale;
#endif
}

}  // namespace



MapperProxyStyle::MapperProxyStyle(QStyle* base_style)
: MapperProxyStyle(QApplication::palette(), base_style)
{
}

MapperProxyStyle::MapperProxyStyle(const QPalette& palette, QStyle* base_style)
: QProxyStyle(base_style)
, default_palette(palette)
{
}

MapperProxyStyle::~MapperProxyStyle() = default;


void MapperProxyStyle::onSettingsChanged()
{
	auto& settings = Settings::getInstance();
	if (touch_mode != settings.touchModeEnabled()
	    || (touch_mode && button_size != buttonSizePixel(settings)))
	{
#ifndef __clang_analyzer__
		// No leak: QApplication takes ownership.
		QApplication::setStyle(new MapperProxyStyle(default_palette));
#endif
	}
}


void MapperProxyStyle::polish(QApplication* application)
{
	common_style = qobject_cast<QCommonStyle*>(baseStyle());
	
	QProxyStyle::polish(application);
	QApplication::setPalette(default_palette);
	
	auto& settings = Settings::getInstance();
	connect(&settings, &Settings::settingsChanged, this, &MapperProxyStyle::onSettingsChanged);
	if (settings.touchModeEnabled())
	{
		touch_mode = true;
		
		button_size = buttonSizePixel(settings);
		auto const margin_size_pixel = button_size / 4;
		toolbar.icon_size = button_size - margin_size_pixel;
		{
			auto const scale_factor = qreal(toolbar.icon_size) / QProxyStyle::pixelMetric(PM_ToolBarIconSize);
			toolbar.item_spacing = std::max(1, margin_size_pixel - 2 * qRound(scale_factor));
			toolbar.separator_extent = qRound(QProxyStyle::pixelMetric(PM_ToolBarSeparatorExtent) * scale_factor);
			toolbar.extension_extent = qRound(QProxyStyle::pixelMetric(PM_ToolBarExtensionExtent) * scale_factor);
		}
		
		small_icon_size = qMax(QProxyStyle::pixelMetric(QStyle::PM_ButtonIconSize), int(0.7 * toolbar.icon_size));
		{
			auto const scale_factor = qreal(small_icon_size) / QProxyStyle::pixelMetric(QStyle::PM_ButtonIconSize);
			menu.button_indicator = qRound(QProxyStyle::pixelMetric(PM_MenuButtonIndicator) * scale_factor);
			if (baseStyle()->inherits("QFusionStyle"))
			{
				// QFusionStyle draws indicators no larger than 14 logical pixels,
				// cf. qt_fusion_draw_array.
				// If we return more than that, a tiny indicator is placed near
				// the center of the button, interfering with the contents.
				auto const fusion_style_bound = int(dpiScaled(14.0));
				menu.button_indicator = qMin(menu.button_indicator, fusion_style_bound);
			}
			menu.h_margin = qMax(margin_size_pixel / 4, qRound(QProxyStyle::pixelMetric(PM_MenuHMargin) * scale_factor));
			menu.v_margin = qMax(margin_size_pixel / 4, qRound(QProxyStyle::pixelMetric(PM_MenuVMargin) * scale_factor));
			menu.panel_width = qRound(QProxyStyle::pixelMetric(PM_MenuPanelWidth) * scale_factor);
			
			menu.item_height = button_size;
			menu.scroller_height = qMax(QProxyStyle::pixelMetric(PM_MenuScrollerHeight), menu.item_height / 2);
		}
		
		menu_font = QApplication::font();
		{
			auto const menu_font_size = small_icon_size - 4;  // cf. QMenu's action item rect calculation.
			if (menu_font_size > original_font.pixelSize())
			{
				menu_font.setPixelSize(menu_font_size);
			}
			QApplication::setFont(QApplication::font());
			QApplication::setFont(menu_font, "QMenu");
			QApplication::setFont(menu_font, "QComboMenuItem");
		}
	}
}

void MapperProxyStyle::unpolish(QApplication* application)
{
	if (touch_mode)
	{
		QApplication::setFont(QApplication::font());
	}
	
	QApplication::setPalette(default_palette);
	QProxyStyle::unpolish(application);
	
	common_style = nullptr;
}


void MapperProxyStyle::childEvent(QChildEvent* event)
{
	if (event->added() || event->removed())
		common_style = qobject_cast<QCommonStyle*>(baseStyle());
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
		if (touch_mode
		    && element == PE_PanelButtonTool
		    && option
		    && option->state & State_On)
		{
			auto const& window_color = option->palette.window().color();
			auto const fill = QBrush(qGray(window_color.rgb()) > 127 ? window_color.darker(125) : window_color.lighter(125));
			painter->setPen(Qt::NoPen);
			painter->setBrush(fill);
			painter->drawRoundedRect(option->rect, 5.0, 5.0, Qt::AbsoluteSize);
			return;
		}
		break;
		
	case PE_IndicatorSpinUp:
	case PE_IndicatorSpinDown:
		overridden_element = element == PE_IndicatorSpinUp ? PE_IndicatorSpinPlus : PE_IndicatorSpinMinus;
		Q_FALLTHROUGH();
	case PE_IndicatorSpinPlus:
	case PE_IndicatorSpinMinus:
		if (touch_mode)
		{
			if (const QStyleOptionSpinBox* spinbox = qstyleoption_cast<const QStyleOptionSpinBox*>(option))
			{
				// Create a small centered square for QCommonStyle to draw '+'/'-'
				auto copy = *spinbox;
				auto const outer_size = qMax(copy.rect.width(), copy.rect.height());
				auto const inner_size = qMin(3 * outer_size / 6, qMin(copy.rect.width(), copy.rect.height()));
				auto const delta_x = (copy.rect.width() - inner_size) / 2;
				auto const delta_y = (copy.rect.height() - inner_size) / 2;
				copy.rect = {copy.rect.x() + delta_x, copy.rect.y() + delta_y, inner_size, inner_size };
				QProxyStyle::drawPrimitive(overridden_element, &copy, painter, widget);
				return;
			}
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


void MapperProxyStyle::drawComplexControl(QStyle::ComplexControl control, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget) const
{
	if (touch_mode)
	{
		switch (control)
		{
		case CC_SpinBox:
			if (common_style)
			{
				common_style->QCommonStyle::drawComplexControl(control, option, painter, widget);
				return;
			}
			break;
			
		default:
			;  // Nothing
		}
	}
	
	QProxyStyle::drawComplexControl(control, option, painter, widget);
}


int MapperProxyStyle::pixelMetric(PixelMetric metric, const QStyleOption* option, const QWidget* widget) const
{
	if (touch_mode)
	{
		switch (metric)
		{
		case QStyle::PM_ToolBarIconSize:
			return toolbar.icon_size;
		case QStyle::PM_ToolBarItemSpacing:
			return toolbar.item_spacing;
		case QStyle::PM_ToolBarSeparatorExtent:
			return toolbar.separator_extent;
		case QStyle::PM_ToolBarExtensionExtent:
			return toolbar.extension_extent;
		case PM_MenuButtonIndicator:
			return menu.button_indicator;
		case PM_MenuHMargin:
			return menu.h_margin;
		case PM_MenuVMargin:
			return menu.v_margin;
		case PM_MenuPanelWidth:
			return menu.panel_width;
		case PM_MenuScrollerHeight:
			return menu.scroller_height;
		case QStyle::PM_ButtonIconSize:
		case QStyle::PM_SmallIconSize:
			return small_icon_size;
		case QStyle::PM_DockWidgetSeparatorExtent:
		case QStyle::PM_SplitterWidth:
			return (QProxyStyle::pixelMetric(metric) + small_icon_size) / 2;
		default:
			break;
		}
	}
#ifdef Q_OS_MACOS
	else
	{
		switch (metric)
		{
		case QStyle::PM_ToolBarIconSize:
			return (QProxyStyle::pixelMetric(metric) + QProxyStyle::pixelMetric(QStyle::PM_SmallIconSize)) / 2;
		default:
			break;
		}
	}
#endif
	
	return QProxyStyle::pixelMetric(metric, option, widget);
}

QSize MapperProxyStyle::sizeFromContents(QStyle::ContentsType ct, const QStyleOption* opt, const QSize& contents_size, const QWidget* w) const
{
	if (touch_mode)
	{
		switch (ct)
		{
		case QStyle::CT_MenuItem:
			if (auto const* menu_item = qstyleoption_cast<const QStyleOptionMenuItem *>(opt))
			{
				auto size = QProxyStyle::sizeFromContents(ct, opt, contents_size, w);
				if (menu_item->icon.isNull() && (!w || !w->inherits("QComboBox")))
					size.rwidth() += small_icon_size;  // reserved for checkmark
				if (menu_item->menuItemType == QStyleOptionMenuItem::Separator && menu_item->text.isEmpty())
					size.rheight() += menu.v_margin;
				else
					size.setHeight(qMax(size.height(), menu.item_height));
				return size;
			}
			break;
		case QStyle::CT_SizeGrip:
			return [](int value) {
				return QSize{ value, value };
			} (qMax(QProxyStyle::pixelMetric(QStyle::PM_ButtonIconSize), toolbar.icon_size / 2));
		default:
			break;
		}
	}
	
	return QProxyStyle::sizeFromContents(ct, opt, contents_size, w);
}

QIcon MapperProxyStyle::standardIcon(QStyle::StandardPixmap standard_icon, const QStyleOption* option, const QWidget* widget) const
{
	QIcon icon;
	switch (standard_icon)
	{
#ifdef Q_OS_ANDROID
	// Cf. https://code.qt.io/cgit/qt/qtbase.git/tree/src/widgets/styles/qfusionstyle.cpp?h=5.12#n3785
	case QStyle::SP_TitleBarNormalButton:
	case QStyle::SP_TitleBarCloseButton:
		if (common_style)
		{
			icon = common_style->QCommonStyle::standardIcon(standard_icon, option, widget);
		}
		break;
#endif
	default:
		break;
	}
	
	if (icon.isNull())
		icon = QProxyStyle::standardIcon(standard_icon, option, widget);
	if (icon.actualSize(QSize(1000,1000)).width() < 1000)
		icon = QIcon(new ScalingIconEngine(icon));
	return icon;
}

QPixmap MapperProxyStyle::standardPixmap(QStyle::StandardPixmap standard_pixmap, const QStyleOption* option, const QWidget* widget) const
{
	switch (standard_pixmap)
	{
#ifdef Q_OS_ANDROID
	// Cf. https://code.qt.io/cgit/qt/qtbase.git/tree/src/widgets/styles/qfusionstyle.cpp?h=5.12#n3807
	case QStyle::SP_TitleBarNormalButton:
	case QStyle::SP_TitleBarCloseButton:
		if (common_style)
		{
			return common_style->QCommonStyle::standardPixmap(standard_pixmap, option, widget);
		}
		break;
#endif
	default:
		break;
	}
	
	return QProxyStyle::standardPixmap(standard_pixmap, option, widget);
}

int MapperProxyStyle::styleHint(QStyle::StyleHint hint, const QStyleOption* option, const QWidget* widget, QStyleHintReturn* return_data) const
{
	switch (hint)
	{
	case SH_Menu_Scrollable:
		if (touch_mode)
			return true;
		break;
#ifdef Q_OS_ANDROID
	case QStyle::SH_FormLayoutWrapPolicy:
		return QFormLayout::WrapLongRows;
#endif
	default:
		break;
	}
	
	return QProxyStyle::styleHint(hint, option, widget, return_data);
}

QRect MapperProxyStyle::subControlRect(QStyle::ComplexControl cc, const QStyleOptionComplex* option, QStyle::SubControl sc, const QWidget* widget) const
{
	if (touch_mode)
	{
		switch (cc)
		{
		case CC_SpinBox:
			if (auto* spinbox = qstyleoption_cast<const QStyleOptionSpinBox *>(option))
			{
				auto const frame_width = spinbox->frame ? qMax(1, pixelMetric(PM_SpinBoxFrameWidth, spinbox, widget) - 1) : 0;
				auto rect = spinbox->rect.adjusted(frame_width, frame_width, -frame_width, -frame_width);
				auto const button_width = rect.height();
				switch (sc) {
				case SC_SpinBoxUp:
					if (spinbox->buttonSymbols == QAbstractSpinBox::NoButtons)
						return {};
					rect.setLeft(rect.left() + rect.width() - 2 * button_width);
					rect.setWidth(button_width);
					break;
				case SC_SpinBoxDown:
					if (spinbox->buttonSymbols == QAbstractSpinBox::NoButtons)
						return {};
					rect.setLeft(rect.left() + rect.width() - button_width);
					rect.setWidth(button_width);
					break;
				case SC_SpinBoxEditField:
					if (spinbox->buttonSymbols != QAbstractSpinBox::NoButtons)
						rect.setWidth(rect.width() - 2 * button_width);
					break;
				case SC_SpinBoxFrame:
					return spinbox->rect;
				default:
					break;
				}
				return visualRect(spinbox->direction, spinbox->rect, rect);
			}
			break;
		default:
			;  // nothing
		}
	}
	
	return QProxyStyle::subControlRect(cc, option, sc, widget);
}


}  // namespace OpenOrienteering
