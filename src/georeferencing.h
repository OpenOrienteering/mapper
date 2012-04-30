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


#ifndef _OPENORIENTEERING_GEOREFERENCING_H_
#define _OPENORIENTEERING_GEOREFERENCING_H_

#include <QDialog>

#include "gps_coordinates.h"

QT_BEGIN_NAMESPACE
class QLineEdit;
QT_END_NAMESPACE

class GeoreferencingDialog : public QDialog
{
Q_OBJECT
public:
	GeoreferencingDialog(QWidget* parent, const GPSProjectionParameters* initial_values = NULL);
	
	inline const GPSProjectionParameters& getParameters() const {return params;}
	
protected slots:
	void editChanged();
	
private:
	GPSProjectionParameters params;
	
	QLineEdit* lat_edit;
	QLineEdit* lon_edit;
	QPushButton* ok_button;
};

#endif
