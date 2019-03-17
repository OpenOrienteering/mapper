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


#ifndef OPENORIENTEERING_POINT_SYMBOL_SETTINGS_H
#define OPENORIENTEERING_POINT_SYMBOL_SETTINGS_H

#include <QObject>
#include <QString>

#include "gui/symbols/symbol_properties_widget.h"

class QVBoxLayout;
class QWidget;

namespace OpenOrienteering {

class PointSymbol;
class PointSymbolEditorWidget;
class Symbol;
class SymbolSettingDialog;


class PointSymbolSettings : public SymbolPropertiesWidget
{
Q_OBJECT
public:
	PointSymbolSettings(PointSymbol* symbol, SymbolSettingDialog* dialog);
	~PointSymbolSettings() override;
	
	void reset(Symbol* symbol) override;
	
public slots:
	void tabChanged(int index);
	
private:
	PointSymbol* symbol;
	PointSymbolEditorWidget* symbol_editor;
	QVBoxLayout* layout;
	QWidget* point_tab;
	
};


}  // namespace OpenOrienteering

#endif
