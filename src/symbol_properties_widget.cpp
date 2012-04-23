/*
 *    Copyright 2012 Thomas Schöps
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


#include "symbol_properties_widget.h"

#include <QtGui>

#include "symbol_setting_dialog.h"

SymbolPropertiesWidget::SymbolPropertiesWidget(Symbol* symbol, SymbolSettingDialog* dialog)
: QTabWidget(), symbol(symbol), dialog(dialog)
{
	QWidget* generalTab = new QWidget();
	
	QGridLayout* layout = new QGridLayout();
	generalTab->setLayout(layout);
	
	QLabel* number_label = new QLabel(tr("Number:"));
	number_edit = new QLineEdit*[Symbol::number_components];
	bool number_edit_enabled = true;
	for (int i = 0; i < Symbol::number_components; ++i)
	{
		int number_edit_value = symbol->getNumberComponent(i);
		number_edit[i] = new QLineEdit((number_edit_enabled && number_edit_value >= 0) ? QString::number(number_edit_value) : "");
		number_edit[i]->setEnabled(number_edit_enabled);
		number_edit[i]->setMaximumWidth(60);
		number_edit[i]->setValidator(new QIntValidator(0, 99999, number_edit[i]));
		number_edit_enabled = number_edit_enabled && number_edit_value >= 0;
	}
	QLabel* name_label = new QLabel(tr("Name:"));
	name_edit = new QLineEdit(symbol->getName());
	QLabel* description_label = new QLabel(tr("Description:"));
	description_edit = new QTextEdit(symbol->getDescription());
	helper_symbol_check = new QCheckBox(tr("Helper symbol (not shown in finished map)"));
	helper_symbol_check->setChecked(symbol->isHelperSymbol());
	
	for (int i = 0; i < Symbol::number_components; ++i)
		connect(number_edit[i], SIGNAL(textEdited(QString)), this, SLOT(numberChanged(QString)));
	connect(name_edit, SIGNAL(textEdited(QString)), this, SLOT(nameChanged(QString)));
	connect(description_edit, SIGNAL(textChanged()), this, SLOT(descriptionChanged()));
	connect(helper_symbol_check, SIGNAL(clicked(bool)), this, SLOT(helperSymbolChanged(bool)));
	
	int row = 0, col = 0;
	// 1st col
	layout->addWidget(number_label, row, col++);
	// 5 cols
	layout->addWidget(number_edit[0], row, col++);
	layout->addWidget(new QLabel("."), row, col++);
	layout->addWidget(number_edit[1], row, col++);
	layout->addWidget(new QLabel("."), row, col++);
	layout->addWidget(number_edit[2], row, col++);
	// 7th col
	layout->setColumnStretch(col, 1);
	
	row++; col = 0;
	layout->addWidget(name_label, row, col++);
	layout->addWidget(name_edit, row, col++, 1, 6);
	
	row++; col = 0;
	layout->addWidget(description_label, row, col, 1, 7);
	
	row++; col = 0;
	layout->addWidget(description_edit, row, col, 1, 7);
	
	row++; col = 0;
	layout->addWidget(helper_symbol_check, row, col, 1, 7);
	
	addPropertiesGroup(tr("General"), generalTab);
}

SymbolPropertiesWidget::~SymbolPropertiesWidget()
{
}

void SymbolPropertiesWidget::addPropertiesGroup(const QString& name, QWidget* widget)
{
	 addTab(widget, name);
}

void SymbolPropertiesWidget::insertPropertiesGroup(int index, const QString& name, QWidget* widget)
{
	insertTab(index, widget, name);
}

void SymbolPropertiesWidget::removePropertiesGroup(int index)
{
	QWidget* tab_contents = widget(index);
	removeTab(index);
	delete tab_contents;
}

void SymbolPropertiesWidget::removePropertiesGroup(const QString& name)
{
	int index = indexOfPropertiesGroup(name);
	removePropertiesGroup(index);
}

void SymbolPropertiesWidget::renamePropertiesGroup(const QString& old_name, const QString& new_name)
{
	int index = indexOfPropertiesGroup(old_name);
	setTabText(index, new_name);
}

int SymbolPropertiesWidget::indexOfPropertiesGroup(const QString& name) const
{
	for (int i = 0; i < count(); i++)
	{
		if (tabText(i) == name)
			return i;
	}
	return -1;
}

void SymbolPropertiesWidget::numberChanged(QString text)
{
	bool is_valid = true;
	int i = 0;
	while (i < Symbol::number_components)
	{
		QString number_text = number_edit[i]->text();
		if (is_valid && !number_text.isEmpty())
			symbol->setNumberComponent(i, number_text.toInt());
		else
		{
			symbol->setNumberComponent(i, -1);
			is_valid = false;
		}
		
		++i;
		if (i < Symbol::number_components)
			number_edit[i]->setEnabled(is_valid);
	}
	dialog->generalModified();
}

void SymbolPropertiesWidget::nameChanged(QString text)
{
	symbol->setName(text);
	dialog->generalModified();
}

void SymbolPropertiesWidget::descriptionChanged()
{
	symbol->setDescription(description_edit->toPlainText());
	dialog->generalModified();
}

void SymbolPropertiesWidget::helperSymbolChanged(bool checked)
{
	symbol->setIsHelperSymbol(checked);
	dialog->generalModified();
}

#include "symbol_properties_widget.moc"
