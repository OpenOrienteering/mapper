/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2017 Kai Pastor
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


#ifndef OPENORIENTEERING_LINE_SYMBOL_SETTINGS_H
#define OPENORIENTEERING_LINE_SYMBOL_SETTINGS_H

#include <vector>

#include <QObject>

#include "gui/symbols/symbol_properties_widget.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QGridLayout;
class QLabel;
class QScrollArea;
class QSpinBox;
class QWidget;

namespace OpenOrienteering {

class ColorDropDown;
class LineSymbol;
struct LineSymbolBorder;
class Map;
class Symbol;
class SymbolSettingDialog;


class LineSymbolSettings : public SymbolPropertiesWidget
{
Q_OBJECT
public:
	LineSymbolSettings(LineSymbol* symbol, SymbolSettingDialog* dialog);
	
	~LineSymbolSettings() override;
	
	void reset(Symbol* symbol) override;
	
	/**
	 * Updates the content and state of all input fields.
	 */
	void updateContents();
	
protected:
	struct BorderWidgets
	{
		std::vector<QWidget*> widget_list;
		QDoubleSpinBox* width_edit;
		ColorDropDown* color_edit;
		QDoubleSpinBox* shift_edit;
		QCheckBox* dashed_check;
		
		std::vector<QWidget*> dash_widget_list;
		QDoubleSpinBox* dash_length_edit;
		QDoubleSpinBox* break_length_edit;
	};
	
	/**
	 * Creates the widgets for one border.
	 */
	void createBorderWidgets(LineSymbolBorder& border, Map* map, int& row, int col, QGridLayout* layout, BorderWidgets& widgets);
	
	/**
	 * Updates the border settings from the values in the widgets.
	 */
	void updateBorder(LineSymbolBorder& border, BorderWidgets& widgets);
	
	void updateBorderContents(LineSymbolBorder& border, BorderWidgets& widgets);
	
	/** 
	 * Ensures that a particular widget is visible in the scoll area. 
	 */
	void ensureWidgetVisible(QWidget* widget);
	
	/** 
	 * Adjusts the visibility and enabled state of all UI parts.
	 * There is a large number of dependencies between different elements
	 * of the line settings. This method handles them all.
	 */
	void updateStates();
	
protected slots:
	/** 
	 * Notifies this settings widget that one of the symbols has been modified.
	 */
	void pointSymbolEdited();
	
	void widthChanged(double value);
	void colorChanged();
	void minimumDimensionsEdited();
	void lineCapChanged(int index);
	void lineJoinChanged(int index);
	void pointedLineCapLengthChanged(double value);
	void dashedChanged(bool checked);
	void segmentLengthChanged(double value);
	void endLengthChanged(double value);
	void showAtLeastOneSymbolChanged(bool checked);
	void dashLengthChanged(double value);
	void breakLengthChanged(double value);
	void dashGroupsChanged(int index);
	void inGroupBreakLengthChanged(double value);
	void halfOuterDashesChanged(bool checked);
	void midSymbolPlacementChanged(int index);
	void midSymbolsPerDashChanged(int value);
	void midSymbolDistanceChanged(double value);
	void borderCheckClicked(bool checked);
	void differentBordersClicked(bool checked);
	void borderChanged();
	void suppressDashSymbolClicked(bool checked);
	void scaleDashSymbolClicked(bool checked);
	
private slots:
	/** Ensure that a predetermined widget is visible in the scoll area.
	 *  The widget is set in advance by ensureWidgetVisible(QWidget* widget).
	 */
	void ensureWidgetVisible();
	
private:
	LineSymbol* symbol;
	SymbolSettingDialog* dialog;
	
	QDoubleSpinBox* width_edit;
	ColorDropDown* color_edit;
	QDoubleSpinBox* minimum_length_edit;
	
	// enabled if line_width > 0 && color
	std::vector<QWidget*> line_settings_list;
	QComboBox* line_cap_combo;
	QComboBox* line_join_combo;
	QLabel* pointed_cap_length_label;
	QDoubleSpinBox* pointed_cap_length_edit;
	QCheckBox* dashed_check;
	
	// dashed == false && mid_symbol
	std::vector<QWidget*> undashed_widget_list;
	QDoubleSpinBox* segment_length_edit;
	QDoubleSpinBox* end_length_edit;
	QCheckBox* show_at_least_one_symbol_check;
	QSpinBox* minimum_mid_symbol_count_edit;
	QSpinBox* minimum_mid_symbol_count_when_closed_edit;
	
	// dashed == true
	std::vector<QWidget*> dashed_widget_list;
	QDoubleSpinBox* dash_length_edit;
	QDoubleSpinBox* break_length_edit;
	QComboBox* dash_group_combo;
	QLabel* in_group_break_length_label;
	QDoubleSpinBox* in_group_break_length_edit;
	QCheckBox* half_outer_dashes_check;
	
	// mid_symbol
	std::vector<QWidget*> mid_symbol_widget_list;
	QComboBox* mid_symbol_placement_combo;
	QSpinBox* mid_symbol_per_spot_edit;
	QLabel* mid_symbol_distance_label;
	QDoubleSpinBox* mid_symbol_distance_edit;
	
	// enabled if line_width > 0
	std::vector<QWidget*> border_widget_list;
	QCheckBox* border_check;
	QCheckBox* different_borders_check;
	std::vector<QWidget*> different_borders_widget_list;
	BorderWidgets border_widgets;
	BorderWidgets right_border_widgets;
	
	QCheckBox* supress_dash_symbol_check;
	QCheckBox* scale_dash_symbol_check;
	
	QScrollArea* scroll_area;
	QWidget* widget_to_ensure_visible;
};


}  // namespace OpenOrienteering

#endif
