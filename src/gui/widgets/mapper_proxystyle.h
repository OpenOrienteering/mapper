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


#ifndef _OPENORIENTEERING_MAPPER_PROXYSTYLE_H_
#define _OPENORIENTEERING_MAPPER_PROXYSTYLE_H_

#include <QProxyStyle>

/**
 * MapperProxyStyle customizes the platform's base style.
 * 
 * It implements the rendering of segmented buttons (in connection with
 * SegmentedButtonLayout).
 * 
 * On OS X, it modifies the size of the toolbar icons.
 */
class MapperProxyStyle : public QProxyStyle
{
Q_OBJECT
public:
	/**
	 * Constructs a new MapperProxyStyle.
	 * Being a QProxyStyle, MapperProxyStyle takes ownership of the base style,
	 * if given.
	 */
	MapperProxyStyle(QStyle* base_style = NULL);
	
	/**
	 * Destroys the object.
	 */
	virtual ~MapperProxyStyle();
	
	/**
	 * Draws the given primitive element.
	 * Implements rendering of segmented buttons.
	 */
	virtual void drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget = NULL) const;
	
	/**
	 * Draw the left margin of a button segment.
	 */
	static void drawLeftSeparatorLine(QPainter* painter, const QStyleOption* option);
	
	/**
	 * Draw the right margin of a button segment.
	 */
	static void drawRightSeparatorLine(QPainter* painter, const QStyleOption* option);
	
#ifdef Q_OS_MAC
	/**
	 * On OS X, returns a reduced size for QStyle::PM_ToolBarIconSize.
	 */ 
	virtual int pixelMetric(PixelMetric metric, const QStyleOption* option = NULL, const QWidget* widget = NULL) const;
#endif
	
private:
	Q_DISABLE_COPY(MapperProxyStyle)
};

#endif // _OPENORIENTEERING_MAPPER_PROXYSTYLE_H_
