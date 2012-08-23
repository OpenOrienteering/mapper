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


#ifndef _OPENORIENTEERING_PRINT_DOCK_WIDGET_H_
#define _OPENORIENTEERING_PRINT_DOCK_WIDGET_H_

#include <QWidget>
#include <QPrinterInfo>

#include "map_editor.h"
#include "tool.h"

QT_BEGIN_NAMESPACE
class QPushButton;
class QComboBox;
class QLineEdit;
class QLabel;
class QCheckBox;
QT_END_NAMESPACE

class Map;
class MapView;
class MainWindow;
class PrintTool;

class PrintWidget : public EditorDockWidgetChild
{
Q_OBJECT
public:
	PrintWidget(Map* map, MainWindow* main_window, MapView* main_view, MapEditorController* editor, QWidget* parent = NULL);
	virtual ~PrintWidget();
	
	virtual QSize sizeHint() const;
	
	void activate();
	
	float getPrintAreaLeft();
	void setPrintAreaLeft(float value);
	float getPrintAreaTop();
	void setPrintAreaTop(float value);
	
	QRectF getEffectivePrintArea();
    QRectF getPaperArea();
	
	/// Returns the scaling factor resulting from the "print/export in different scale" option
	float calcScaleFactor();
	
protected slots:
	void printMap(QPrinter* printer);
	void setPrinterSettings(QPrinter* printer);
	void closed();
	
	void currentDeviceChanged();
	void pageOrientationChanged();
	void pageFormatChanged();
	void showTemplatesClicked();
	void printAreaPositionChanged();
	void printAreaSizeChanged();
	void updatePrintAreaSize();
	void centerPrintArea();
	void centerPrintAreaClicked();
	void differentScaleClicked(bool checked);
	void differentScaleEdited(QString text);
	void previewClicked();
	void printClicked();
	
private:
	enum Exporters
	{
		PdfExporter = -1,
		ImageExporter = -2
	};
	
	void drawMap(QPaintDevice* paint_device, float dpi, const QRectF& page_rect, bool white_background);
	void addPaperSize(QPrinter::PaperSize size);
	bool checkForEmptyMap();
	
	float print_width;
	float print_height;
	
	bool have_prev_paper_size;
	int prev_paper_size;
	
	QComboBox* device_combo;
	QComboBox* page_orientation_combo;
	QComboBox* page_format_combo;
	QLabel* dpi_label;
	QLineEdit* dpi_edit;
	QLabel* copies_label;
	QLineEdit* copies_edit;
	QCheckBox* show_templates_check;
	QLineEdit* left_edit;
	QLineEdit* top_edit;
	QLineEdit* width_edit;
	QLineEdit* height_edit;
	QPushButton* center_button;
	QCheckBox* different_scale_check;
	QLineEdit* different_scale_edit;
	QPushButton* preview_button;
	QPushButton* print_button;
	
	QList<QPrinterInfo> printers;
	PrintTool* print_tool;
	
	Map* map;
	MainWindow* main_window;
	MapView* main_view;
	MapEditorController* editor;
	bool react_to_changes;
};

class PrintTool : public MapEditorTool
{
Q_OBJECT
public:
	PrintTool(MapEditorController* editor, PrintWidget* widget);
	
	virtual void init();
	virtual QCursor* getCursor() {return cursor;}
	
	virtual bool mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	
    virtual void draw(QPainter* painter, MapWidget* widget);
	
	void updatePrintArea();
	
	static QCursor* cursor;
	
private:
	void updateDragging(MapCoordF mouse_pos_map);
    void drawBetweenRects(QPainter* painter, const QRect &outer, const QRect &inner) const;

	bool dragging;
	PrintWidget* widget;
	MapCoordF click_pos_map;
};

#endif
