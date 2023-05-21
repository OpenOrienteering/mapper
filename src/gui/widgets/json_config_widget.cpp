/*
 *    Copyright 2023 Matthias Kühlewein
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

#include "json_config_widget.h"

#include <QAbstractButton>
#include <QToolButton>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QMessageBox>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

#include "gui/util_gui.h"
#include "gui/file_dialog.h"
#include "util/json_config.h"

namespace OpenOrienteering {

JSONConfigWidget::JSONConfigWidget(QWidget* parent)
: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
, json_config_instance(JSONConfiguration::getInstance())
{
	setWindowTitle(tr("JSON configuration"));
	
	auto file_select_widget = new QWidget();
	auto file_select_layout = new QHBoxLayout(file_select_widget);
	
	QFormLayout* file_layout = new QFormLayout();
	json_file_edit = new QLineEdit();
	file_layout->addRow(tr("JSON file:"), json_file_edit);
	json_file_edit->setEnabled(false);
	
	file_select_layout->addLayout(file_layout);
	
	auto file_select_button = new QToolButton();
	/*if (Settings::mobileModeEnforced())
	{
		json_file_button->setVisible(false);
	}
	else
	{*/
		file_select_button->setIcon(QIcon(QLatin1String(":/images/open.png")));
	//}
	file_select_layout->addWidget(file_select_button);
	
	jsonEditor = new QPlainTextEdit();
	
	auto button_widget = new QWidget();
	auto button_layout = new QHBoxLayout(button_widget);
	
	auto defaultButton = new QPushButton(tr("Use default"));
	button_layout->addWidget(defaultButton);
	auto checkButton = new QPushButton(tr("Check"));
	button_layout->addWidget(checkButton);
	saveButton = new QPushButton(tr("Save"));
	button_layout->addWidget(saveButton);
	saveAsButton = new QPushButton(tr("Save as"));
	button_layout->addWidget(saveAsButton);
	
	button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal);
	
	auto main_layout = new QVBoxLayout();
	main_layout->addWidget(file_select_widget);
	main_layout->addItem(Util::SpacerItem::create(this));
	main_layout->addWidget(jsonEditor);
	main_layout->addWidget(button_widget);
	main_layout->addItem(Util::SpacerItem::create(this));
	main_layout->addStretch();
	main_layout->addWidget(button_box);
	
	setLayout(main_layout);
	
	connect(file_select_button, &QAbstractButton::clicked, this, &JSONConfigWidget::openJSONFileDialog);
	connect(defaultButton, &QAbstractButton::clicked, this, &JSONConfigWidget::useDefault);
	connect(checkButton, &QAbstractButton::clicked, this, &JSONConfigWidget::check);
	connect(saveButton, &QAbstractButton::clicked, this, &JSONConfigWidget::save);
	connect(saveAsButton, &QAbstractButton::clicked, this, &JSONConfigWidget::saveAs);
	connect(button_box, &QDialogButtonBox::accepted, this, &JSONConfigWidget::accept);
	connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(jsonEditor, &QPlainTextEdit::textChanged, this, &JSONConfigWidget::textChanged);
	
	if (json_config_instance.isLoadedConfigValid())
	{
		temp_config = *json_config_instance.getLoadedConfig();
		temp_filename = json_config_instance.getJSONFilename();
		saved_config = temp_config;
		showCurrentConfig();
	}
}

// slot
void JSONConfigWidget::openJSONFileDialog()
{
	const auto filename = FileDialog::getOpenFileName(this, tr("Select JSON configuration file"), {}, tr("JSON files (*.json)"));
	if (!filename.isNull())
	{
		if (!temp_config.readJSONFile(filename))
		{
			QMessageBox::warning(this, tr("Error"), tr("Reading JSON file %1 failed").arg(filename), QMessageBox::Ok);
			return;
		}
		temp_filename = filename;
		saved_config = temp_config;
		showCurrentConfig();
	}
}

// slot
void JSONConfigWidget::useDefault()
{
	temp_config = *json_config_instance.getDefault();
	temp_filename.clear();
	showCurrentConfig();
}

// slot
void JSONConfigWidget::check()
{
	checkJSON(true);
}

// slot
void JSONConfigWidget::save()
{
	if (!checkJSON(false))	// check again
		return;
	if (temp_filename.isEmpty())	// don't just rely on disabling the 'Save' button
		saveAs();
	saveConfig(temp_filename);
}

// slot
void JSONConfigWidget::saveAs()
{
	if (!checkJSON(false))	// check again
		return;
	const auto filename = FileDialog::getSaveFileName(this, tr("Save As"), {}, tr("JSON files (*.json)"));
	saveConfig(filename);
}

void JSONConfigWidget::saveConfig(const QString& filename)
{
	if (filename.isEmpty())
		return;
	if (temp_config.JSONfromText(jsonEditor->toPlainText()))
	{
		if (!temp_config.writeJSONFile(filename))
		{
			QMessageBox::warning(this, tr("Error"), tr("Saving JSON file failed"), QMessageBox::Ok);
			return;
		}
		temp_filename = filename;
		saved_config = temp_config;
		showCurrentConfig();
	}
}

// slot
void JSONConfigWidget::accept()
{
	if (saved_config != temp_config)
	{
		auto ret = QMessageBox::question(this, tr("Save changes"), tr("The current configuration has unsaved changes.\nLeave without saving?"), QMessageBox::Yes | QMessageBox::No);
		if (ret == QMessageBox::No)
			return;
	}
	if (!temp_filename.isEmpty())
		json_config_instance.setJSONFilename(temp_filename);
	//if (temp_config)
	json_config_instance.setJSONConfig(temp_config);
	
	QDialog::accept();
}

// slot
void JSONConfigWidget::textChanged()
{
	setEnabled(temp_config.JSONfromText(jsonEditor->toPlainText()) && temp_config.IsValid());
}

void JSONConfigWidget::setEnabled(const bool enabled) const
{
	saveButton->setEnabled(enabled && !temp_filename.isEmpty());
	saveAsButton->setEnabled(enabled);
	button_box->button(QDialogButtonBox::Ok)->setEnabled(enabled);
}

bool JSONConfigWidget::checkJSON(bool showSuccess)
{
	auto ok = true;
	QJsonParseError json_parse_error;
	if (!temp_config.JSONfromText(jsonEditor->toPlainText(), &json_parse_error))
	{
		auto error_string = json_parse_error.errorString();
		QMessageBox::warning(this, tr("Error"), tr("JSON parsing failed:\n%1").arg(error_string), QMessageBox::Ok);
		return false;
	}
	auto &temp_json = temp_config.getJSONObject();
	const auto& def_json = json_config_instance.getDefault()->getJSONObject();
	if (temp_json != def_json)
	{
		for (auto it = temp_json.begin(); it != temp_json.end(); ++it)
		{
			if (!def_json.contains(it.key()))
			{
				ok = false;
				auto ret = QMessageBox::question(this, tr("Remove parameter"), tr("Configuration parameter %1 is not used by this version of Mapper.\nRemove parameter from current configuration?").arg(it.key()), QMessageBox::Yes | QMessageBox::No);
				if (ret == QMessageBox::Yes)
				{
					it = temp_json.erase(it);
					if (it == temp_json.end())
						break;
				}
			}
			else if (temp_json.value(it.key()).type() != def_json.value(it.key()).type())
			{
				ok = false;
				auto ret = QMessageBox::warning(this, tr("Error"), tr("Configuration parameter %1 has wrong type.\nReplace it by default setting?").arg(it.key()), QMessageBox::Yes | QMessageBox::No);
				if (ret == QMessageBox::Yes)
				{
					it.value() = def_json.value(it.key());
				}
			}
		}
		for (auto it = def_json.constBegin(); it != def_json.constEnd(); ++it)
		{
			if (!temp_json.contains(it.key()))
			{
				ok = false;
				auto ret = QMessageBox::question(this, tr("Add parameter"), tr("Configuration parameter %1 is used by this version of Mapper but is not present in the current configuration.\nAdd parameter to current configuration?").arg(it.key()), QMessageBox::Yes | QMessageBox::No);
				if (ret == QMessageBox::Yes)
				{
					temp_json.insert(it.key(), it.value());
				}
			}
		}
	}
	if (ok && showSuccess)
		QMessageBox::information(this, tr("Success"), tr("JSON parsing successful"), QMessageBox::Ok);
	
	showCurrentConfig();
	return true;
}

void JSONConfigWidget::showCurrentConfig()
{
	auto json_text = temp_config.getText();
	jsonEditor->setPlainText(json_text);
	json_file_edit->setText(temp_filename);
	setEnabled(true);
}

}  // namespace OpenOrienteering
