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


#ifndef _OPENORIENTEERING_PRINT_WIDGET_H_
#define _OPENORIENTEERING_PRINT_WIDGET_H_

#include <QWidget>
#include <QPrinterInfo>

#include "../map_editor.h"
#include "../tool.h"

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

/**
 * The print widget lets the user adjust methods, devices and parameters
 * for printing and export.
 */
class PrintWidget : public QWidget
{
Q_OBJECT
public:
	PrintWidget(Map* map, MainWindow* main_window, MapView* main_view, MapEditorController* editor, QWidget* parent = NULL);
	virtual ~PrintWidget();
	
	virtual QSize sizeHint() const;
	
	float getPrintAreaLeft();
	void setPrintAreaLeft(float value);
	float getPrintAreaTop();
	void setPrintAreaTop(float value);
	
	/// Returns the printing margins returned by Qt. Only valid if a printer is selected.
	void getMargins(float& top, float& left, float& bottom, float& right);
	
	/// Returns the area of the printed part on the map after applying the scaling from the "print/export in different scale" option
	QRectF getEffectivePrintArea();
	
	/// Returns the scaling factor resulting from the "print/export in different scale" option
	float calcScaleFactor();
	
	/// Returns true if an exporter is active (instead of a printer)
	bool exporterSelected();
	
public slots:
	/** 
	 * Sets the active state of the print widget.
	 * 
	 * When the widget becomes active, it activates a tool on the map editor
	 * which allows to move the print area. When the widget becomes inactive,
	 * the tool is removed.
	 */
	void setActive(bool state);
	
signals:
	/**
	 * This signal is emitted when a print or export job has been started 
	 * and finished. 
	 * 
	 * The signal is not emitted when the widget is hidden 
	 * (cf. QDialog::finished(int result) ).
	 */
	void finished(int result);
	
protected slots:
	void printMap(QPrinter* printer);
	void setPrinterSettings(QPrinter* printer);
	void savePrinterSettings();
	
	void currentDeviceChanged();
	void pageOrientationChanged();
	void pageFormatChanged();
	void showTemplatesClicked();
	void showGridClicked();
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
	float margin_top, margin_left, margin_bottom, margin_right;
	
	bool have_prev_paper_size;
	int prev_paper_size;
	
	QComboBox* device_combo;
	QComboBox* page_orientation_combo;
	QComboBox* page_format_combo;
	QLabel* dpi_label;
	QComboBox* dpi_combo;
	QLabel* copies_label;
	QLineEdit* copies_edit;
	QCheckBox* show_templates_check;
	QCheckBox* show_grid_check;
	QCheckBox* overprinting_check;
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

#endif
