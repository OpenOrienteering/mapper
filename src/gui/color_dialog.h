/*
 *    Copyright 2012, 2013 Kai Pastor
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


#ifndef _OPENORIENTEERING_COLOR_DIALOG_H_
#define _OPENORIENTEERING_COLOR_DIALOG_H_

#include <qglobal.h>
#if QT_VERSION < 0x050000
#include <QtGui>
#else
#include <QtWidgets>
#endif

#include <vector>

#include "../core/map_color.h"

class ColorDropDown;
class Map;

/**
 * A dialog for editing a single map color.
 */
class ColorDialog: public QDialog
{
Q_OBJECT
public:
	/** Constructs a new dialog for the given map and color. */
	ColorDialog(const Map& map, const MapColor& source_color, QWidget* parent = 0, Qt::WindowFlags f = 0);
	
	/**
	 * Returns the edited color. 
	 */
	const MapColor& getColor() const { return color; }
	
protected slots:
	void accept();
	
	void reset();
	
	void showHelp();
	
	void spotColorTypeChanged(int id);
	
	void nameChanged();
	
	void spotColorCompositionChanged();
	
	void knockoutChanged();
	
	void cmykColorTypeChanged(int id);
	
	void cmykValueChanged();
	
	void rgbColorTypeChanged(int id);
	
	void rgbValueChanged();
	
	void setColorModified() { setColorModified(true); }
	
protected:
	void setColorModified(bool modified);
	
	void updateWidgets();
	
	void updateButtons();
	
	const Map& map;
	const MapColor& source_color;
	
	MapColor color;
	bool color_modified;
	
	bool react_to_changes;
	
	QLabel* color_preview_label;
	QLabel* mc_name_label;
	
	QRadioButton* full_tone_option;
	QRadioButton* composition_option;
	QLineEdit* sc_name_edit;
	QCheckBox* knockout_option;
	
	QRadioButton* cmyk_spot_color_option;
	QRadioButton* evaluate_rgb_option;
	QRadioButton* custom_cmyk_option;
	QDoubleSpinBox* c_edit;
	QDoubleSpinBox* m_edit;
	QDoubleSpinBox* y_edit;
	QDoubleSpinBox* k_edit;
	
	QRadioButton* rgb_spot_color_option;
	QRadioButton* evaluate_cmyk_option;
	QRadioButton* custom_rgb_option;
	QDoubleSpinBox* r_edit;
	QDoubleSpinBox* g_edit;
	QDoubleSpinBox* b_edit;
	QLineEdit* html_edit;
	
	QTabWidget* properties_widget;
	
	QAbstractButton* ok_button;
	QAbstractButton* reset_button;
	
	static const int icon_size = 32;
	
	std::vector< ColorDropDown* > component_colors;
	std::vector< QDoubleSpinBox* > component_halftone;
	int components_row0;
	int components_col0;
	QGridLayout* prof_color_layout;
	int stretch_row0;
	int stretch_col0;
	QWidget* stretch;
};

#endif
