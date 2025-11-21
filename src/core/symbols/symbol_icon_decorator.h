/*
 *    Copyright 2014, 2018 Kai Pastor
 *    Copyright 2025 Matthias Kühlewein
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


#ifndef OPENORIENTEERING_SYMBOL_ICON_DECORATOR_H
#define OPENORIENTEERING_SYMBOL_ICON_DECORATOR_H

#include <QPoint>

class QPainter;

namespace OpenOrienteering {


/**
 * An abstract interface for classes which draws icon decorations.
 * 
 * The icon is expected to be at (0, 0) in the painter's coordinates.
 */
class SymbolIconDecorator
{
public:
	SymbolIconDecorator() noexcept = default;
	SymbolIconDecorator(const SymbolIconDecorator&) = delete;
	SymbolIconDecorator(SymbolIconDecorator&&) = delete;
	SymbolIconDecorator& operator=(const SymbolIconDecorator&) = delete;
	SymbolIconDecorator& operator=(SymbolIconDecorator&&) = delete;
	virtual ~SymbolIconDecorator();
	virtual void draw(QPainter& p) const = 0;
};



/**
 * Draws the decoration for a hidden symbol.
 * 
 * A small red x is drawn in the top-left corner of the icon.
 */
class HiddenSymbolDecorator : public SymbolIconDecorator
{
public:
	explicit HiddenSymbolDecorator(int icon_size);
	~HiddenSymbolDecorator() override;
	void draw(QPainter& p) const override;
	
private:
	int icon_size;
	int pen_width;
	int x_width;
	QPoint offset;
};



/**
 * Draws the decoration for a protected symbol.
 * 
 * A small gray lock is drawn in the top-right corner of the icon.
 */
class ProtectedSymbolDecorator : public SymbolIconDecorator
{
public:
	explicit ProtectedSymbolDecorator(int icon_size);
	~ProtectedSymbolDecorator() override;
	void draw(QPainter& p) const override;
	
private:
	int arc_size;
	int pen_width;
	int box_width;
	int box_height;
	QPoint offset;
};


/**
 * Draws the decoration for a helper symbol.
 * 
 * A small blue gearwheel is drawn in the bottom-left corner of the icon.
 */
class HelperSymbolDecorator : public SymbolIconDecorator
{
public:
	explicit HelperSymbolDecorator(int icon_size);
	~HelperSymbolDecorator() override;
	void draw(QPainter& p) const override;
	
private:
	int pen_width;
	int x_width;
	QPoint offset;
};


}  // namespace OpenOrienteering

#endif  // OPENORIENTEERING_SYMBOL_ICON_DECORATOR_H
