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


#ifndef _OPENORIENTEERING_TEMPLATE_POSITION_DOCK_WIDGET_H_
#define _OPENORIENTEERING_TEMPLATE_POSITION_DOCK_WIDGET_H_

#include <QDockWidget>

QT_BEGIN_NAMESPACE
class QLineEdit;
QT_END_NAMESPACE

class MapEditorController;
class Template;
class TemplateWidget;

class TemplatePositionDockWidget : public QDockWidget
{
Q_OBJECT
public:
	TemplatePositionDockWidget(Template* temp, MapEditorController* controller, QWidget* parent = NULL);
	
	void updateValues();
	
	virtual bool event(QEvent* event);
	virtual void closeEvent(QCloseEvent* event);
	
public slots:
	void templateChanged(int index, Template* temp);
	void templateDeleted(int index, Template* temp);
	void valueChanged();
	
private:
	QLineEdit* x_edit;
	QLineEdit* y_edit;
	QLineEdit* scale_x_edit;
	QLineEdit* scale_y_edit;
	QLineEdit* rotation_edit;
	
	bool react_to_changes;
	Template* temp;
	MapEditorController* controller;
};

#endif
