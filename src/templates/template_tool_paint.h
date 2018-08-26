/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2017, 2018 Kai Pastor
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

#include <Qt>
#include <QtGlobal>
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
class QListWidget;
class QMouseEvent;
class QPaintEvent;
class QPainter;
class QPushButton;
class QRect;

namespace OpenOrienteering {

class MainWindow;
class Map;
class MapEditorController;
class MapView;
class MapWidget;
class PaintOnTemplatePaletteWidget;
class Template;


/**
 * A tool to paint on image templates.
 */
class PaintOnTemplateTool : public MapEditorTool
{
Q_OBJECT
public:
	PaintOnTemplateTool(MapEditorController* editor, QAction* tool_action);
	~PaintOnTemplateTool() override;
	
	void setTemplate(Template* temp);
	
	void init() override;
	const QCursor& getCursor() const override;
	
	bool mousePressEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget) override;
	bool mouseMoveEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget) override;
	bool mouseReleaseEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget) override;
	
	void draw(QPainter* painter, MapWidget* widget) override;
	
public slots:
	void templateDeleted(int pos, const Template* temp);
	void colorSelected(const QColor& color);
	void undoSelected();
	void redoSelected();
	
private:
	bool dragging = false;
	bool erasing  = false;
	QColor paint_color = Qt::black;
	QRectF map_bbox;
	std::vector<MapCoordF> coords;
	
	Template* temp = nullptr;
	QPointer<PaintOnTemplatePaletteWidget> widget;
	
	static int erase_width;
	
	Q_DISABLE_COPY(PaintOnTemplateTool)
};



/**
 * A color selection widget for PaintOnTemplateTool.
 */
class PaintOnTemplatePaletteWidget : public QWidget
{
Q_OBJECT
public:
	PaintOnTemplatePaletteWidget(bool close_on_selection);
	~PaintOnTemplatePaletteWidget() override;

	QColor getSelectedColor();
	
	QSize sizeHint() const override;
	
signals:
	void colorSelected(const QColor& color);
	void undoSelected();
	void redoSelected();
	
protected:
	void paintEvent(QPaintEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	
private:
	int getNumFieldsX() const;
	int getNumFieldsY() const;
	QColor getFieldColor(int x, int y) const;
	bool isUndoField(int x, int y) const;
	bool isRedoField(int x, int y) const;
	
	void drawIcon(QPainter* painter, const QString& resource_path, const QRect& field_rect);
	
	Qt::MouseButtons::Int pressed_buttons;
	int selected_color;
	bool close_on_selection;
};



/**
 * Template selection dialog for PaintOnTemplateTool.
*/
class PaintOnTemplateSelectDialog : public QDialog
{
Q_OBJECT
public:
	PaintOnTemplateSelectDialog(Map* map, MapView* view, Template* selected, MainWindow* parent);
	
	Template* getSelectedTemplate() const { return selection; }
	
protected:
	void drawClicked();
	
	Template* addNewTemplate() const;
	
private:
	Map* map;
	MapView* view;
	Template* selection = nullptr;
	QListWidget* template_list;
	QPushButton* draw_button;
};


}  // namespace OpenOrienteering

#endif
