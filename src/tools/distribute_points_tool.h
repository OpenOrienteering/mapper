/*
 *    Copyright 2013 Thomas Schöps
 *    Copyright 2015 Kai Pastor
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


#ifndef OPENORIENTEERING_TOOL_DISTRIBUTE_POINTS_H
#define OPENORIENTEERING_TOOL_DISTRIBUTE_POINTS_H

#include <vector>

#include <QDialog>
#include <QObject>

class QDoubleSpinBox;
class QCheckBox;
class QSpinBox;
class QWidget;

namespace OpenOrienteering {

class PathObject;
class PointObject;
class PointSymbol;


/**
 * Provides methods to create evenly spaced point objects along a line.
 * 
 * \todo Integrate implementation of MapEditorController::distributePointsClicked().
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
		constexpr Settings()
		 : num_points_per_line{ 3 }
		 , points_at_ends{ true }
		 , rotate_symbols{ true }
		 , additional_rotation{ 0.0 }
		{
			// Nothing else
		}
	};
	
	/**
	 * Shows a settings dialog for the tool.
	 * 
	 * If the user presses Ok, returns true and saves the chosen values to
	 * settings, otherwise returns false.
	 * 
	 * The settings parameter is also used as initial values for the dialog.
	 * The point symbol is used to determine the enabled state of some options.
	 */
	static bool showSettingsDialog(
	        QWidget* parent,
	        const PointSymbol* point,
	        DistributePointsTool::Settings& settings
	);
	
	/** 
	 * Executes the tool on the path, creating points according to settings.
	 * 
	 * Appends the created objects to the out_objects vector, but does not add
	 * them to the map.
	 */
	static void execute(
	        const PathObject* path,
	        PointSymbol* point,
	        const DistributePointsTool::Settings& settings,
	        std::vector<PointObject*>& out_objects
	);
};



/**
 * Settings dialog for DistributePointsTool
 */
class DistributePointsSettingsDialog : public QDialog
{
Q_OBJECT
public:
	/** Creates a new DistributePointsSettingsDialog. */
	DistributePointsSettingsDialog(
	        QWidget* parent,
	        const PointSymbol* point,
	        const DistributePointsTool::Settings& settings
	);
	
	/** After the dialog finished successfully, returns the entered values. */
	void getValues(DistributePointsTool::Settings& settings);
	
private:
	QSpinBox* num_points_edit;
	QCheckBox* points_at_ends_check;
	
	QCheckBox* rotate_symbols_check;
	QDoubleSpinBox* additional_rotation_edit;
};


}  // namespace OpenOrienteering
#endif
