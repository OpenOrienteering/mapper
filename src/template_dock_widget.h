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


#ifndef _OPENORIENTEERING_TEMPLATE_DOCK_WIDGET_H_
#define _OPENORIENTEERING_TEMPLATE_DOCK_WIDGET_H_

#include <qglobal.h>
#if QT_VERSION < 0x050000
#include <QtGui>
#else
#include <QtWidgets>
#endif

class Map;
class MapEditorController;
class MapView;
class Template;
class PercentageDelegate;

/**
 * Widget showing the list of templates, including the map layer.
 * Allows to load templates, set their view properties and reoder them,
 * and do various other actions like adjusting template positions.
 */
class TemplateWidget : public QWidget
{
Q_OBJECT
public:
	TemplateWidget(Map* map, MapView* main_view, MapEditorController* controller, QWidget* parent = NULL);
	virtual ~TemplateWidget();
	
	void addTemplateAt(Template* new_template, int pos);
	
	static Template* showOpenTemplateDialog(QWidget* dialog_parent, MapEditorController* controller);
	
public slots:
	/**
	 * Sets or clears the all-templates-hidden state.
	 * When all templates are hidden, operations on templates are disabled.
	 */
	void setAllTemplatesHidden(bool value);
	
protected:
	/**
	 * When key events for Qt::Key_Space are sent to the template_table,
	 * this will toggle the visibility of the current template.
	 */
	virtual bool eventFilter(QObject* watched, QEvent* event);
	
	/**
	 * Returns a new QToolButton with a unified appearance.
	 */
	QToolButton* newToolButton(const QIcon& icon, const QString& text);
	
protected slots:
	void newTemplate(QAction* action);
	void openTemplate();
	void deleteTemplate();
	void duplicateTemplate();
	void moveTemplateUp();
	void moveTemplateDown();
	void showHelp();
	
	void cellChange(int row, int column);
	void updateButtons();
	void currentCellChange(int current_row, int current_column, int previous_row, int previous_column);
	void cellDoubleClick(int row, int column);
	void updateDeleteButtonText();
	
	void moveByHandClicked(bool checked);
	void adjustClicked(bool checked);
	void adjustWindowClosed();
	//void groupClicked();
	void positionClicked(bool checked);
	void moreActionClicked(QAction* action);
	
	void templateAdded(int pos, Template* temp);
	void templatePositionDockWidgetClosed(Template* temp);
	
	void changeTemplateFile();
	
private:
	void addRow(int row);
	void updateRow(int row);
	int posFromRow(int row);
	int rowFromPos(int pos);
	Template* getCurrentTemplate();
	
	PercentageDelegate* percentage_delegate;
	
	QCheckBox* all_hidden_check;
	QTableWidget* template_table;
	QBoxLayout* all_templates_layout;
	
	QAction* duplicate_action;
	QAction* move_by_hand_action;
	
	// Buttons
	QWidget* list_buttons_group;
	QToolButton* delete_button;
	QToolButton* move_up_button;
	QToolButton* move_down_button;
	QToolButton* georef_button;
	QToolButton* move_by_hand_button;
	QToolButton* adjust_button;
	QToolButton* position_button;
	//QToolButton* group_button;
	//QToolButton* more_button;
	
	Map* map;
	MapView* main_view;
	MapEditorController* controller;
	bool react_to_changes;
};

#endif
