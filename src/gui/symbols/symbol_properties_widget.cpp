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


#include "symbol_properties_widget.h"

#include <QAbstractButton>
#include <QCheckBox>
#include <QGridLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLatin1Char>
#include <QLineEdit>
#include <QSignalBlocker>
#include <QTextEdit>
#include <QWidget>

#include "core/symbols/symbol.h"
#include "gui/symbols/symbol_setting_dialog.h"

// IWYU pragma: no_forward_declare QGridLayout
// IWYU pragma: no_forward_declare QLabel


// ### Symbol ###

SymbolPropertiesWidget* Symbol::createPropertiesWidget(SymbolSettingDialog* dialog)
{
	return new SymbolPropertiesWidget(this, dialog);
}



// ### SymbolPropertiesWidget ###

SymbolPropertiesWidget::SymbolPropertiesWidget(Symbol* symbol, SymbolSettingDialog* dialog)
: QTabWidget()
, dialog(dialog)
{
	auto generalTab = new QWidget();
	
	auto layout = new QGridLayout();
	generalTab->setLayout(layout);
	
	auto number_label = new QLabel(tr("Number:"));
	number_editors.resize(Symbol::number_components, nullptr);
	for (auto& editor : number_editors)
	{
		editor = new QLineEdit{};
		editor->setMaximumWidth(60);
		editor->setValidator(new QIntValidator(0, 99999, editor));
	}
	auto name_label = new QLabel(tr("Name:"));
	name_edit = new QLineEdit();
	auto description_label = new QLabel(tr("Description:"));
	description_edit = new QTextEdit();
	helper_symbol_check = new QCheckBox(tr("Helper symbol (not shown in finished map)"));
	SymbolPropertiesWidget::reset(symbol);

	for (auto editor : number_editors)
		connect(editor, &QLineEdit::textEdited, this, &SymbolPropertiesWidget::numberChanged);
	connect(name_edit, &QLineEdit::textEdited, this, &SymbolPropertiesWidget::nameChanged);
	connect(description_edit, &QTextEdit::textChanged, this, &SymbolPropertiesWidget::descriptionChanged);
	connect(helper_symbol_check, &QAbstractButton::clicked, this, &SymbolPropertiesWidget::helperSymbolChanged);
	connect(this, &SymbolPropertiesWidget::propertiesModified, [dialog]() { dialog->setSymbolModified(true); });
	
	int row = 0, col = 0;
	// 1st col
	layout->addWidget(number_label, row, col++);
	// 5 cols
	for (auto editor : number_editors)
	{
		layout->addWidget(editor, row, col++);
		if (editor != number_editors.back())
			layout->addWidget(new QLabel(QString(QLatin1Char{'.'})), row, col++);
	}
	// 7th col
	layout->setColumnStretch(col, 1);
	
	auto last_col = col;
	
	row++; col = 0;
	layout->addWidget(name_label, row, col++);
	layout->addWidget(name_edit, row, col++, 1, last_col);
	
	row++; col = 0;
	layout->addWidget(description_label, row, col, 1, last_col+1);
	
	row++; col = 0;
	layout->addWidget(description_edit, row, col, 1, last_col+1);
	
	row++; col = 0;
	layout->addWidget(helper_symbol_check, row, col, 1, last_col+1);
	
	addPropertiesGroup(tr("General"), generalTab);
}


SymbolPropertiesWidget::~SymbolPropertiesWidget() = default;



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
	auto tab_contents = widget(index);
	removeTab(index);
	delete tab_contents;
}

void SymbolPropertiesWidget::removePropertiesGroup(const QString& name)
{
	auto index = indexOfPropertiesGroup(name);
	removePropertiesGroup(index);
}

void SymbolPropertiesWidget::renamePropertiesGroup(int index, const QString& new_name)
{
	setTabText(index, new_name);
}

void SymbolPropertiesWidget::renamePropertiesGroup(const QString& old_name, const QString& new_name)
{
	auto index = indexOfPropertiesGroup(old_name);
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
	Q_UNUSED(text);
	bool editors_enabled = true;
	int i = 0;
	for (auto editor : number_editors)
	{
		editor->setEnabled(editors_enabled);
		if (editor == sender())
			symbol->setNumberComponent(i, text.isEmpty() ? -1 : text.toInt());
		if (editors_enabled && editor->text().isEmpty())
			editors_enabled = false;
		++i;
	}
	emit propertiesModified();
}

void SymbolPropertiesWidget::nameChanged(QString text)
{
	symbol->setName(text);
	emit propertiesModified();
}

void SymbolPropertiesWidget::descriptionChanged()
{
	symbol->setDescription(description_edit->toPlainText());
	emit propertiesModified();
}

void SymbolPropertiesWidget::helperSymbolChanged(bool checked)
{
	symbol->setIsHelperSymbol(checked);
	emit propertiesModified();
}

void SymbolPropertiesWidget::reset(Symbol* symbol)
{
	QSignalBlocker block(this);
	this->symbol = symbol;
	bool editors_enabled = true;
	int i = 0;
	for (auto editor : number_editors)
	{
		editor->setEnabled(editors_enabled);
		
		int value = symbol->getNumberComponent(i);
		if (value > 0 && editors_enabled)
		{
			editor->setText(QString::number(value));
		}
		else
		{
			editor->setText({});
			editors_enabled = false;
		}
		++i;
	}
	name_edit->setText(symbol->getName());
	description_edit->setText(symbol->getDescription());
	helper_symbol_check->setChecked(symbol->isHelperSymbol());
}
