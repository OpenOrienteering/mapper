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


#ifndef _OPENORIENTEERING_TEMPLATE_ADJUST_H_
#define _OPENORIENTEERING_TEMPLATE_ADJUST_H_

#include <QWidget>
#include <QDockWidget>

#include "map_editor_activity.h"
#include "template.h"
#include "tool.h"

QT_BEGIN_NAMESPACE
class QGridLayout;
class QPushButton;
class QTableWidget;
class QCheckBox;
QT_END_NAMESPACE

class Template;
class TemplateAdjustDockWidget;
class TemplateAdjustWidget;

/// Activity which allows the positioning of a template by specifying source-destination pass point pairs
class TemplateAdjustActivity : public MapEditorActivity
{
Q_OBJECT
public:
	TemplateAdjustActivity(Template* temp, MapEditorController* controller);
	virtual ~TemplateAdjustActivity();
	
	virtual void init();
	virtual void draw(QPainter* painter, MapWidget* widget);
	
	inline TemplateAdjustDockWidget* getDockWidget() const {return dock;}
	
	static void drawCross(QPainter* painter, QPoint midpoint, QColor color);
	static int findHoverPoint(Template* temp, QPoint mouse_pos, MapWidget* widget, bool& point_src);
	static bool calculateTemplateAdjust(Template* temp, TemplateTransform& out, QWidget* dialog_parent);
	
	static float cross_radius;
	
public slots:
	void templateChanged(int index, Template* temp);
	void templateDeleted(int index, Template* temp);
	
private:
	TemplateAdjustDockWidget* dock;
	TemplateAdjustWidget* widget;
	MapEditorController* controller;
};

/// Custom QDockWidget which closes the assigned activity when it is closed
class TemplateAdjustDockWidget : public QDockWidget
{
Q_OBJECT
public:
	TemplateAdjustDockWidget(const QString title, MapEditorController* controller, QWidget* parent = 0);
	virtual bool event(QEvent* event);
	virtual void closeEvent(QCloseEvent* event);
	
signals:
	void closed();
	
private:
	MapEditorController* controller;
};

/// The widget contained in a TemplateAdjustDockWidget
class TemplateAdjustWidget : public QWidget
{
Q_OBJECT
public:
	TemplateAdjustWidget(Template* temp, MapEditorController* controller, QWidget* parent = NULL);
	virtual ~TemplateAdjustWidget();
	
	void addPassPoint(MapCoordF src, MapCoordF dest);
	void deletePassPoint(int number);
	void stopTemplateAdjust();	// disables the georeferencing tools, if active
	
	void updatePointErrors();
	void updateAllRows();
	void updateRow(int row);
	void updateDirtyRect(bool redraw = true);
	
	inline Template* getTemplate() const {return temp;}
	
protected slots:
	void newClicked(bool checked);
	void moveClicked(bool checked);
	void deleteClicked(bool checked);
	
	void applyClicked(bool checked);
	void clearAndApplyClicked(bool checked);
	void clearAndRevertClicked(bool checked);
	
private:
	void addRow(int row);
	void updateActions();
	void clearPassPoints();
	
	QTableWidget* table;
	QCheckBox* apply_check;
	QPushButton* clear_and_apply_button;
	QPushButton* clear_and_revert_button;
	
	QAction* new_act;
	QAction* move_act;
	QAction* delete_act;
	
	Template* temp;
	MapEditorController* controller;
	bool react_to_changes;
};

/// Base class for tools which involve selecting pass points for editing
class TemplateAdjustEditTool : public MapEditorTool
{
Q_OBJECT
public:
	TemplateAdjustEditTool(MapEditorController* editor, QAction* tool_button, TemplateAdjustWidget* widget);
	
	virtual void draw(QPainter* painter, MapWidget* widget);
	
protected:
	void findHoverPoint(QPoint mouse_pos, MapWidget* map_widget);
	
	int active_point;			// -1 if no point active
	bool active_point_is_src;
	
	TemplateAdjustWidget* widget;
};

/// Tool to add a new pass point
class TemplateAdjustAddTool : public MapEditorTool
{
Q_OBJECT
public:
	TemplateAdjustAddTool(MapEditorController* editor, QAction* tool_button, TemplateAdjustWidget* widget);
	
	virtual void init();
	virtual QCursor* getCursor() {return cursor;}
	
	virtual bool mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool keyPressEvent(QKeyEvent* event);
	
	virtual void draw(QPainter* painter, MapWidget* widget);
	
	static QCursor* cursor;
	
protected:
	void setDirtyRect(MapCoordF mouse_pos);
	
	bool first_point_set;
	MapCoordF first_point;
	MapCoordF mouse_pos;
	
	TemplateAdjustWidget* widget;
};

/// Tool to move pass points
class TemplateAdjustMoveTool : public TemplateAdjustEditTool
{
Q_OBJECT
public:
	TemplateAdjustMoveTool(MapEditorController* editor, QAction* tool_button, TemplateAdjustWidget* widget);
	
	virtual void init();
	virtual QCursor* getCursor() {return cursor;}
	
	virtual bool mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	
	static QCursor* cursor;
	static QCursor* cursor_invisible;
	
protected:
	void setActivePointPosition(MapCoordF map_coord);
	
	bool dragging;
	MapCoordF dragging_offset;	// offset from cursor pos to dragged point center
};

/// Tool to delete pass points
class TemplateAdjustDeleteTool : public TemplateAdjustEditTool
{
Q_OBJECT
public:
	TemplateAdjustDeleteTool(MapEditorController* editor, QAction* tool_button, TemplateAdjustWidget* widget);
	
	virtual bool mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	
	virtual void init();
	virtual QCursor* getCursor() {return cursor;}
	
	static QCursor* cursor;
};

#endif
