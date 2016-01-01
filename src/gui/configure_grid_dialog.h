/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2014 Kai Pastor
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


#ifndef _OPENORIENTEERING_CONFIGURE_GRID_DIALOG_H_
#define _OPENORIENTEERING_CONFIGURE_GRID_DIALOG_H_

#include <QDialog>

#include "../core/map_grid.h"

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QRadioButton;
QT_END_NAMESPACE

class Map;

class ConfigureGridDialog : public QDialog
{
Q_OBJECT
public:
	ConfigureGridDialog(QWidget* parent, const MapGrid& grid, bool grid_visible);
	
	~ConfigureGridDialog() override;
	
	const MapGrid& grid() const;
	
	bool gridVisible() const;
	
private slots:
	void chooseColor();
	void updateColorDisplay();
	void okClicked();
	void updateStates();
	void showHelp();
	
private:
	QCheckBox* show_grid_check;
	QCheckBox* snap_to_grid_check;
	QPushButton* choose_color_button;
	QComboBox* display_mode_combo;

	QRadioButton* mag_north_radio;
	QRadioButton* grid_north_radio;
	QRadioButton* true_north_radio;
	QDoubleSpinBox* additional_rotation_edit;
	
	QComboBox* unit_combo;
	QDoubleSpinBox* horz_spacing_edit;
	QDoubleSpinBox* vert_spacing_edit;
	QLabel* origin_label;
	QDoubleSpinBox* horz_offset_edit;
	QDoubleSpinBox* vert_offset_edit;
	
	MapGrid result_grid;
	bool grid_visible;
	QRgb current_color;
};

inline const MapGrid& ConfigureGridDialog::grid() const
{
	return result_grid;
}

inline bool ConfigureGridDialog::gridVisible() const
{
	return grid_visible;
}

#endif
