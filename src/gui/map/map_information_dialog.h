/*
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

#ifndef OPENORIENTEERING_MAP_INFORMATION_DIALOG_H
#define OPENORIENTEERING_MAP_INFORMATION_DIALOG_H

#include <memory>

#include <QDialog>
#include <QObject>
#include <QString>

namespace OpenOrienteering {

class MainWindow;
class Map;
class MapInformation;

/**
 * A class for providing information about the current map.
 * 
 * Information is given about the number of objects in total and per map part,
 * the number of symbols, templates and undo/redo steps.
 * For each color a list of symbols using that color is shown.
 * All fonts (and their substitutions) being used by symbols are shown.
 * For objects there is a hierarchical view:
 * - the number of objects per symbol class (e.g., Point symbols, Line symbols etc.)
 *   - for each symbol class the symbols in use and the related number of objects
 *     - for each symbol the colors used by it
 */
class MapInformationDialog : public QDialog
{
Q_OBJECT
public:
	/**
	 * Creates a new MapInformationDialog object.
	 */
	MapInformationDialog(MainWindow* parent, Map* map);
	
	~MapInformationDialog() override;
	
private slots:
	void save();
	
private:
	const MainWindow* main_window;
	
	std::unique_ptr<MapInformation> map_information;
};

}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_MAP_INFORMATION_DIALOG_H
