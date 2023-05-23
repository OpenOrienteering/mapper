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

#ifndef JSON_CONFIG_WIDGET_H
#define JSON_CONFIG_WIDGET_H

#include <QDialog>
#include <QObject>

#include "util/json_config.h"

class QDialogButtonBox;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QWidget;

namespace OpenOrienteering {

/**
 *
 */
class JSONConfigWidget : public QDialog
{
Q_OBJECT
public:
	JSONConfigWidget(QWidget* parent);
	
private slots:
	void openJSONFileDialog();
	void useDefault();
	void check();
	void save();
	void saveAs();
	void accept();
	void textChanged();
	
private:
	bool checkJSON(bool showSuccess);
	void showCurrentConfig();
	void saveConfig(const QString& filename);
	void setEnabled(const bool enabled) const;
	
	JSONConfiguration& json_config_instance;	// the single instance of the JSONConfiguration object
	JSONConfigurationObject temp_config;		// temporary configuration set during user configuration
	JSONConfigurationObject saved_config;		// configuration set to track unsaved changes
	QPlainTextEdit* jsonEditor;
	QLineEdit* json_file_edit;
	QPushButton* saveButton;
	QPushButton* saveAsButton;
	QDialogButtonBox* button_box;
	QString temp_filename;						// temporary filename of the current configuration
};

}  // namespace Openorienteering

#endif // JSON_CONFIG_WIDGET_H
