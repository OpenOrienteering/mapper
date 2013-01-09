/*
 *    Copyright 2012, 2013 Thomas Schöps
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


#ifndef _OPENORIENTEERING_MAP_DIALOG_SCALE_H_
#define _OPENORIENTEERING_MAP_DIALOG_SCALE_H_

#include <map>

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QCheckBox;
QT_END_NAMESPACE

class Map;

class ScaleMapDialog : public QDialog
{
Q_OBJECT
public:
	ScaleMapDialog(QWidget* parent, Map* map);
	
private slots:
	void okClicked();
	
private:
	QLineEdit* scale_edit;
	QCheckBox* adjust_symbols_check;
	QCheckBox* adjust_objects_check;
	QCheckBox* adjust_georeferencing_check;
	QCheckBox* adjust_templates_check;
	QPushButton* ok_button;
	
	Map* map;
};

#endif
