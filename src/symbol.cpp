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


#include "symbol.h"

#include <QFile>
#include <qmath.h>

#include "util.h"
#include "object.h"
#include "object_text.h"
#include "symbol_line.h"
#include "symbol_point.h"
#include "symbol_area.h"
#include "symbol_text.h"
#include "symbol_combined.h"

Symbol::Symbol(Type type) : type(type), name(""), description(""), is_helper_symbol(false), is_hidden(false), is_protected(false), icon(NULL)
{
	for (int i = 0; i < number_components; ++i)
		number[i] = -1;
}
Symbol::~Symbol()
{
	delete icon;
}

bool Symbol::isTypeCompatibleTo(Object* object)
{
	if (type == Point && object->getType() == Object::Point)
		return true;
	else if ((type == Line || type == Area || type == Combined) && object->getType() == Object::Path)
		return true;
	else if (type == Text && object->getType() == Object::Text)
		return true;

	return false;
}

void Symbol::save(QFile* file, Map* map)
{
	saveString(file, name);
	for (int i = 0; i < number_components; ++i)
		file->write((const char*)&number[i], sizeof(int));
	saveString(file, description);
	file->write((const char*)&is_helper_symbol, sizeof(bool));
	file->write((const char*)&is_hidden, sizeof(bool));
	file->write((const char*)&is_protected, sizeof(bool));
	
	saveImpl(file, map);
}
bool Symbol::load(QFile* file, int version, Map* map)
{
	loadString(file, name);
	for (int i = 0; i < number_components; ++i)
		file->read((char*)&number[i], sizeof(int));
	loadString(file, description);
	file->read((char*)&is_helper_symbol, sizeof(bool));
	if (version >= 10)
		file->read((char*)&is_hidden, sizeof(bool));
	if (version >= 11)
		file->read((char*)&is_protected, sizeof(bool));
	
	return loadImpl(file, version, map);
}

QImage* Symbol::getIcon(Map* map, bool update)
{
	if (icon && !update)
		return icon;
	delete icon;
	
	// Create icon map and view
	Map icon_map;
	icon_map.useColorsFrom(map);
	icon_map.setScaleDenominator(map->getScaleDenominator());
	MapView view(&icon_map);
	
	// If the icon is bigger than the rectagle with this zoom factor, it is zoomed out to fit into the rectangle
	const float best_zoom = 2;
	view.setZoom(best_zoom);
	const float max_icon_mm = 0.001f * view.pixelToLength(Symbol::icon_size - 1);
	const float max_icon_mm_half = 0.5f * max_icon_mm;
	
	// Create image
	icon = new QImage(icon_size, icon_size, QImage::Format_ARGB32_Premultiplied);
	QPainter painter;
	painter.begin(icon);
	painter.setRenderHint(QPainter::Antialiasing);
	
	// Make background transparent
	QPainter::CompositionMode mode = painter.compositionMode();
	painter.setCompositionMode(QPainter::CompositionMode_Clear);
	painter.fillRect(icon->rect(), Qt::transparent);
	painter.setCompositionMode(mode);
	
	// Create geometry
	Object* object = NULL;
	if (type == Point)
	{
		PointObject* point = new PointObject(this);
		point->setPosition(0, 0);
		object = point;
	}
	else if (type == Line)
	{
		PathObject* path = new PathObject(this);
		path->addCoordinate(0, MapCoord(-max_icon_mm_half, max_icon_mm_half));
		path->addCoordinate(1, MapCoord(max_icon_mm_half, -max_icon_mm_half));
		object = path;
	}
	else if (type == Area)
	{
		PathObject* path = new PathObject(this);
		path->addCoordinate(0, MapCoord(-max_icon_mm_half, -max_icon_mm_half));
		path->addCoordinate(1, MapCoord(max_icon_mm_half, -max_icon_mm_half));
		path->addCoordinate(2, MapCoord(max_icon_mm_half, max_icon_mm_half));
		path->addCoordinate(3, MapCoord(-max_icon_mm_half, max_icon_mm_half));
		object = path;
	}
	else if (type == Text)
	{
		TextObject* text = new TextObject(this);
		text->setAnchorPosition(0, 0);
		text->setHorizontalAlignment(TextObject::AlignHCenter);
		text->setVerticalAlignment(TextObject::AlignVCenter);
		text->setText("A");
		object = text;
	}
	else if (type == Combined)
	{
		PathObject* path = new PathObject(this);
		for (int i = 0; i < 5; ++i)
			path->addCoordinate(i, MapCoord(sin(2*M_PI * i/5.0) * max_icon_mm_half, -cos(2*M_PI * i/5.0) * max_icon_mm_half));
		path->getPart(0).setClosed(true, false);
		object = path;
	}
	else
		assert(false);
	
	icon_map.addObject(object);
	
	qreal real_icon_mm_half = qMax(object->getExtent().right(), object->getExtent().bottom());
	real_icon_mm_half = qMax(real_icon_mm_half, -object->getExtent().left());
	real_icon_mm_half = qMax(real_icon_mm_half, -object->getExtent().top());
	if (real_icon_mm_half > max_icon_mm_half)
		view.setZoom(best_zoom * (max_icon_mm_half / real_icon_mm_half));
	
	painter.translate(0.5f * (icon_size-1), 0.5f * (icon_size-1));
	view.applyTransform(&painter);
	icon_map.draw(&painter, QRectF(-10000, -10000, 20000, 20000), false, view.calculateFinalZoomFactor(), true);
	
	painter.end();
	
	return icon;
}

QString Symbol::getNumberAsString() const
{
	QString str = "";
	for (int i = 0; i < number_components; ++i)
	{
		if (number[i] < 0)
			break;
		if (!str.isEmpty())
			str += ".";
		str += QString::number(number[i]);
	}
	return str;
}

Symbol* Symbol::getSymbolForType(Symbol::Type type)
{
	if (type == Symbol::Point)
		return new PointSymbol();
	else if (type == Symbol::Line)
		return new LineSymbol();
	else if (type == Symbol::Area)
		return new AreaSymbol();
	else if (type == Symbol::Text)
		return new TextSymbol();
	else if (type == Symbol::Combined)
		return new CombinedSymbol();
	else
	{
		assert(false);
		return NULL;
	}
}

void Symbol::duplicateImplCommon(Symbol* other)
{
	type = other->type;
	name = other->name;
	for (int i = 0; i < number_components; ++i)
		number[i] = other->number[i];
	description = other->description;
	is_helper_symbol = other->is_helper_symbol;
	is_hidden = other->is_hidden;
	if (other->icon)
		icon = new QImage(*other->icon);
	else
		icon = NULL;
}

// ### SymbolDropDown ###

SymbolDropDown::SymbolDropDown(Map* map, int filter, Symbol* initial_symbol, Symbol* excluded_symbol, QWidget* parent): QComboBox(), map(map)
{
	addItem(tr("- none -"), qVariantFromValue<void*>(NULL));
	
	int size = map->getNumSymbols();
	for (int i = 0; i < size; ++i)
	{
		Symbol* symbol = map->getSymbol(i);
		if (!(symbol->getType() & filter))
			continue;
		if (symbol == excluded_symbol)
			continue;
		if (symbol->getType() == Symbol::Combined)	// TODO: if point objects start to be able to contain objects of other ordinary symbols, add a check for these here, too, to prevent circular references
		{
			CombinedSymbol* combined_symbol = reinterpret_cast<CombinedSymbol*>(symbol);
			if (combined_symbol->containsSymbol(excluded_symbol))
				continue;
		}
		
		addItem(QPixmap::fromImage(*symbol->getIcon(map)), symbol->getName(), qVariantFromValue<void*>(symbol));
	}
	setSymbol(initial_symbol);
}
Symbol* SymbolDropDown::symbol()
{
	return reinterpret_cast<Symbol*>(itemData(currentIndex()).value<void*>());
}
void SymbolDropDown::setSymbol(Symbol* symbol)
{
	setCurrentIndex(findData(qVariantFromValue<void*>(symbol)));
}

#include "symbol.moc"
