/*
 *    Copyright 2012, 2013 Jan Dalheimer
 *    Copyright 2012-2016  Kai Pastor
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

#include <QAbstractButton>
#include <QDialogButtonBox>
#include <QTabWidget>
#include <QVBoxLayout>

#include "../util.h"

#include "widgets/editor_settings_page.h"
#include "widgets/general_settings_page.h"
#ifdef MAPPER_USE_GDAL
#  include "../gdal/gdal_settings_page.h"
#endif


SettingsDialog::SettingsDialog(QWidget* parent)
 : QDialog(parent)
{
	setWindowTitle(tr("Settings"));
	
	QVBoxLayout* layout = new QVBoxLayout();
	this->setLayout(layout);
	
	tab_widget = new QTabWidget();
	layout->addWidget(tab_widget);
	
	button_box = new QDialogButtonBox(
	  QDialogButtonBox::Ok | QDialogButtonBox::Reset | QDialogButtonBox::Cancel | QDialogButtonBox::Help,
	  Qt::Horizontal );
	layout->addWidget(button_box);
	connect(button_box, &QDialogButtonBox::clicked, this, &SettingsDialog::buttonPressed);
	
	// Add all pages
	addPage(new GeneralSettingsPage(this));
	addPage(new EditorSettingsPage(this));
#ifdef MAPPER_USE_GDAL
	addPage(new GdalSettingsPage(this));
#endif
}

SettingsDialog::~SettingsDialog()
{
	// Nothing, not inlined.
}

void SettingsDialog::addPage(SettingsPage* page)
{
	tab_widget->addTab(page, page->title());
}

void SettingsDialog::buttonPressed(QAbstractButton* button)
{
	QDialogButtonBox::StandardButton id = button_box->standardButton(button);
	const int count = tab_widget->count();
	switch (id)
	{
	case QDialogButtonBox::Ok:
		for (int i = 0; i < count; i++)
			static_cast< SettingsPage* >(tab_widget->widget(i))->apply();
		Settings::getInstance().applySettings();
		this->accept();
		break;
		
	case QDialogButtonBox::Reset:
		for (int i = 0; i < count; i++)
			static_cast< SettingsPage* >(tab_widget->widget(i))->reset();
		break;
		
	case QDialogButtonBox::Cancel:
		this->reject();
		break;
		
	case QDialogButtonBox::Help:
		Util::showHelp(this, QLatin1String("settings.html"));
		break;
		
	default:
		Q_UNREACHABLE();
		break;
	}
}

