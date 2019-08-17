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
	
	/// The six world file parameters, order as in the text file.
	double parameters[6];
	
	/// Creates an unloaded world file
	WorldFile() noexcept;
	
	/// Creates a world file with the given parameters (in-order).
	WorldFile(double xw, double xh, double yw, double yh, double dx, double dy) noexcept;

	/// Creates a loaded world file from a QTransform
	explicit WorldFile(const QTransform& wld) noexcept;
	
	WorldFile(const WorldFile&) noexcept = default;
	WorldFile(WorldFile&&) noexcept = default;
	~WorldFile() = default;
	
	WorldFile& operator=(const WorldFile&) noexcept = default;
	WorldFile& operator=(WorldFile&&) noexcept = default;
	
	/// Returns a QTransform from pixels to projected coordinates,
	/// with (0,0) being the top-left corner of the top left pixel.
	operator QTransform() const;

	/// Tries to load the given path as world file.
	/// Returns true on success and sets loaded to true or false.
	bool load(const QString& path);

	/// Writes the world file to the given path.
	bool save(const QString& path) const;
	
	/// Tries to find and load a world file for the given image path.
	bool tryToLoadForImage(const QString& image_path);
	
	/// Returns the proposed world file path for the given image path.
	static QString pathForImage(const QString& image_path);
};


}  // namespace OpenOrienteering

#endif
