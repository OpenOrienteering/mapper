/*
 *    Copyright 2011 Thomas Schöps
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


#ifndef _OPENORIENTEERING_MAP_EDITOR_H_
#define _OPENORIENTEERING_MAP_EDITOR_H_

#include "main_window.h"

#include <QDockWidget>

class MapView;
class Map;
class MapWidget;
class EditorDockWidget;

class MapEditorController : public MainWindowController
{
Q_OBJECT
public:
	MapEditorController();
	MapEditorController(Map* map);
	~MapEditorController();
	
	virtual bool save(const QString& path);
	virtual bool load(const QString& path);
	
    virtual void attach(MainWindow* window);
    virtual void detach();
	
public slots:
	void undo();
	void redo();
	void cut();
	void copy();
	void paste();
	
	void showSymbolWindow(bool show);
	void showColorWindow(bool show);
	void loadSymbolsFromClicked();
	void loadColorsFromClicked();
	
private:
	Map* map;
	MapView* main_view;
	MapWidget* map_widget;
	
	QAction* color_window_act;
	QDockWidget* color_dock_widget;
};

class MapWidget : public QWidget
{
public:
	MapWidget(QWidget* parent = NULL);
	~MapWidget();
	
	void setMapView(MapView* view);
	
protected:
	virtual void paintEvent(QPaintEvent* event);
	virtual void resizeEvent(QResizeEvent* event);
	virtual void mousePressEvent(QMouseEvent* event);
	virtual void mouseMoveEvent(QMouseEvent* event);
	virtual void mouseReleaseEvent(QMouseEvent* event);
	virtual void wheelEvent(QWheelEvent* event);
	
private:
	MapView* view;
};

/// Custom QDockWidget which unchecks the associated menu action when closed
class EditorDockWidget : public QDockWidget
{
public:
	EditorDockWidget(const QString title, QAction* action, QWidget* parent = NULL);
    virtual void closeEvent(QCloseEvent* event);
private:
	QAction* action;
};

#endif
