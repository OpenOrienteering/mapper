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

#include "gui/symbols/symbol_properties_widget.h"

#include "core/symbols/point_symbol.h"

QT_BEGIN_NAMESPACE
class QVBoxLayout;
class QWidget;
QT_END_NAMESPACE

class PointSymbolEditorWidget;
class SymbolSettingDialog;


class PointSymbolSettings : public SymbolPropertiesWidget
{
Q_OBJECT
public:
	PointSymbolSettings(PointSymbol* symbol, SymbolSettingDialog* dialog);
	
	virtual void reset(Symbol* symbol);
	
public slots:
	void tabChanged(int index);
	
private:
	PointSymbol* symbol;
	PointSymbolEditorWidget* symbol_editor;
	QVBoxLayout* layout;
	QWidget* point_tab;
	
};

#endif
