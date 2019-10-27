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


#ifndef OPENORIENTEERING_TEMPLATE_POSITION_DOCK_WIDGET_H
#define OPENORIENTEERING_TEMPLATE_POSITION_DOCK_WIDGET_H

#include <QDockWidget>
#include <QObject>

class QEvent;
class QCloseEvent;
class QLineEdit;
class QWidget;

namespace OpenOrienteering {

class MapEditorController;
class Template;


/** Dock widget allowing to enter template positioning properties numerically. */
class TemplatePositionDockWidget : public QDockWidget
{
Q_OBJECT
public:
	TemplatePositionDockWidget(Template* temp, MapEditorController* controller, QWidget* parent = nullptr);
	
	void updateValues();
	
	bool event(QEvent* event) override;
	void closeEvent(QCloseEvent* event) override;
	
public slots:
	void templateChanged(int index, const OpenOrienteering::Template* temp);
	void templateDeleted(int index, const OpenOrienteering::Template* temp);
	void valueChanged();
	
private:
	QLineEdit* x_edit;
	QLineEdit* y_edit;
	QLineEdit* scale_x_edit;
	QLineEdit* scale_y_edit;
	QLineEdit* rotation_edit;
	QLineEdit* shear_edit;
	
	bool react_to_changes;
	Template* temp;
	MapEditorController* controller;
};


}  // namespace OpenOrienteering

#endif
