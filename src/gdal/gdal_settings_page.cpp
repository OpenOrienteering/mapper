/*
 *    Copyright 2016-2018 Kai Pastor
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


#include "gdal_settings_page.h"

#include <QtGlobal>
#include <QAbstractItemModel>
#include <QCheckBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QSignalBlocker>
#include <QSpacerItem>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include "settings.h"
#include "fileformats/file_format_registry.h"
#include "gdal/gdal_manager.h"
#include "gdal/ogr_file_format.h"
#include "gui/util_gui.h"
#include "util/backports.h"


namespace OpenOrienteering {

GdalSettingsPage::GdalSettingsPage(QWidget* parent)
: SettingsPage(parent)
{
	auto form_layout = new QFormLayout();
	
	form_layout->addRow(Util::Headline::create(tr("Import with GDAL/OGR:")));
	
	import_dxf = new QCheckBox(tr("DXF"));
	form_layout->addRow(import_dxf);
	
	import_gpx = new QCheckBox(tr("GPX"));
	form_layout->addRow(import_gpx);
	
	import_osm = new QCheckBox(tr("OSM"));
	form_layout->addRow(import_osm);
	
	
	form_layout->addItem(Util::SpacerItem::create(this));
	form_layout->addRow(Util::Headline::create(tr("Templates")));
	
	view_hatch = new QCheckBox(tr("Hatch areas"));
	form_layout->addRow(view_hatch);
	
	view_baseline = new QCheckBox(tr("Baseline view"));
	form_layout->addRow(view_baseline);
	
	
	form_layout->addItem(Util::SpacerItem::create(this));
	form_layout->addRow(Util::Headline::create(tr("Export Options")));

	export_one_layer_per_symbol = new QCheckBox(tr("Create a layer for each symbol"));
	form_layout->addRow(export_one_layer_per_symbol);
	
	
	form_layout->addItem(Util::SpacerItem::create(this));
	form_layout->addRow(Util::Headline::create(tr("Configuration")));
	
	auto layout = new QVBoxLayout(this);
	
	layout->addLayout(form_layout);
	
	parameters = new QTableWidget(1, 2);
	parameters->verticalHeader()->hide();
	parameters->setHorizontalHeaderLabels({ tr("Parameter"), tr("Value") });
	auto header_view = parameters->horizontalHeader();
	header_view->setSectionResizeMode(0, QHeaderView::Stretch);
	header_view->setSectionResizeMode(1, QHeaderView::Stretch);
	header_view->setSectionsClickable(false);
	layout->addWidget(parameters, 1);
	
	updateWidgets();
	
	connect(parameters, &QTableWidget::cellChanged, this, &GdalSettingsPage::cellChange);
}

GdalSettingsPage::~GdalSettingsPage()
{
	// nothing, not inlined
}

QString GdalSettingsPage::title() const
{
	return tr("GDAL/OGR");
}

void GdalSettingsPage::apply()
{
	GdalManager manager;
	manager.setFormatEnabled(GdalManager::DXF, import_dxf->isChecked());
	manager.setFormatEnabled(GdalManager::GPX, import_gpx->isChecked());
	manager.setFormatEnabled(GdalManager::OSM, import_osm->isChecked());
	manager.setAreaHatchingEnabled(view_hatch->isChecked());
	manager.setBaselineViewEnabled(view_baseline->isChecked());
	
	// The file format constructor establishes the extensions.
	auto format = new OgrFileImportFormat();
	FileFormats.unregisterFormat(FileFormats.findFormat(format->id()));
	FileFormats.registerFormat(format);

	manager.setExportOptionEnabled(GdalManager::OneLayerPerSymbol, export_one_layer_per_symbol->isChecked());
	
	const auto old_parameters = manager.parameterKeys();
	
	QStringList new_parameters;
	new_parameters.reserve(parameters->rowCount());
	for (int row = 0, end = parameters->rowCount(); row < end; ++row)
	{
		auto key = parameters->item(row, 0)->text().trimmed();
		if (!key.isEmpty())
		{
			new_parameters.append(key);
			auto value = parameters->item(row, 1)->text();
			manager.setParameterValue(key, value.trimmed());
		}
	}
	for (const auto& key : qAsConst(old_parameters))
	{
		if (!new_parameters.contains(key))
		{
			manager.unsetParameter(key);
		}
	}
	
	Settings::getInstance().applySettings();
}

void GdalSettingsPage::reset()
{
	updateWidgets();
}

void GdalSettingsPage::updateWidgets()
{
	GdalManager manager;
	import_dxf->setChecked(manager.isFormatEnabled(GdalManager::DXF));
	import_gpx->setChecked(manager.isFormatEnabled(GdalManager::GPX));
	import_osm->setChecked(manager.isFormatEnabled(GdalManager::OSM));
	view_hatch->setChecked(manager.isAreaHatchingEnabled());
	view_baseline->setChecked(manager.isBaselineViewEnabled());
	
	export_one_layer_per_symbol->setChecked(manager.isExportOptionEnabled(GdalManager::OneLayerPerSymbol));
	
	auto options = manager.parameterKeys();
	options.sort();
	parameters->setRowCount(options.size() + 1);
	
	QSignalBlocker block(parameters);
	auto row = 0;
	for (const auto& item : qAsConst(options))
	{
		auto key_item = new QTableWidgetItem(item);
		parameters->setItem(row, 0, key_item);
		auto value_item = new QTableWidgetItem(manager.parameterValue(item));
		parameters->setItem(row, 1, value_item);
		++row;
	}
	parameters->setRowCount(row+1);
	parameters->setItem(row, 0, new QTableWidgetItem());
	parameters->setItem(row, 1, new QTableWidgetItem());
}

void GdalSettingsPage::cellChange(int row, int column)
{
	const QString key = parameters->item(row, 0)->text().trimmed();
	const QString value = parameters->item(row, 1)->text();
	
	if (column == 1 && key.isEmpty())
	{
		// Shall not happen
		qWarning("Empty key for modified tag value!");
	}
	else if (column == 0)
	{
		QSignalBlocker block(parameters);
		parameters->item(row, 0)->setText(key); // trimmed
		
		auto last_row = parameters->rowCount() - 1;
		int duplicate = findDuplicateKey(key, row);
		if (key.isEmpty())
		{
			if (row == last_row)
			{
				parameters->item(row, 1)->setText({ });
			}
			else
			{
				parameters->model()->removeRow(row);
				parameters->setCurrentCell(row, 0);
				parameters->setFocus();
			}
		}
		else if (duplicate != row)
		{
			if (row == last_row)
			{
				parameters->item(row, 0)->setText({ });
				parameters->item(row, 0)->setText({ });
			}
			else
			{
				parameters->model()->removeRow(row);
			}
			parameters->setCurrentCell(duplicate, 0);
			parameters->setFocus();
		}
		else
		{
			if (row == last_row)
			{
				parameters->setRowCount(last_row + 2);
				parameters->setItem(last_row + 1, 0, new QTableWidgetItem());
				parameters->setItem(last_row + 1, 1, new QTableWidgetItem());
			}
			
			if (value.isEmpty())
			{
				GdalManager manager;
				parameters->item(row, 1)->setText(manager.parameterValue(key));
			}
		}
	}
}

int GdalSettingsPage::findDuplicateKey(const QString& key, int row) const
{
	for (int i = 0, end = parameters->rowCount(); i < end; ++i)
	{
		if (i != row
		    && parameters->item(i, 0)->text() == key)
		{
			row = i;
			break;
		}
	}
	return row;
}


}  // namespace OpenOrienteering
