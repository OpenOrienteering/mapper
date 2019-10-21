/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2019 Kai Pastor
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


#ifndef OPENORIENTEERING_TEXT_SYMBOL_SETTINGS_H
#define OPENORIENTEERING_TEXT_SYMBOL_SETTINGS_H

#include <QtGlobal>
#include <QObject>
#include <QString>

#include "gui/symbols/symbol_properties_widget.h"

class QCheckBox;
class QDoubleSpinBox;
class QFont;
class QFontComboBox;
class QLineEdit;
class QListWidget;
class QPushButton;
class QRadioButton;
class QWidget;

namespace OpenOrienteering {

class ColorDropDown;
class Symbol;
class SymbolSettingDialog;
class TextSymbol;


class TextSymbolSettings : public SymbolPropertiesWidget
{
Q_OBJECT
public:
	TextSymbolSettings(TextSymbol* symbol, SymbolSettingDialog* dialog);
    ~TextSymbolSettings() override;
	
	void reset(Symbol* symbol) override;
	
	void updateGeneralContents();
	
	void updateFramingContents();
	
	void updateCompatibilityCheckEnabled();
	
	void updateCompatibilityContents();
	
protected slots:
	void orientedToNorthClicked(bool checked);
	void fontChanged(const QFont& font);
	void fontSizeChanged(double value);
	void letterSizeChanged();
	void colorChanged();
	void checkToggled(bool checked);
	void spacingChanged(double value);
	void iconTextEdited(const QString& text);
	
	void framingCheckClicked(bool checked);
	void framingColorChanged();
	void framingModeChanged();
	void framingSettingChanged();
	
	void ocadCompatibilityButtonClicked(bool checked);
	void lineBelowCheckClicked(bool checked);
	void lineBelowSettingChanged();
	void customTabRowChanged(int row);
	void addCustomTabClicked();
	void removeCustomTabClicked();
	
protected:
	void updateFontSizeEdit();
	void updateLetterSizeEdit();
	qreal calculateLetterHeight() const;
	
private:
	TextSymbol* symbol;
	SymbolSettingDialog* dialog;
	
	QCheckBox* oriented_to_north;
	ColorDropDown*  color_edit;
	QFontComboBox*  font_edit;
	QDoubleSpinBox* font_size_edit;
	QLineEdit*      letter_edit;
	QDoubleSpinBox* letter_size_edit;
	QCheckBox*      bold_check;
	QCheckBox*      italic_check;
	QCheckBox*      underline_check;
	QDoubleSpinBox* line_spacing_edit;
	QDoubleSpinBox* paragraph_spacing_edit;
	QDoubleSpinBox* character_spacing_edit;
	QCheckBox*      kerning_check;
	QLineEdit*      icon_text_edit;
	QCheckBox*      framing_check;
	QCheckBox*      ocad_compat_check;
	
	QWidget*        framing_widget;
	ColorDropDown*  framing_color_edit;
	QRadioButton*   framing_line_radio;
	QDoubleSpinBox* framing_line_half_width_edit;
	QRadioButton*   framing_shadow_radio;
	QDoubleSpinBox* framing_shadow_x_offset_edit;
	QDoubleSpinBox* framing_shadow_y_offset_edit;
	
	QWidget*        ocad_compat_widget;
	QCheckBox*      line_below_check;
	QDoubleSpinBox* line_below_width_edit;
	ColorDropDown*  line_below_color_edit;
	QDoubleSpinBox* line_below_distance_edit;
	QListWidget*    custom_tab_list;
	QPushButton*    custom_tab_add;
	QPushButton*    custom_tab_remove;
	
	bool react_to_changes;
};


}  // namespace OpenOrienteering

#endif
