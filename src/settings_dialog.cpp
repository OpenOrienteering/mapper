/*
 *    Copyright 2012 Jan Dalheimer
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

#include "settings_dialog.h"

#include <QtGui>

#include "map_editor.h"
#include "map_widget.h"
#include "main_window.h"
#include "settings.h"

void SettingsPage::apply()
{
	QSettings settings;
	for (int i = 0; i < changes.size(); i++)
		settings.setValue(changes.keys().at(i), changes.values().at(i));
}

// ### EditorPage ###

EditorPage::EditorPage(QWidget* parent) : SettingsPage(parent)
{
	QGridLayout* layout = new QGridLayout();
	this->setLayout(layout);
	
	int row = 0;
	
	QCheckBox* antialiasing = new QCheckBox(tr("Enable Antialiasing"), this);
	antialiasing->setToolTip(tr("Antialiasing makes the map look much better, but also slows down the map display"));
	layout->addWidget(antialiasing, row++, 0, 1, 2);
	
	QLabel* tolerance_label = new QLabel(tr("Click tolerance:"));
	QSpinBox* tolerance = new QSpinBox(this);
	tolerance->setMinimum(1);
	tolerance->setMaximum(50);
	layout->addWidget(tolerance_label, row, 0);
	layout->addWidget(tolerance, row++, 1);
	
	antialiasing->setChecked(Settings::getInstance().getSetting(Settings::MapDisplay_Antialiasing).toBool());
	tolerance->setValue(Settings::getInstance().getSetting(Settings::MapEditor_ClickTolerance).toInt());
	
	layout->setRowStretch(row, 1);

	connect(antialiasing, SIGNAL(toggled(bool)), this, SLOT(antialiasingClicked(bool)));
	connect(tolerance, SIGNAL(valueChanged(int)), this, SLOT(toleranceChanged(int)));
}

void EditorPage::antialiasingClicked(bool checked)
{
	changes.insert(Settings::getInstance().getSettingPath(Settings::MapDisplay_Antialiasing), QVariant(checked));
}

void EditorPage::toleranceChanged(int value)
{
	changes.insert(Settings::getInstance().getSettingPath(Settings::MapEditor_ClickTolerance), QVariant(value));
}

// ### SettingsDialog ###

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent)
{
	setWindowTitle(tr("Settings"));
	QVBoxLayout* layout = new QVBoxLayout();
	this->setLayout(layout);
	//this->resize(640, 480);
	
	tabWidget = new QTabWidget(this);
	layout->addWidget(tabWidget);
	
	button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel,
								Qt::Horizontal, this);
	layout->addWidget(button_box);
	connect(button_box, SIGNAL(clicked(QAbstractButton*)), this, SLOT(buttonPressed(QAbstractButton*)));
	
	// Add all pages
	addPage(new EditorPage(this));
}

void SettingsDialog::addPage(SettingsPage* page)
{
	pages.append(page);
	tabWidget->addTab(pages.last(), pages.last()->title());
}

void SettingsDialog::buttonPressed(QAbstractButton* button)
{
	QDialogButtonBox::StandardButton id = button_box->standardButton(button);
	if (id == QDialogButtonBox::Ok)
	{
		foreach(SettingsPage* page, pages)
			page->ok();
		Settings::getInstance().applySettings();
		this->accept();
	}
	else if (id == QDialogButtonBox::Apply)
	{
		foreach(SettingsPage* page, pages)
			page->apply();
		Settings::getInstance().applySettings();
	}
	else if (id == QDialogButtonBox::Cancel)
	{
		foreach(SettingsPage* page, pages)
			page->cancel();
		this->reject();
	}
}

#include "settings_dialog.moc"
