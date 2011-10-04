/*
 *    Copyright 2011 Thomas Schöps
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


#include "map.h"

#include <assert.h>

#include <QMessageBox>

#include "map_editor.h"
#include <QFile>

// ### Map::Color ###

void Map::Color::updateFromCMYK()
{
	color.setCmykF(c, m, y, k);
	qreal rr, rg, rb;
	color.getRgbF(&rr, &rg, &rb);
	r = rr;
	g = rg;
	b = rb;
	assert(color.spec() == QColor::Cmyk);
}
void Map::Color::updateFromRGB()
{
	color.setRgbF(r, g, b);
	color.convertTo(QColor::Cmyk);
	qreal rc, rm, ry, rk;
	color.getCmykF(&rc, &rm, &ry, &rk, NULL);
	c = rc;
	m = rm;
	y = ry;
	k = rk;
}

// ### Map ###

Map::Map()
{
	colors_dirty = false;
	unsaved_changes = false;
}
Map::~Map()
{
	
}

bool Map::saveTo(const QString& path)
{
	QFile file(path);
	if (!file.open(QIODevice::WriteOnly))
	{
		QMessageBox::warning(NULL, tr("Error"), tr("Cannot open file:\n%1\nfor writing.").arg(path));
		return false;
	}
	
	// Basic stuff
	const char FILE_TYPE_ID[4] = {0x4F, 0x4D, 0x41, 0x50};	// "OMAP"
	file.write(FILE_TYPE_ID, 4);
	
	const int FILE_VERSION_ID = 0;
	file.write((const char*)&FILE_VERSION_ID, sizeof(int));
	
	// Write colors
	int num_colors = (int)colors.size();
	file.write((const char*)&num_colors, sizeof(int));
	
	for (int i = 0; i < num_colors; ++i)
	{
		Color* color = colors[i];
		
		file.write((const char*)&color->priority, sizeof(int));
		file.write((const char*)&color->c, sizeof(float));
		file.write((const char*)&color->m, sizeof(float));
		file.write((const char*)&color->y, sizeof(float));
		file.write((const char*)&color->k, sizeof(float));
		file.write((const char*)&color->opacity, sizeof(float));
		
		saveString(&file, color->name);
	}
	
	file.close();
	
	colors_dirty = false;
	unsaved_changes = false;
	return true;
}
void Map::saveString(QFile* file, const QString& str)
{
	int length = str.length();
	
	file->write((const char*)&length, sizeof(int));
	file->write((const char*)str.constData(), length * sizeof(QChar));
}
void Map::loadString(QFile* file, QString& str)
{
	int length;
	
	file->read((char*)&length, sizeof(int));
	if (length > 0)
	{
		str.resize(length);
		file->read((char*)str.data(), length * sizeof(QChar));
	}
	else
		str = "";
}
bool Map::loadFrom(const QString& path)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly))
	{
		QMessageBox::warning(NULL, tr("Error"), tr("Cannot open file:\n%1\nfor reading.").arg(path));
		return false;
	}
	
	char buffer[256];
	
	// Basic stuff
	const char FILE_TYPE_ID[4] = {0x4F, 0x4D, 0x41, 0x50};	// "OMAP"
	file.read(buffer, 4);
	for (int i = 0; i < 4; ++i)
	{
		if (buffer[i] != FILE_TYPE_ID[i])
		{
			QMessageBox::warning(NULL, tr("Error"), tr("Cannot open file:\n%1\n\nInvalid file type.").arg(path));
			return false;
		}
	}
	
	int FILE_VERSION_ID;
	file.read((char*)&FILE_VERSION_ID, sizeof(int));
	if (FILE_VERSION_ID != 0)
		QMessageBox::warning(NULL, tr("Warning"), tr("Problem while opening file:\n%1\n\nUnknown file format version.").arg(path));
	
	// Load colors
	int num_colors;
	file.read((char*)&num_colors, sizeof(int));
	colors.resize(num_colors);
	
	for (int i = 0; i < num_colors; ++i)
	{
		Color* color = new Color();
		
		file.read((char*)&color->priority, sizeof(int));
		file.read((char*)&color->c, sizeof(float));
		file.read((char*)&color->m, sizeof(float));
		file.read((char*)&color->y, sizeof(float));
		file.read((char*)&color->k, sizeof(float));
		file.read((char*)&color->opacity, sizeof(float));
		color->updateFromCMYK();
		
		loadString(&file, color->name);
		
		colors[i] = color;
	}
	
	file.close();
	return true;
}

void Map::addMapWidget(MapWidget* widget)
{
	widgets.push_back(widget);
}
void Map::removeMapWidget(MapWidget* widget)
{
	for (int i = 0; i < (int)widgets.size(); ++i)
	{
		if (widgets[i] == widget)
		{
			widgets.erase(widgets.begin() + i);
			return;
		}
	}
	assert(false);
}
void Map::updateAllMapWidgets()
{
	for (int i = 0; i < (int)widgets.size(); ++i)
		widgets[i]->update();
}

void Map::setColor(Map::Color* color, int pos)
{
	colors[pos] = color;
	color->priority = pos;
}
Map::Color* Map::addColor(int pos)
{
	Color* new_color = new Color();
	new_color->name = tr("New color");
	new_color->priority = pos;
	new_color->c = 0;
	new_color->m = 0;
	new_color->y = 0;
	new_color->k = 1;
	new_color->updateFromCMYK();
	
	colors.insert(colors.begin() + pos, new_color);
	checkIfFirstColorAdded();
	return new_color;
}
void Map::addColor(Map::Color* color, int pos)
{
	colors.insert(colors.begin() + pos, color);
	checkIfFirstColorAdded();
	color->priority = pos;
}
void Map::deleteColor(int pos)
{
	colors.erase(colors.begin() + pos);
	for (int i = pos; i < (int)colors.size(); ++i)
		colors[i]->priority = i;
	
	if (getNumColors() == 0)
	{
		// That was the last color - the help text in the map widget(s) should be updated
		updateAllMapWidgets();
	}
}
void Map::setColorsDirty()
{
	if (!colors_dirty && !unsaved_changes)
	{
		emit(gotUnsavedChanges());
		unsaved_changes = true;
	}
	colors_dirty = true;
}

void Map::checkIfFirstColorAdded()
{
	if (getNumColors() == 1)
	{
		// This is the first color - the help text in the map widget(s) should be updated
		updateAllMapWidgets();
	}
}

// ### MapView ###

MapView::MapView(Map* map) : map(map)
{

}

#include "map.moc"
