/*
 *    Copyright 2024 Kai Pastor
 *    Copyright 2024 Matthias Kühlewein
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

#ifndef OPENORIENTEERING_MAP_INFORMATION_H
#define OPENORIENTEERING_MAP_INFORMATION_H

#include <vector>

#include <QString>


namespace OpenOrienteering {

class Map;

class MapInformation
{
public:
	struct TreeItem {
		int level;              ///< Depth in the hierarchy
		QString label;          ///< Display label
		QString value = {};     ///< Auxiliary value
	};
	
	/**
	 * Constructs the map information object.
	 */
	explicit MapInformation(const Map* map);
	
	/**
	 * A sequence which defines a hierarchy of map information in text form.
	 */
	const std::vector<TreeItem>& treeItems() const { return tree_items; }
	
	/**
	 * Create a text report.
	 */
	QString makeTextReport(int indent = 2) const;
	
private:
	std::vector<TreeItem> tree_items;
};

}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_MAP_INFORMATION_H
