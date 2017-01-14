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


#include "point_symbol_settings.h"

#include <QVBoxLayout>

#include "gui/symbols/symbol_setting_dialog.h"
#include "gui/symbols/point_symbol_editor_widget.h"


// ### PointSymbol ###

SymbolPropertiesWidget* PointSymbol::createPropertiesWidget(SymbolSettingDialog* dialog)
{
	return new PointSymbolSettings(this, dialog);
}


// ### PointSymbolSettings ###

PointSymbolSettings::PointSymbolSettings(PointSymbol* symbol, SymbolSettingDialog* dialog)
: SymbolPropertiesWidget(symbol, dialog), 
  symbol(symbol)
{
	symbol_editor = new PointSymbolEditorWidget(dialog->getPreviewController(), symbol, 0, true);
	connect(symbol_editor, SIGNAL(symbolEdited()), this, SIGNAL(propertiesModified()) );
	
	layout = new QVBoxLayout();
	layout->addWidget(symbol_editor);
	
	point_tab = new QWidget();
	point_tab->setLayout(layout);
	addPropertiesGroup(tr("Point symbol"), point_tab);
	
	connect(this, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
}

void PointSymbolSettings::reset(Symbol* symbol)
{
	Q_ASSERT(symbol->getType() == Symbol::Point);
	
	SymbolPropertiesWidget::reset(symbol);
	this->symbol = reinterpret_cast<PointSymbol*>(symbol);
	
	layout->removeWidget(symbol_editor);
	delete(symbol_editor);
	
	symbol_editor = new PointSymbolEditorWidget(dialog->getPreviewController(), this->symbol, 0, true);
	connect(symbol_editor, SIGNAL(symbolEdited()), this, SIGNAL(propertiesModified()) );
	layout->addWidget(symbol_editor);
}

void PointSymbolSettings::tabChanged(int index)
{
	Q_UNUSED(index);
	symbol_editor->setEditorActive( currentWidget()==point_tab );
}
