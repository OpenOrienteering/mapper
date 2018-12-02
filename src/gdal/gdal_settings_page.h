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

#ifndef OPENORIENTEERING_GDAL_SETTINGS_PAGE_H
#define OPENORIENTEERING_GDAL_SETTINGS_PAGE_H

#include <QObject>
#include <QString>

#include "gui/widgets/settings_page.h"

class QCheckBox;
class QTableWidget;
class QWidget;

namespace OpenOrienteering {


class GdalSettingsPage : public SettingsPage
{
Q_OBJECT
public:
	explicit GdalSettingsPage(QWidget* parent = nullptr);
	
	~GdalSettingsPage() override;
	
	QString title() const override;

	void apply() override;
	
	void reset() override;
	
protected:
	void updateWidgets();
	
	void cellChange(int row, int column);
	
	int findDuplicateKey(const QString& key, int row) const;
	
private:
	QCheckBox* import_dxf;
	QCheckBox* import_gpx;
	QCheckBox* import_osm;
	QCheckBox* view_hatch;
	QCheckBox* view_baseline;
	QTableWidget* parameters;
};


}  // namespace OpenOrienteering

#endif
