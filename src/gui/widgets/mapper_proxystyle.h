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


#ifndef OPENORIENTEERING_MAPPER_PROXYSTYLE_H
#define OPENORIENTEERING_MAPPER_PROXYSTYLE_H

#include <QtGlobal>
#include <QIcon>
#include <QFont>
#include <QObject>
#include <QPalette>
#include <QPixmap>
#include <QProxyStyle>
#include <QRect>
#include <QSize>
#include <QString>
#include <QStyle>

class QApplication;
class QPainter;
class QStyleHintReturn;
class QStyleOptionComplex;
class QStyleOption;
class QWidget;

namespace OpenOrienteering {


/**
 * MapperProxyStyle customizes the platform's base style.
 * 
 * It supports the implementation of missing UI elements, and 
 * it adjustes the size of some elements to more practical values.
 */
class MapperProxyStyle : public QProxyStyle
{
Q_OBJECT
	
	/**
	 * This structure holds custom toolbar metrics when touch_mode is active.
	 */
	struct ToolBarMetrics
	{
		int icon_size;
		int item_spacing;
		int separator_extent;
		int extension_extent;
	};
	
	/**
	 * This structure holds custom menu metrics when touch_mode is active.
	 */
	struct MenuMetrics
	{
		int button_indicator;
		int h_margin;
		int v_margin;
		int panel_width;
		int item_height;
		int scroller_height;
	};
	
public:
	/**
	 * Constructs a new MapperProxyStyle.
	 * 
	 * Being a QProxyStyle, MapperProxyStyle takes ownership of the base style,
	 * if given.
	 */
	MapperProxyStyle(QStyle* base_style = nullptr);
	
	MapperProxyStyle(const QPalette& palette, QStyle* base_style = nullptr);
	
	/**
	 * Destroys the object.
	 */
	~MapperProxyStyle() override;
	
	
	void polish(QApplication* application) override;
	
	void unpolish(QApplication* application) override;
	
	
	/**
	 * Draws the given primitive element.
	 * 
	 * Implements rendering of segmented buttons for all platforms.
	 * 
	 * In touch mode:
	 * - Up/down spinbox indicators are turned into plus/minus indicators.
	 * - For spin box indicators, the rect is performed in square with sufficient margin.
	 * 
	 * On Android:
	 * - QStyle::PE_IndicatorItemViewItemCheck is modified for disabled and tristate checkboxes.
	 */
	void drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget = nullptr) const override;
	
	/**
	 * Draws the given complex control.
	 * 
	 * In touch mode:
	 * - Use QCommonStyle for spinbox drawing if the base style inherits from it.
	 *   This improves drawing the buttons side by side with full height.
	 */
	void drawComplexControl(ComplexControl control, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget = nullptr) const override;
	
	/**
	 * Returns some adjusted pixel metrics.
	 *
	 * On OS X:
	 * - QStyle::PM_ToolBarIconSize is adjusted (reduced) towards QStyle::PM_SmallIconSize.
	 * 
	 * On Android:
	 * - QStyle::PM_ButtonIconSize is enlarged to QStyle::PM_IndicatorWidth (checkbox size),
	 * - QStyle::PM_SmallIconSize is enlarged to the (overwritten) PM_ButtonIconSize
	 *   for dockwidget related widgets.
	 * - QStyle::PM_DockWidgetSeparatorExtent and
	 *   QStyle::PM_SplitterWidth are adjusted (enlarged) towards QStyle::PM_IndicatorWidth.
	 */ 
	int pixelMetric(PixelMetric metric, const QStyleOption* option = nullptr, const QWidget* widget = nullptr) const override;
	
	/**
	 * Returns adjusted widget sizes.
	 * 
	 * On Android:
	 * - QStyle::CT_SizeGrip is enlarged to the (overwritten) PM_ButtonIconSize.
	 */
	QSize sizeFromContents(ContentsType ct, const QStyleOption* opt, const QSize& contents_size, const QWidget* w = nullptr) const override;
	
	/**
	 * Returns adjusted standard icons.
	 * 
	 * On Android:
	 * - For dockwidget buttons, the QCommonStyle implementation is used instead
	 *   of the QFusionStyle one.
	 */
	QIcon standardIcon(StandardPixmap standard_icon, const QStyleOption* option, const QWidget* widget) const override;
	
	/**
	 * Returns adjusted standard pixmaps.
	 * 
	 * On Android:
	 * - For dockwidget buttons, the QCommonStyle implementation is used instead
	 *   of the QFusionStyle one.
	 */
	QPixmap standardPixmap(StandardPixmap standard_icon, const QStyleOption* option, const QWidget* widget) const override;
	
	/**
	 * Returns adjusted style hints.
	 * 
	 * On Android:
	 * - QStyle::SH_FormLayoutWrapPolicy is QFormLayout::QFormLayout::WrapLongRows.
	 */
	int styleHint(StyleHint hint, const QStyleOption* option = nullptr, const QWidget* widget = nullptr, QStyleHintReturn* return_data = nullptr) const override;
	
	/**
	 * Determines location and size of subcontrols of complex controls.
	 * 
	 * In touch mode:
	 * - Spinbox buttons are placed side by side with full height (.... | + | - |).
	 */
	QRect subControlRect(ComplexControl cc, const QStyleOptionComplex* option, SubControl sc, const QWidget* widget) const override;
	
private:
	void drawSegmentedButton(int segment, PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const;
	
	void onSettingsChanged();
	
	
	QPalette default_palette;
	ToolBarMetrics toolbar = {};
	MenuMetrics menu       = {};
	QFont original_font    = {};
	QFont menu_font        = {};
	int button_size        = 0;
	int small_icon_size    = 0;
	bool touch_mode        = false;
	
	Q_DISABLE_COPY(MapperProxyStyle)
};


}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_MAPPER_PROXYSTYLE_H
