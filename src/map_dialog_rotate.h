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


#ifndef _OPENORIENTEERING_MAP_DIALOG_ROTATE_H_
#define _OPENORIENTEERING_MAP_DIALOG_ROTATE_H_

#include <map>

#include <QDialog>

QT_BEGIN_NAMESPACE
class QDoubleSpinBox;
class QCheckBox;
QT_END_NAMESPACE

class Map;

class RotateMapDialog : public QDialog
{
Q_OBJECT
public:
	RotateMapDialog(QWidget* parent, Map* map);
	
private slots:
	void okClicked();
	
private:
	QDoubleSpinBox* rotation_edit;
	QCheckBox* adjust_georeferencing_check;
	QCheckBox* adjust_declination_check;
	
	Map* map;
};

#endif
