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


#include "map_dialog_new.h"

#include <cassert>

#include <QtGui>
#include <QDir>
#include <QSettings>
#include <QCoreApplication>

NewMapDialog::NewMapDialog(QWidget* parent) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
{
	setWindowTitle(tr("Create new map"));
	
	QLabel* desc_label = new QLabel(tr("Choose the scale and symbol set for the new map.\nThe available symbol sets depend on the selected scale."));
	
	QLabel* scale_label = new QLabel(tr("Scale:  1 : "));
	scale_combo = new QComboBox();
	scale_combo->setEditable(true);
	scale_combo->setValidator(new QIntValidator(1, 9999999, scale_combo));
	
	QLabel* symbol_set_label = new QLabel(tr("Symbol sets:"));
	symbol_set_list = new QListWidget();
	
	QPushButton* cancel_button = new QPushButton(tr("Cancel"));
	create_button = new QPushButton(QIcon(":/images/arrow-right.png"), tr("Create"));
	create_button->setDefault(true);
	
	QHBoxLayout* scale_layout = new QHBoxLayout();
	scale_layout->addWidget(scale_label);
	scale_layout->addWidget(scale_combo);
	scale_layout->addStretch(1);
	
	QHBoxLayout* buttons_layout = new QHBoxLayout();
	buttons_layout->addWidget(cancel_button);
	buttons_layout->addStretch(1);
	buttons_layout->addWidget(create_button);
	
	QVBoxLayout* layout = new QVBoxLayout();
	layout->addWidget(desc_label);
	layout->addSpacing(16);
	layout->addLayout(scale_layout);
	layout->addWidget(symbol_set_label);
	layout->addWidget(symbol_set_list);
	layout->addSpacing(16);
	layout->addLayout(buttons_layout);
	setLayout(layout);
	
	loadSymbolSetMap();
	for (SymbolSetMap::iterator it = symbol_set_map.begin(); it != symbol_set_map.end(); ++it)
		scale_combo->addItem(QString::number(it->first));
	
	QSettings settings;
	settings.beginGroup("NewMapDialog");
	const QString default_scale = settings.value("DefaultScale", "10000").toString();
	settings.endGroup();
	
	scale_combo->setEditText(default_scale);
	connect(scale_combo, SIGNAL(editTextChanged(QString)), this, SLOT(scaleChanged(QString)));
	connect(symbol_set_list, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(symbolSetDoubleClicked(QListWidgetItem*)));
	connect(cancel_button, SIGNAL(clicked(bool)), this, SLOT(reject()));
	connect(create_button, SIGNAL(clicked(bool)), this, SLOT(createClicked()));
	scaleChanged(default_scale);
}

int NewMapDialog::getSelectedScale() const
{
	return scale_combo->currentText().toInt();
}

QString NewMapDialog::getSelectedSymbolSetPath() const
{
	QListWidgetItem* item = symbol_set_list->currentItem();
	if (! item || ! item->data(Qt::UserRole).isValid())
	{
		// FIXME: add proper error handling for release builds, or remove.
		assert(false);
		return "";
	}

	return item->data(Qt::UserRole).toString();
}

void NewMapDialog::accept()
{
	QSettings settings;
	settings.beginGroup("NewMapDialog");
	settings.setValue("DefaultScale", getSelectedScale());
	settings.endGroup();
	
	QDialog::accept();
}

void NewMapDialog::scaleChanged(QString new_text)
{
	int scale = new_text.toInt();
	if (scale == 0)
	{
		create_button->setEnabled(false);
		symbol_set_list->setEnabled(false);
		return;
	}
	
	create_button->setEnabled(true);
	symbol_set_list->setEnabled(true);
	symbol_set_list->clear();
		
	QListWidgetItem* item = new QListWidgetItem(tr("Empty symbol set"));
	item->setData(Qt::UserRole, "");
	item->setIcon(QIcon(":/images/new.png"));
	symbol_set_list->addItem(item);
	
	QIcon control(":/images/control.png");
	SymbolSetMap::iterator it = symbol_set_map.find(scale);
	if (it != symbol_set_map.end())
		foreach (const QFileInfo& symbol_set, it->second)
		{
			item = new QListWidgetItem(symbol_set.completeBaseName());
			item->setData(Qt::UserRole, symbol_set.canonicalFilePath());
			item->setIcon(control);
			symbol_set_list->addItem(item);
		}
	
	load_from_file = new QListWidgetItem(tr("Load symbol set from a file..."));
	load_from_file->setData(Qt::UserRole, qVariantFromValue<void*>(NULL));
	load_from_file->setIcon(QIcon(":/images/open.png"));
	symbol_set_list->addItem(load_from_file);
	
	symbol_set_list->setCurrentRow(0);
}

void NewMapDialog::symbolSetDoubleClicked(QListWidgetItem* item)
{
	symbol_set_list->setCurrentItem(item);
	if (item == load_from_file)
		showFileDialog();
	else
		accept();
}

void NewMapDialog::createClicked()
{
	QListWidgetItem* item = symbol_set_list->currentItem();
	if (item == load_from_file)
		showFileDialog();
	else
		accept();
}

void NewMapDialog::showFileDialog()
{
	// Get the saved directory to start in, defaulting to the user's home directory.
	QString open_directory = QSettings().value("openFileDirectory", QDir::homePath()).toString();
	
	QString filters = 
	  tr("OpenOrienteering symbol sets")  % " (*.omap);;" %
	  tr("All files") % " (*.*)";
	  
	QString path = QFileDialog::getOpenFileName(this, tr("Load symbol set from a file..."), open_directory, filters);
	path = QFileInfo(path).canonicalFilePath();
	if (path.isEmpty())
		return;
	
	load_from_file->setData(Qt::UserRole, path);
	accept();
}

void NewMapDialog::loadSymbolSetMap()
{
	QString app_dir = QCoreApplication::applicationDirPath();
	
	// FIXME: How to translate directory name "my symbol sets"?
	
	// #1: symbol sets from HOME/my symbol sets
	QDir symbol_set_dir(QDir::homePath() % "/my symbol sets");
	if (symbol_set_dir.exists())
		loadSymbolSetDir(symbol_set_dir);
	
	// #2: symbol sets from APPDIR/my symbol sets
	symbol_set_dir.cd(app_dir % "/my symbol sets");
	if (symbol_set_dir.exists())
		loadSymbolSetDir(symbol_set_dir);
	
#ifndef WIN32
	// #3: symbol sets from /usr/share et al. if executable is in /usr/bin
	symbol_set_dir.cd(app_dir % "/../share/openorienteering-mapper/symbol sets");
	if (symbol_set_dir.exists())
		loadSymbolSetDir(symbol_set_dir);
#endif
}

void NewMapDialog::loadSymbolSetDir(const QDir& symbol_set_dir)
{
	QStringList subdirs = symbol_set_dir.entryList(QDir::Dirs | QDir::Hidden | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDir::NoSort);
	foreach (const QString dir_name, subdirs)
	{
		int scale = dir_name.toInt();
		if (scale == 0)
		{
			qDebug() << dir_name % ": not a valid map scale denominator.";
			continue;
		}
		
		QDir subdir(symbol_set_dir);
		if (! subdir.cd(dir_name))
		{
			qDebug() << dir_name % ": cannot access this directory.";
			continue;
		}
		
		QStringList symbol_set_filters;
		symbol_set_filters << "*.omap";
		subdir.setNameFilters(symbol_set_filters);
		QFileInfoList symbol_set_files = subdir.entryInfoList(QDir::Files | QDir::Hidden | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDir::Name);
		symbol_set_map.insert(std::make_pair(scale, symbol_set_files));
	}
}

#include "map_dialog_new.moc"
