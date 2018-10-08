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
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QFlags>
#include <QGridLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLatin1Char>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QTextDocument>
#include <QTextEdit>
#include <QWidget>

#include "core/map.h"
#include "core/symbols/symbol.h"
#include "gui/symbols/icon_properties_widget.h"
#include "gui/symbols/symbol_setting_dialog.h"
#include "util/translation_util.h"
#include "util/backports.h"

// IWYU pragma: no_forward_declare QGridLayout
// IWYU pragma: no_forward_declare QLabel


namespace OpenOrienteering {

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
	
	auto language_label = new QLabel(QApplication::translate("OpenOrienteering::MapSymbolTranslation", "Text source:"));
	language_combo = new QComboBox();
	edit_button = new QPushButton(tr("Edit"));
	auto name_label = new QLabel(tr("Name:"));
	name_edit = new QLineEdit();
	auto description_label = new QLabel(tr("Description:"));
	description_edit = new QTextEdit();
	helper_symbol_check = new QCheckBox(tr("Helper symbol (not shown in finished map)"));
	
	icon_widget = new IconPropertiesWidget(symbol, dialog);  // before reset()
	
	SymbolPropertiesWidget::reset(symbol);

	for (auto editor : number_editors)
		connect(editor, &QLineEdit::textEdited, this, &SymbolPropertiesWidget::numberChanged);
	connect(language_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SymbolPropertiesWidget::languageChanged);
	connect(edit_button, &QPushButton::clicked, this, &SymbolPropertiesWidget::editClicked);
	connect(name_edit, &QLineEdit::textEdited, this, &SymbolPropertiesWidget::nameChanged);
	connect(description_edit, &QTextEdit::textChanged, this, &SymbolPropertiesWidget::descriptionChanged);
	connect(helper_symbol_check, &QAbstractButton::clicked, this, &SymbolPropertiesWidget::helperSymbolChanged);
	connect(this, &SymbolPropertiesWidget::propertiesModified, dialog, [dialog]() { dialog->setSymbolModified(true); });
	
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
	++col;
	// 8th col
	layout->setColumnStretch(col++, 1);
	
	auto num_col = col;
	
	row++; col = 0;
	layout->addWidget(language_label, row, col++);
	layout->addWidget(language_combo, row, col++, 1, num_col-3);
	layout->addWidget(edit_button, row, num_col-2);
	
	row++; col = 0;
	layout->addWidget(name_label, row, col++);
	layout->addWidget(name_edit, row, col++, 1, num_col-1);
	
	row++; col = 0;
	layout->addWidget(description_label, row, col, 1, num_col);
	
	row++; col = 0;
	layout->addWidget(description_edit, row, col, 1, num_col);
	
	row++; col = 0;
	layout->addWidget(helper_symbol_check, row, col, 1, num_col);
	
	addPropertiesGroup(tr("General"), generalTab);
	
	connect(icon_widget, &IconPropertiesWidget::iconModified, this, &SymbolPropertiesWidget::propertiesModified);
	addPropertiesGroup(tr("Icon"), icon_widget);
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

void SymbolPropertiesWidget::numberChanged(const QString& text)
{
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

void SymbolPropertiesWidget::languageChanged()
{
	updateTextEdits();
}

void SymbolPropertiesWidget::editClicked()
{
	int result;
	auto question = QString{};
	if (language_combo->currentIndex() == 1)
	{
		question = QApplication::translate("OpenOrienteering::MapSymbolTranslation",
		             "Before editing, the stored text will be "
		             "replaced with the current translation. "
		             "Do you want to continue?");
	}
	else
	{
		question = QApplication::translate("OpenOrienteering::MapSymbolTranslation",
		             "After modifying the stored text, "
		             "the translation may no longer be found. "
		             "Do you want to continue?");
	}
	result = QMessageBox::warning(dialog, tr("Warning"), question,
	                              QMessageBox::Yes | QMessageBox::No,
	                              QMessageBox::Yes);
	if (result == QMessageBox::Yes)
	{
		language_combo->setEnabled(false);
		edit_button->setEnabled(false);
		name_edit->setEnabled(true);
		description_edit->setEnabled(true);
		if (language_combo->currentIndex() == 1)
		{
			{
				QSignalBlocker block(language_combo);
				language_combo->setCurrentIndex(0);
			}
			auto name = name_edit->text();
			if (name.isEmpty())
			{
				QSignalBlocker block(name_edit);
				name = symbol->getName();
				name_edit->setText(name);
			}
			if (description_edit->document()->isEmpty())
			{
				QSignalBlocker block(description_edit);
				description_edit->setText(symbol->getDescription());
			}
			symbol->setName(name);
			symbol->setDescription(description_edit->toPlainText());
		}
		emit propertiesModified();
	}
}

void SymbolPropertiesWidget::nameChanged(const QString& text)
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

void SymbolPropertiesWidget::updateTextEdits()
{
	auto name = symbol->getName();
	auto description = symbol->getDescription();
	if (language_combo->currentIndex() == 1)
	{
		name = dialog->getSourceMap()->raw_translation(name);
		description = dialog->getSourceMap()->raw_translation(description);
	}
	{
		QSignalBlocker block(name_edit);
		name_edit->setText(name);
	}
	{
		QSignalBlocker block(description_edit);
		description_edit->setText(description);
	}
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
		if (value >= 0 && editors_enabled)
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
	language_combo->clear();
	auto name_translated = dialog->getSourceMap()->raw_translation(symbol->getName());
	auto description_translated = dialog->getSourceMap()->raw_translation(symbol->getDescription());
	///: The language of the symbol name in the map file is not defined explicitly.
	language_combo->addItem(QApplication::translate("OpenOrienteering::MapSymbolTranslation", "Map (%1)").
	                        arg(QApplication::translate("OpenOrienteering::MapSymbolTranslation", "undefined language")));
	if (name_translated.isEmpty() && description_translated.isEmpty())
	{
		language_combo->setEnabled(false);
		edit_button->setEnabled(false);
		name_edit->setEnabled(true);
		description_edit->setEnabled(true);
	}
	else
	{
		auto language = TranslationUtil::languageFromSettings(QSettings());
		if (!language.isValid())
		{
			language.displayName = QApplication::translate("OpenOrienteering::MapSymbolTranslation", "undefined language");
		}

		language_combo->addItem(QApplication::translate("OpenOrienteering::MapSymbolTranslation", "Translation (%1)").arg(language.displayName));
		language_combo->setEnabled(true);
		language_combo->setCurrentIndex(1);
		edit_button->setEnabled(true);
		name_edit->setEnabled(false);
		description_edit->setEnabled(false);
	}
	updateTextEdits();
	helper_symbol_check->setChecked(symbol->isHelperSymbol());
	
	icon_widget->reset(symbol);
}


}  // namespace OpenOrienteering
