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
#include "symbol_properties_widget.h"
#include "symbol_point_editor.h"
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
Symbol* PointSymbol::duplicate() const
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
		new_point->objects[i]->setSymbol(new_point->symbols[i], true);
	}
	
	return new_point;
}

void PointSymbol::createRenderables(Object* object, const MapCoordVector& flags, const MapCoordVectorF& coords, RenderableVector& output)
{
	createRenderablesScaled(object, flags, coords, output, 1.0f);
}
void PointSymbol::createRenderablesScaled(Object* object, const MapCoordVector& flags, const MapCoordVectorF& coords, RenderableVector& output, float coord_scale)
{
	if (inner_color && inner_radius > 0)
		output.push_back(new DotRenderable(this, coords[0]));
	if (outer_color && outer_width > 0)
		output.push_back(new CircleRenderable(this, coords[0]));
	
	PointObject* point = reinterpret_cast<PointObject*>(object);
	float rotation = -point->getRotation();
	double offset_x = coords[0].getX();
	double offset_y = coords[0].getY();
	
	// Add elements which possibly need to be moved and rotated
	int size = (int)objects.size();
	for (int i = 0; i < size; ++i)
	{
		const MapCoordVector& object_flags = objects[i]->getRawCoordinateVector();
		int coords_size = (int)object_flags.size();
		MapCoordVectorF transformed_coords;
		transformed_coords.resize(coords_size);
		
		if (rotation == 0)
		{
			for (int c = 0; c < coords_size; ++c)
			{
				transformed_coords[c] = MapCoordF(coord_scale * object_flags[c].xd() + offset_x,
												   coord_scale * object_flags[c].yd() + offset_y);
			}
		}
		else
		{
			float cosr = cos(rotation);
			float sinr = sin(rotation);
			
			for (int c = 0; c < coords_size; ++c)
			{
				float ox = coord_scale * object_flags[c].xd();
				float oy = coord_scale * object_flags[c].yd();
				transformed_coords[c] = MapCoordF(ox * cosr - oy * sinr + offset_x,
												   oy * cosr + ox * sinr + offset_y);
			}
		}
		
		// TODO: if this point is rotated, it has to pass it on to its children to make it work that rotatable point objects can be children.
		// But currently only basic, rotationally symmetric points can be children, so it does not matter for now.
		symbols[i]->createRenderables(objects[i], object_flags, transformed_coords, output);
	}
}

int PointSymbol::getNumElements() const
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
const Object* PointSymbol::getElementObject(int pos) const
{
    return objects[pos];
}
Symbol* PointSymbol::getElementSymbol(int pos)
{
	return symbols[pos];
}
const Symbol* PointSymbol::getElementSymbol(int pos) const
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

bool PointSymbol::isEmpty() const
{
	return getNumElements() == 0 && (inner_color == NULL || inner_radius == 0) && (outer_color == NULL || outer_width == 0);
}
bool PointSymbol::isSymmetrical() const
{
	int num_elements = (int)objects.size();
	for (int i = 0; i < num_elements; ++i)
	{
		if (symbols[i]->getType() != Symbol::Point)
			return false;
		PointObject* point = reinterpret_cast<PointObject*>(objects[i]);
		if (point->getCoordF() != MapCoordF(0, 0))
			return false;
	}
	return true;
}

void PointSymbol::colorDeleted(Map* map, int pos, MapColor* color)
{
	bool change = false;
	
	if (color == inner_color)
	{
		inner_color = NULL;
		change = true;
	}
	if (color == outer_color)
	{
		outer_color = NULL;
		change = true;
	}
	
	int num_elements = (int)objects.size();
	for (int i = 0; i < num_elements; ++i)
	{
		symbols[i]->colorDeleted(map, pos, color);
		change = true;
	}
	
	if (change)
		getIcon(map, true);
}
bool PointSymbol::containsColor(MapColor* color)
{
	if (color == inner_color)
		return true;
	if (color == outer_color)
		return true;
	
	int num_elements = (int)objects.size();
	for (int i = 0; i < num_elements; ++i)
	{
		if (symbols[i]->containsColor(color))
			return true;
	}
	
	return false;
}

void PointSymbol::scale(double factor)
{
	inner_radius = qRound(inner_radius * factor);
	outer_width = qRound(outer_width * factor);
	
	int size = (int)objects.size();
	for (int i = 0; i < size; ++i)
	{
		symbols[i]->scale(factor);
		objects[i]->scale(factor);
	}
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
bool PointSymbol::loadImpl(QFile* file, int version, Map* map)
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
		if (!symbols[i]->load(file, version, map))
			return false;
		
		file->read((char*)&save_type, sizeof(int));
		objects[i] = Object::getObjectForType(static_cast<Object::Type>(save_type), symbols[i]);
		if (!objects[i])
			return false;
		objects[i]->load(file, version, NULL);
	}
	
	return true;
}

SymbolPropertiesWidget* PointSymbol::createPropertiesWidget(SymbolSettingDialog* dialog)
{
	return new PointSymbolSettings(this, dialog);
}


// ### PointSymbolSettings ###

PointSymbolSettings::PointSymbolSettings(PointSymbol* symbol, SymbolSettingDialog* dialog)
: SymbolPropertiesWidget(symbol, dialog), symbol(symbol)
{
	QCheckBox* oriented_to_north = new QCheckBox(tr("Always oriented to north (not rotatable)"));
	oriented_to_north->setChecked(!symbol->rotatable);
	connect(oriented_to_north, SIGNAL(clicked(bool)), this, SLOT(orientedToNorthClicked(bool)));
	
	symbol_editor = new PointSymbolEditorWidget(dialog->getPreviewController(), symbol, 0, true);
	connect(symbol_editor, SIGNAL(symbolEdited()), dialog, SLOT(updatePreview()) );
	
	QVBoxLayout* layout = new QVBoxLayout();
	layout->addWidget(oriented_to_north);
	layout->setAlignment(oriented_to_north, Qt::AlignLeft);
	layout->addWidget(symbol_editor);
	
	point_tab = new QWidget();
	point_tab->setLayout(layout);
	addPropertiesGroup(tr("Point symbol"), point_tab);
	
	connect(this, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
}

void PointSymbolSettings::orientedToNorthClicked(bool checked)
{
	symbol->rotatable = !checked;
	dialog->updatePreview();
}

void PointSymbolSettings::tabChanged(int index)
{
	symbol_editor->setEditorActive( currentWidget()==point_tab );
}


#include "symbol_point.moc"
