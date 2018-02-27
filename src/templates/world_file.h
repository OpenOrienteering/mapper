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


#ifndef OPENORIENTEERING_WORLD_FILE_H
#define OPENORIENTEERING_WORLD_FILE_H

#include <QString>
#include <QTransform>


namespace OpenOrienteering {

/**
 * Handles pixel-to-world transformations given by world files.
 * 
 * \see https://en.wikipedia.org/wiki/World_file
 */
struct WorldFile
{
	bool loaded;
	QTransform pixel_to_world;
	
	/// Creates an unloaded world file
	WorldFile();

	/// Creates a loaded world file from a QTransform
	WorldFile(const QTransform& wld);

	/// Tries to load the given path as world file.
	/// Returns true on success and sets loaded to true or false.
	bool load(const QString& path);

	/// Writes the world file to the given path.
	bool save(const QString& path);
	
	/// Tries to find and load a world file for the given image path.
	bool tryToLoadForImage(const QString& image_path);
};


}  // namespace OpenOrienteering

#endif
