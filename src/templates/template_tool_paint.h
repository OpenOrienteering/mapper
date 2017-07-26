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


#ifndef OPENORIENTEERING_TEMPLATE_TOOL_PAINT_H
#define OPENORIENTEERING_TEMPLATE_TOOL_PAINT_H

#include <vector>

#include <QColor>
#include <QDialog>
#include <QObject>
#include <QPointer>
#include <QRectF>
#include <QSize>
#include <QString>
#include <QWidget>

#include "core/map_coord.h"
#include "tools/tool.h"

class QAction;
class QCursor;
class QListWidgetItem;
class QMouseEvent;
class QPaintEvent;
class QPainter;
class QPushButton;
class QRect;

class Map;
class MapEditorController;
class MapWidget;
class Template;
class PaintOnTemplatePaletteWidget;


/** Tool to paint on image templates. */
class PaintOnTemplateTool : public MapEditorTool
{
Q_OBJECT
public:
	PaintOnTemplateTool(MapEditorController* editor, QAction* tool_action, Template* temp);
	virtual ~PaintOnTemplateTool();
	
	virtual void init();
	virtual const QCursor& getCursor() const;
	
	virtual bool mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	
	virtual void draw(QPainter* painter, MapWidget* widget);
	
public slots:
	void templateDeleted(int pos, const Template* temp);
	void colorSelected(QColor color);
	void undoSelected();
	void redoSelected();
	
private:
	bool dragging;
	bool erasing;
	QColor paint_color;
	QRectF map_bbox;
	std::vector<MapCoordF> coords;
	
	Template* temp;
	QPointer<PaintOnTemplatePaletteWidget> widget;
	
	static int erase_width;
};

/** Color selection widget for PaintOnTemplateTool. */
class PaintOnTemplatePaletteWidget : public QWidget
{
Q_OBJECT
public:
	PaintOnTemplatePaletteWidget(bool close_on_selection);
	~PaintOnTemplatePaletteWidget();

	QColor getSelectedColor();
	
	virtual QSize sizeHint() const;
	
signals:
	void colorSelected(QColor color);
	void undoSelected();
	void redoSelected();
	
protected:
	virtual void paintEvent(QPaintEvent* event);
	virtual void mousePressEvent(QMouseEvent* event);
	virtual void mouseReleaseEvent(QMouseEvent* event);
	
private:
	int getNumFieldsX() const;
	int getNumFieldsY() const;
	QColor getFieldColor(int x, int y) const;
	bool isUndoField(int x, int y) const;
	bool isRedoField(int x, int y) const;
	
	void drawIcon(QPainter* painter, const QString& resource_path, const QRect& field_rect);
	
	int pressed_buttons;
	int selected_color;
	bool close_on_selection;
};

/** Template selection dialog for PaintOnTemplateTool. */
class PaintOnTemplateSelectDialog : public QDialog
{
Q_OBJECT
public:
	PaintOnTemplateSelectDialog(Map* map, QWidget* parent);
	
	inline Template* getSelectedTemplate() const {return selection;}
	
protected slots:
	void currentTemplateChanged(QListWidgetItem* current, QListWidgetItem* previous);
	
private:
	Template* selection;
	QPushButton* draw_button;
};

#endif
