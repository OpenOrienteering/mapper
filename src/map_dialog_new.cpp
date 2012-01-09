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

#include "assert.h"

#include <QtGui>

NewMapDialog::NewMapDialog(QWidget* parent) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
{
	setWindowTitle(tr("Create new map"));
	
	QLabel* desc_label = new QLabel(tr("Choose the scale and symbol set for your new map here.\nThe available symbol sets depend on the selected scale."));
	
	QLabel* scale_label = new QLabel(tr("Scale:  1 : "));
	scale_combo = new QComboBox();
	scale_combo->setEditable(true);
	scale_combo->setValidator(new QIntValidator(1, 9999999, scale_combo));
	
	QLabel* symbol_set_label = new QLabel(tr("Symbol sets:"));
	symbol_set_list = new QListWidget();
	
	QPushButton* cancel_button = new QPushButton(tr("Cancel"));
	create_button = new QPushButton(QIcon("images/arrow-right.png"), tr("Create"));
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
	for (std::map<int, QStringList>::iterator it = symbol_set_map.begin(); it != symbol_set_map.end(); ++it)
		scale_combo->addItem(QString::number(it->first));
	
	connect(scale_combo, SIGNAL(editTextChanged(QString)), this, SLOT(scaleChanged(QString)));
	connect(cancel_button, SIGNAL(clicked(bool)), this, SLOT(reject()));
	connect(create_button, SIGNAL(clicked(bool)), this, SLOT(createClicked()));
	
	scale_combo->setEditText("10000");
}

int NewMapDialog::getSelectedScale()
{
	return scale_combo->currentText().toInt();
}
QString NewMapDialog::getSelectedSymbolSetPath()
{
	const QString dir_name = "my symbol sets";
	QListWidgetItem* item = symbol_set_list->currentItem();
	if (!item)
	{
		assert(false);
		return "";
	}
	
	if (item->data(Qt::UserRole).isValid())
		return "";
	else
		return dir_name + "/" + QString::number(getSelectedScale()) + "/" + item->text() + ".omap";
}

void NewMapDialog::scaleChanged(QString new_text)
{
	if (new_text.isEmpty())
	{
		create_button->setEnabled(false);
		symbol_set_list->setEnabled(false);
		return;
	}
	
	bool ok = false;
	int scale = new_text.toInt(&ok);
	create_button->setEnabled(ok);
	symbol_set_list->setEnabled(ok);
	
	if (ok)
	{
		symbol_set_list->clear();
		
		QListWidgetItem* item = new QListWidgetItem(tr("(empty symbol set)"));
		item->setData(Qt::UserRole, qVariantFromValue<void*>(NULL));
		symbol_set_list->addItem(item);
		
		std::map<int, QStringList>::iterator it = symbol_set_map.find(scale);
		if (it != symbol_set_map.end())
			symbol_set_list->addItems(it->second);
		
		symbol_set_list->setCurrentRow(0);
	}
}
void NewMapDialog::createClicked()
{
	accept();
}

void NewMapDialog::loadSymbolSetMap()
{
	const QString dir_name = "my symbol sets";
	QDir dir(dir_name);
	if (!dir.exists())
	{
		QDir().mkdir(dir_name);
		dir = QDir(dir_name);
	}
	
	QStringList my_scales = dir.entryList(QDir::Dirs | QDir::Hidden | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDir::NoSort);
	
	for (int i = 0; i < my_scales.size(); ++i)
	{
		bool ok = false;
		int scale = my_scales.at(i).toInt(&ok);
		if (!ok)
			continue;
		
		QDir subdir = QDir(dir_name + "/" + my_scales.at(i));
		
		QStringList symbol_set_filters;
		symbol_set_filters << "*.omap";
		QStringList symbol_set_files = subdir.entryList(QDir::Files | QDir::Hidden | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDir::Name);
		for (int k = 0; k < symbol_set_files.size(); ++k)
			symbol_set_files[k] = symbol_set_files[k].left(symbol_set_files[k].size() - 5);
		symbol_set_map.insert(std::make_pair(scale, symbol_set_files));
	}
}

#include "map_dialog_new.moc"
