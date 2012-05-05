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

#include <QHeaderView>
#include <QEvent>
#include <QLineEdit>
#include <QDateTime>
#include <QDate>
#include <QTime>
#include <QHBoxLayout>
#include <QDebug>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QRadioButton>
#include <QCheckBox>
#include <QApplication>
#include <QDesktopWidget>
#include <QSpinBox>
#include <QGroupBox>
#include "map_editor.h"
#include "map_widget.h"
#include "main_window.h"

void SettingsPage::apply(){
	QSettings settings;
	for(int i = 0; i < changes.size(); i++){
		settings.setValue(changes.keys().at(i), changes.values().at(i));
	}
}

////////////////////////////

EditorPage::EditorPage(MainWindow* main_window, QWidget* parent) : SettingsPage(main_window, parent){
	QVBoxLayout *l = new QVBoxLayout;
	this->setLayout(l);
	QCheckBox *antialiasing = new QCheckBox(tr("Enable Antialiasing"), this);
	l->addWidget(antialiasing);
	QSpinBox *tolerance = new QSpinBox(this);
	tolerance->setMinimum(1);
	tolerance->setMaximum(50);
	l->addWidget(tolerance);

	QSettings current;
	if(current.value("MapDisplay/antialiasing", QVariant(true)).toBool())
		antialiasing->setChecked(true);
	else
		antialiasing->setChecked(false);
	tolerance->setValue(current.value("MapEditor/click_tolerance", QVariant(5)).toInt());

	connect(antialiasing, SIGNAL(toggled(bool)), this, SLOT(antialiasingClicked(bool)));
	connect(tolerance, SIGNAL(valueChanged(int)), this, SLOT(toleranceValueChanged(int)));
}

void EditorPage::antialiasingClicked(bool res){
	changes.insert("MapDisplay/antialiasing", res);
}

void EditorPage::toleranceValueChanged(int val){
	changes.insert("MapEditor/click_tolerance", QVariant(val));
}

void EditorPage::apply(){
	SettingsPage::apply();
	QSettings settings;
	bool use = settings.value("MapDisplay/antialiasing", QVariant(true)).toBool();
	MapEditorTool::setToolClickTolerance(settings.value("MapEditor/click_tolerance", QVariant(5)).toInt());
	foreach (QWidget *widget, qApp->topLevelWidgets())
	{
		MainWindow* other = qobject_cast<MainWindow*>(widget);
		if(!other)
			continue;
		if(qobject_cast<MapEditorController*>(other->getController())){
			qobject_cast<MapEditorController*>(other->getController())->getMainWidget()->setUsesAntialiasing(use);
		}
	}
}

////////////////////////////

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
	tabWidget = new QTabWidget(this);
	QVBoxLayout *l = new QVBoxLayout;
	l->addWidget(tabWidget);
	this->setLayout(l);
	this->resize(640, 480);

	dbb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel,
												 Qt::Horizontal, this);
	l->addWidget(dbb);
	connect(dbb, SIGNAL(clicked(QAbstractButton*)), this, SLOT(buttonPressed(QAbstractButton*)));

	MainWindow *main_window = qobject_cast<MainWindow*>(parent);
	if(!main_window){
		this->reject();
		return;
	}

	pages.append(new EditorPage(main_window, this));
	tabWidget->addTab(pages.last(), pages.last()->title());
}

void SettingsDialog::buttonPressed(QAbstractButton *b){
	QDialogButtonBox::StandardButton button = dbb->standardButton(b);
	if(button == QDialogButtonBox::Ok){
		foreach(SettingsPage *page, pages){
			page->ok();
		}
		this->accept();
	}
	else if(button == QDialogButtonBox::Apply){
		foreach(SettingsPage *page, pages){
			page->apply();
		}
	}
	else if(button == QDialogButtonBox::Cancel){
		foreach(SettingsPage *page, pages){
			page->cancel();
		}
		this->reject();
	}
}

#include "settings_dialog.moc"
