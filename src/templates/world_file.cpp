/*
 *    Copyright 2012 Thomas Schöps
 *    Copyright 2015-2017 Kai Pastor
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


#include "world_file.h"

#include <QFile>
#include <QTextStream>


WorldFile::WorldFile()
{
	loaded = false;
}


bool WorldFile::load(const QString& path)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		loaded = false;
		return false;
	}
	QTextStream text_stream(&file);
	
	bool ok = false;
	double numbers[6];
	for (int i = 0; i < 6; ++i)
	{
		double number = text_stream.readLine().toDouble(&ok);
		if (!ok)
		{
			file.close();
			loaded = false;
			return false;
		}
		numbers[i] = number;
	}
	
	file.close();
	
	pixel_to_world.setMatrix(
		numbers[0], numbers[2], numbers[4],
		numbers[1], numbers[3], numbers[5],
		0, 0, 1);
	pixel_to_world = pixel_to_world.transposed();
	loaded = true;
	return true;
}

bool WorldFile::tryToLoadForImage(const QString& image_path)
{
	int last_dot_index = image_path.lastIndexOf(QLatin1Char('.'));
	if (last_dot_index < 0)
		return false;
	QString path_without_ext = image_path.left(last_dot_index + 1);
	QString ext = image_path.right(image_path.size() - (last_dot_index + 1));
	if (ext.size() <= 2)
		return false;
	
	// Possibility 1: use first and last character from image filename extension and 'w'
	QString test_path = path_without_ext + ext[0] + ext[ext.size() - 1] + QLatin1Char('w');
	if (load(test_path))
		return true;
	
	// Possibility 2: append 'w' to original extension
	test_path = image_path + QLatin1Char('w');
	if (load(test_path))
		return true;
	
	// Possibility 3: replace original extension by 'wld'
	test_path = path_without_ext + QLatin1String("wld");
	if (load(test_path))
		return true;
	
	return false;
}

