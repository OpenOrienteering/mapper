/*
 *    Copyright 2013 Thomas Schöps
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


#ifndef _OPENORIENTEERING_TOOL_DISTRIBUTE_POINTS_H_
#define _OPENORIENTEERING_TOOL_DISTRIBUTE_POINTS_H_

#include <vector>

#include <QDialog>

class Map;
class PathObject;
class PointObject;
class PointSymbol;

QT_BEGIN_NAMESPACE
class QDoubleSpinBox;
class QCheckBox;
class QSpinBox;
QT_END_NAMESPACE

/**
 * Provides methods to create evenly spaced point objects along a line.
 */
class DistributePointsTool
{
public:
	/** Required user input for applying the action. */
	struct Settings
	{
		/** Number of points to create */
		int num_points_per_line;
		
		/** If true, points will also be placed at open paths ends,
		 *  otherwise only inside for open paths*/
		bool points_at_ends;
		
		/** If true, points will be aligned with the path direction if rotatable */
		bool rotate_symbols;
		
		/** Additional rotation for rotatable point symbols,
		 *  in radians counter-clockwise */
		double additional_rotation;
		
		/** Constructor, sets default values. */
		inline Settings()
		{
			num_points_per_line = 3;
			points_at_ends = true;
			rotate_symbols = true;
			additional_rotation = 0;
		}
	};
	
	/** Shows the settings dialog. If the user presses Ok, returns true
	 *  and writes the chosen values to settings, otherwise returns false.
	 *  settings is also used as initial values for the dialog.
	 *  The point symbol is used to determine the enabled state of some options. */
	static bool showSettingsDialog(QWidget* parent, PointSymbol* point, Settings& settings);
	
	/** Executes the tool in the map on the path,
	 *  creating points according to settings. Appends the created objects to
	 *  the out_objects vector, if set (for creating undo steps in the calling code).
	 *  You have to add the objects to a map yourself if you want to. */
	static void execute(PathObject* path, PointSymbol* point,
				 const Settings& settings, std::vector<PointObject*>* out_objects = NULL);
};

/** Settings dialog for DistributePointsTool */
class DistributePointsSettingsDialog : public QDialog
{
Q_OBJECT
public:
	/** Creates a new DistributePointsSettingsDialog. */
	DistributePointsSettingsDialog(QWidget* parent, PointSymbol* point,
								   DistributePointsTool::Settings& settings);
	
	/** After the dialog finished successfully, returns the entered values. */
	void getValues(DistributePointsTool::Settings& settings);
	
private slots:
	void updateWidgets();
	
private:
	QSpinBox* num_points_edit;
	QCheckBox* points_at_ends_check;
	
	QCheckBox* rotate_symbols_check;
	QDoubleSpinBox* additional_rotation_edit;
	
	Map* map;
};

#endif
