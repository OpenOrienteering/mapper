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


#include "symbol_point.h"

#include <QtGui>

#include "map.h"
#include "util.h"
#include "map_color.h"
#include "symbol_setting_dialog.h"
#include "object.h"

PointSymbol::PointSymbol() : Symbol(Symbol::Point)
{
	rotatable = false;
	inner_radius = 1000;
	inner_color = NULL;
	outer_width = 0;
	outer_color = NULL;
}
PointSymbol::~PointSymbol()
{
	int size = objects.size();
	for (int i = 0; i < size; ++i)
	{
		delete objects[i];
		delete symbols[i];
	}
}
Symbol* PointSymbol::duplicate()
{
	PointSymbol* new_point = new PointSymbol();
	new_point->duplicateImplCommon(this);
	
	new_point->rotatable = rotatable;
	new_point->inner_radius = inner_radius;
	new_point->inner_color = inner_color;
	new_point->outer_width = outer_width;
	new_point->outer_color = outer_color;
	
	new_point->symbols.resize(symbols.size());
	for (int i = 0; i < (int)symbols.size(); ++i)
		new_point->symbols[i] = symbols[i]->duplicate();
	
	new_point->objects.resize(objects.size());
	for (int i = 0; i < (int)objects.size(); ++i)
	{
		new_point->objects[i] = objects[i]->duplicate();
		new_point->objects[i]->setSymbol(new_point->symbols[i]);
	}
	
	return new_point;
}

void PointSymbol::createRenderables(Object* object, const MapCoordVectorF& coords, RenderableVector& output)
{
	if (inner_color && inner_radius > 0)
		output.push_back(new DotRenderable(this, coords[0]));
	if (outer_color && outer_width > 0)
		output.push_back(new CircleRenderable(this, coords[0]));
	
	PointObject* point = reinterpret_cast<PointObject*>(object);
	float rotation = point->getRotation();
	double offset_x = coords[0].getX();
	double offset_y = coords[0].getY();
	
	// Add elements which possibly need to be moved and rotated
	int size = (int)objects.size();
	for (int i = 0; i < size; ++i)
	{
		MapCoordVectorF transformed_coords;
		const MapCoordVector& original_coords = objects[i]->getCoordinateVector();
		
		int coords_size = original_coords.size();
		transformed_coords.resize(coords_size);
		
		if (rotation == 0)
		{
			for (int c = 0; c < coords_size; ++c)
			{
				transformed_coords[c] = MapCoordF(original_coords[c].xd() + offset_x,
												  original_coords[c].yd() + offset_y);
			}
		}
		else
		{
			float cosr = cos(rotation);
			float sinr = sin(rotation);
			
			for (int c = 0; c < coords_size; ++c)
			{
				float ox = original_coords[c].xd();
				float oy = original_coords[c].yd();
				transformed_coords[c] = MapCoordF(ox * cosr - oy * sinr + offset_x,
												  oy * cosr + ox * sinr + offset_y);
			}
		}
		
		// TODO: if this point is rotated, it has to pass it on to its children to make it work that rotatable point objects can be children.
		// But currently only basic, rotationally symmetric points can be children, so it does not matter for now.
		symbols[i]->createRenderables(objects[i], transformed_coords, output);
	}
}

int PointSymbol::getNumElements()
{
	return (int)objects.size();
}
void PointSymbol::addElement(int pos, Object* object, Symbol* symbol)
{
	objects.insert(objects.begin() + pos, object);
	symbols.insert(symbols.begin() + pos, symbol);
}
Object* PointSymbol::getElementObject(int pos)
{
	return objects[pos];
}
Symbol* PointSymbol::getElementSymbol(int pos)
{
	return symbols[pos];
}
void PointSymbol::deleteElement(int pos)
{
	delete objects[pos];
	objects.erase(objects.begin() + pos);
	delete symbols[pos];
	symbols.erase(symbols.begin() + pos);
}

void PointSymbol::colorDeleted(int pos, MapColor* color)
{
	if (color == inner_color)
		inner_color = NULL;
	if (color == outer_color)
		outer_color = NULL;
}

void PointSymbol::saveImpl(QFile* file, Map* map)
{
	file->write((const char*)&rotatable, sizeof(bool));
	
	file->write((const char*)&inner_radius, sizeof(int));
	int temp = map->findColorIndex(inner_color);
	file->write((const char*)&temp, sizeof(int));
	
	file->write((const char*)&outer_width, sizeof(int));
	temp = map->findColorIndex(outer_color);
	file->write((const char*)&temp, sizeof(int));
	
	int num_elements = (int)objects.size();
	file->write((const char*)&num_elements, sizeof(int));
	for (int i = 0; i < num_elements; ++i)
	{
		int save_type = static_cast<int>(symbols[i]->getType());
		file->write((const char*)&save_type, sizeof(int));
		symbols[i]->save(file, map);
		
		save_type = static_cast<int>(objects[i]->getType());
		file->write((const char*)&save_type, sizeof(int));
		objects[i]->save(file);
	}
}
bool PointSymbol::loadImpl(QFile* file, Map* map)
{
	file->read((char*)&rotatable, sizeof(bool));
	
	file->read((char*)&inner_radius, sizeof(int));
	int temp;
	file->read((char*)&temp, sizeof(int));
	inner_color = (temp >= 0) ? map->getColor(temp) : NULL;
	
	file->read((char*)&outer_width, sizeof(int));
	file->read((char*)&temp, sizeof(int));
	outer_color = (temp >= 0) ? map->getColor(temp) : NULL;
	
	int num_elements;
	file->read((char*)&num_elements, sizeof(int));
	objects.resize(num_elements);
	symbols.resize(num_elements);
	for (int i = 0; i < num_elements; ++i)
	{
		int save_type;
		file->read((char*)&save_type, sizeof(int));
		symbols[i] = Symbol::getSymbolForType(static_cast<Symbol::Type>(save_type));
		if (!symbols[i])
			return false;
		if (!symbols[i]->load(file, map))
			return false;
		
		file->read((char*)&save_type, sizeof(int));
		objects[i] = Object::getObjectForType(static_cast<Object::Type>(save_type), NULL, symbols[i]);
		if (!objects[i])
			return false;
		objects[i]->load(file);
	}
	
	return true;
}

// ### PointSymbolSettings ###

PointSymbolSettings::PointSymbolSettings(PointSymbol* symbol, Map* map, SymbolSettingDialog* parent) : QGroupBox(tr("Point settings"), parent), dialog(parent)
{
	this->symbol = symbol;
	
	oriented_to_north_check = new QCheckBox(tr("Always oriented to north (not rotatable)"));
	oriented_to_north_check->setChecked(!symbol->rotatable);
	
	QVBoxLayout* layout = new QVBoxLayout();
	layout->addWidget(oriented_to_north_check);
	layout->setAlignment(oriented_to_north_check, Qt::AlignLeft);
	
	setLayout(layout);
	
	connect(oriented_to_north_check, SIGNAL(clicked(bool)), this, SLOT(orientedToNorthClicked(bool)));
}
void PointSymbolSettings::orientedToNorthClicked(bool checked)
{
	symbol->rotatable = !checked;
}

#include "symbol_point.moc"
