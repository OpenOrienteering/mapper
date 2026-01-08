/*
 *    Copyright 2013 Thomas Schöps
 *    Copyright 2015, 2017-2019 Kai Pastor
 *    Copyright 2024 Matthias Kühlewein
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


#ifndef OPENORIENTEERING_DISTRIBUTE_POINTS_TOOL_H
#define OPENORIENTEERING_DISTRIBUTE_POINTS_TOOL_H

#include <QDialog>
#include <QObject>

// IWYU pragma: no_include <QString>
class QDoubleSpinBox;
class QCheckBox;
class QSpinBox;
class QWidget;

namespace OpenOrienteering {

class Map;
class PathObject;
class PointObject;
class PointSymbol;

class DistributePointsDialog : public QDialog
{
Q_OBJECT
public:
	/**
	 * Creates a new DistributePointsDialog object.
	 */
	DistributePointsDialog(QWidget* parent, Map* map, const PointSymbol* point);
	
	~DistributePointsDialog() override;
	
private slots:
	void okClicked();
	
private:
	void distributePoints(const PathObject* path);
	
	QSpinBox* num_points_edit;
	QCheckBox* points_at_ends_check;
	
	QCheckBox* rotate_symbols_check;
	QDoubleSpinBox* additional_rotation_edit;
	
	std::vector<PointObject*> created_objects;
	
	Map* map;
	const PointSymbol* point;
};


}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_DISTRIBUTE_POINTS_TOOL_H
