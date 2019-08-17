/*
 *    Copyright 2012 Thomas Schöps
 *    Copyright 2015-2019 Kai Pastor
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

#include <QChar>
#include <QFile>
#include <QFileDevice>
#include <QFileInfo>
#include <QIODevice>
#include <QLatin1Char>
#include <QLatin1String>
#include <QTextStream>


namespace OpenOrienteering {

WorldFile::WorldFile() noexcept
: parameters { 1, 0, 0, 1, 0, 0 }
{
	loaded = false;
}

WorldFile::WorldFile(double xw, double xh, double yw, double yh, double dx, double dy) noexcept
: parameters { xw, xh, yw, yh, dx, dy }
{
	loaded = true;
}

WorldFile::WorldFile(const QTransform& wld) noexcept
: loaded { true }
, parameters { wld.m11(), wld.m12(), wld.m21(), wld.m22(), wld.m31(), wld.m32() }
{
	// nothing else
}


WorldFile::operator QTransform() const
{
	return {
	  parameters[0], parameters[1], 0,
	  parameters[2], parameters[3], 0,
	  parameters[4], parameters[5], 1
	};
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
	for (auto& parameter : parameters)
	{
		parameter = text_stream.readLine().toDouble(&ok);
		if (!ok)
		{
			file.close();
			loaded = false;
			return false;
		}
	}
	
	file.close();
	loaded = true;
	return true;
}

bool WorldFile::save(const QString &path) const
{
	if (!loaded) { return false;}

	QFile file(path);
	if (file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QTextStream stream(&file);
		stream.setRealNumberPrecision(10);
		for (auto value : parameters)
			stream << value << endl;
		file.close();
	}
	return file.error() == QFileDevice::NoError;
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
		return true; // NOLINT
	
	return false;
}

QString WorldFile::pathForImage(const QString& image_path)
{
	// Just use QFileInfo rather than rolling our own path processing.
	const QFileInfo info(image_path);
	const QString base = info.path() + QLatin1Char('/') + info.completeBaseName(); 
	const auto suffix = info.suffix();
	switch (suffix.length())
	{
	case 0:
		// If there is no suffix, append ".wld"
		return base + QLatin1String(".wld");
	case 3:
		// If there are three chars, remove middle char and append 'w'
		return base + QLatin1Char('.') + suffix[0] + suffix[2] + QLatin1Char('w');
	default:
		// Otherwise, just append 'w'
		return base + QLatin1Char('.') + suffix + QLatin1Char('w');
	}
}


}  // namespace OpenOrienteering
