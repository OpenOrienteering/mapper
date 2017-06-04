/*
 *    Copyright 2012 Thomas Schöps
 *    Copyright 2013, 2017 Kai Pastor
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


#ifndef OPENORIENTEERING_TEMPLATE_POSITIIONING_DIALOG_H
#define OPENORIENTEERING_TEMPLATE_POSITIIONING_DIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
class QComboBox;
class QDoubleSpinBox;
class QRadioButton;
QT_END_NAMESPACE

class TemplateTrack;

/**
 * Dialog allowing for positioning of a template.
 */
class TemplatePositioningDialog : public QDialog
{
Q_OBJECT
public:
	TemplatePositioningDialog(QWidget* parent = nullptr);
	
	bool useRealCoords() const;
	double getUnitScale() const;
	bool centerOnView() const;
	
private:
	QComboBox* coord_system_box;
	QDoubleSpinBox* unit_scale_edit;
	QRadioButton* original_pos_radio;
	QRadioButton* view_center_radio;
	
	Q_DISABLE_COPY(TemplatePositioningDialog)
};

#endif
