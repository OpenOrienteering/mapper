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


#ifndef OPENORIENTEERING_TEMPLATE_LIST_WIDGET_H
#define OPENORIENTEERING_TEMPLATE_LIST_WIDGET_H

#include <memory>

#include <QWidget>
#include <QObject>
#include <QString>

#include "core/map_view.h"

class QAction;
class QBoxLayout;
class QCheckBox;
class QEvent;
class QIcon;
class QTableWidget;
class QToolButton;

namespace OpenOrienteering {

class Map;
class MapEditorController;
class Template;


/**
 * Widget showing the list of templates, including the map layer.
 * Allows to load templates, set their view properties and reoder them,
 * and do various other actions like adjusting template positions.
 */
class TemplateListWidget : public QWidget
{
Q_OBJECT
public:
	TemplateListWidget(Map* map, MapView* main_view, MapEditorController* controller, QWidget* parent = nullptr);
	~TemplateListWidget() override;
	
	void addTemplateAt(Template* new_template, int pos);
	
	static std::unique_ptr<Template> showOpenTemplateDialog(QWidget* dialog_parent, MapEditorController* controller);
	
public slots:
	/**
	 * Sets or clears the all-templates-hidden state.
	 * When all templates are hidden, operations on templates are disabled.
	 */
	void setAllTemplatesHidden(bool value);
	
signals:
	void closeClicked();
	
protected:
	/**
	 * When key events for Qt::Key_Space are sent to the template_table,
	 * this will toggle the visibility of the current template.
	 */
	bool eventFilter(QObject* watched, QEvent* event) override;
	
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
	void cellClicked(int row, int column);
	void cellDoubleClicked(int row, int column);
	
	void moveByHandClicked(bool checked);
	void adjustClicked(bool checked);
	void adjustWindowClosed();
	//void groupClicked();
	void positionClicked(bool checked);
	void importClicked();
	void changeGeorefClicked();
	void moreActionClicked(QAction* action);
	
	void templateAdded(int pos, const Template* temp);
	void templatePositionDockWidgetClosed(Template* temp);
	
	void changeTemplateFile(int pos);
	
	void showOpacitySlider(int row);
	
	/**
	 * Reacts to feature visibility changes.
	 * 
	 * @see MapView::visibilityChanged
	 */
	void updateVisibility(MapView::VisibilityFeature feature, bool active, const Template* temp = nullptr);
	
private:
	void updateAll();
	void addRowItems(int row);
	void updateRow(int row);
	int posFromRow(int row);
	int rowFromPos(int pos);
	Template* getCurrentTemplate();
	
	Map* map;
	MapView* main_view;
	MapEditorController* controller;
	bool mobile_mode;
	int name_column;
	
	QCheckBox* all_hidden_check;
	QTableWidget* template_table;
	QBoxLayout* all_templates_layout;
	
	QAction* duplicate_action;
	QAction* move_by_hand_action;
	QAction* position_action;
	QAction* import_action;
	QAction* georef_action;
	
	// Buttons
	QWidget* list_buttons_group;
	QToolButton* delete_button;
	QToolButton* move_up_button;
	QToolButton* move_down_button;
	QToolButton* move_by_hand_button;
	QToolButton* adjust_button;
	QToolButton* edit_button;
	
	//QToolButton* group_button;
	//QToolButton* more_button;
};


}  // namespace OpenOrienteering

#endif
