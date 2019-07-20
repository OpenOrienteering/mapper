/*
 *    Copyright 2013, 2016 Kai Pastor
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
#include <QObject>
#include <QProxyStyle>
#include <QStyle>

class QPainter;
class QStyleHintReturn;
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
public:
	/**
	 * Constructs a new MapperProxyStyle.
	 * 
	 * Being a QProxyStyle, MapperProxyStyle takes ownership of the base style,
	 * if given.
	 */
	MapperProxyStyle(QStyle* base_style = nullptr);
	
	/**
	 * Destroys the object.
	 */
	~MapperProxyStyle() override;
	
	/**
	 * Draws the given primitive element.
	 * 
	 * Implements rendering of segmented buttons for all platforms.
	 * 
	 * On Android:
	 * - QStyle::PE_IndicatorItemViewItemCheck is modified for disabled and tristate checkboxes.
	 */
	void drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget = nullptr) const override;
	
	/**
	 * Returns some adjusted pixel metrics.
	 *
	 * On OS X:
	 * - QStyle::PM_ToolBarIconSize is adjusted (reduced) towards QStyle::PM_SmallIconSize.
	 * 
	 * On Android:
	 * - QStyle::PM_ButtonIconSize is enlarged to QStyle::PM_IndicatorWidth (checkbox size),
	 * - QStyle::PM_SplitterWidth is adjusted (enlarged) towards QStyle::PM_IndicatorWidth.
	 */ 
	int pixelMetric(PixelMetric metric, const QStyleOption* option = nullptr, const QWidget* widget = nullptr) const override;
	
	/**
	 * Returns adjusted style hints.
	 * 
	 * On Android:
	 * - QStyle::SH_FormLayoutWrapPolicy is QFormLayout::QFormLayout::WrapLongRows.
	 */
	int styleHint(StyleHint hint, const QStyleOption* option = nullptr, const QWidget* widget = nullptr, QStyleHintReturn* return_data = nullptr) const override;
	
private:
	void drawSegmentedButton(int segment, PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const;
	
	Q_DISABLE_COPY(MapperProxyStyle)
};


}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_MAPPER_PROXYSTYLE_H
