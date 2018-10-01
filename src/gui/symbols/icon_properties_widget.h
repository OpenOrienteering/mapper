/*
 *    Copyright 2018 Kai Pastor
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


#ifndef OPENORIENTEERING_ICON_PROPERTIES_WIDGET_H
#define OPENORIENTEERING_ICON_PROPERTIES_WIDGET_H

#include <QWidget>

#include "gui/symbols/symbol_properties_widget.h"

class QAbstractButton;
class QLabel;
class QSpinBox;
class QLineEdit;

namespace OpenOrienteering {

class Map;
class Symbol;


/**
 * A widget for modifying symbol icons.
 */
class IconPropertiesWidget : public QWidget
{
	Q_OBJECT
	
public:
	explicit IconPropertiesWidget(Symbol* symbol, SymbolSettingDialog* dialog = nullptr);
	
	~IconPropertiesWidget() override;
	
	void reset(Symbol* symbol);
	
signals:
	void iconModified();
	
protected:
	void updateWidgets();
	
	void sizeEdited(int size);
	
	void copyClicked();
	
	void saveClicked();
	
	void loadClicked();
	
	void clearClicked();
	
private:
	SymbolSettingDialog* const dialog;
	Symbol* symbol;
	
	QLabel* default_icon_display;
	QSpinBox* default_icon_size_edit;
	QLabel* custom_icon_display;
	QLineEdit* custom_icon_size_edit;
	QAbstractButton* save_default_button;
	QAbstractButton* copy_default_button;
	QAbstractButton* save_button;
	QAbstractButton* load_button;
	QAbstractButton* clear_button;
};


}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_ICON_PROPERTIES_WIDGET_H
