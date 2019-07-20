/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2016  Kai Pastor
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


#ifdef QT_PRINTSUPPORT_LIB

#ifndef OPENORIENTEERING_PRINT_WIDGET_H
#define OPENORIENTEERING_PRINT_WIDGET_H

#include <QFlags>
#include <QList>
#include <QObject>
#include <QPrinter>
// IWYU pragma: no_include <QRectF>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QWidget>


class QAbstractButton;
class QButtonGroup;
class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QDoubleSpinBox;
class QFormLayout;
class QLabel;
class QPushButton;
class QPrinterInfo;
class QRectF;
class QScrollArea;
class QSpinBox;
class QToolButton;

namespace OpenOrienteering {

class MainWindow;
class Map;
class MapEditorController;
class MapPrinter;
class MapPrinterOptions;
class MapPrinterPageFormat;
class MapView;
class PrintTool;


/**
 * The print widget lets the user adjust targets and parameters
 * for printing and export.
 */
class PrintWidget : public QWidget
{
Q_OBJECT
public:
	enum TaskFlag
	{
		EXPORT_FLAG    = 0x02,
		MULTIPAGE_FLAG = 0x04,
		
		UNDEFINED_TASK     = 0x00,
		PRINT_TASK         = 0x14, // 0x10 | 0x04
		EXPORT_PDF_TASK    = 0x26, // 0x20 | 0x04 | 0x02
		EXPORT_IMAGE_TASK  = 0x42, // 0x40        | 0x02
		
		END_JOB_TYPE
	};
	Q_DECLARE_FLAGS(TaskFlags, TaskFlag)
	
	/** Constructs a new print widget. */
	PrintWidget(Map* map, MainWindow* main_window, MapView* main_view, MapEditorController* editor, QWidget* parent = nullptr);
	
	/** Destroys the widget. */
	~PrintWidget() override;
	
	/** Indicates the default widget size. */
	QSize sizeHint() const override;
	
	/** Returns a translated name for the given paper size. */
	static QString toString(QPrinter::PaperSize size);
	
public slots:
	/** Changes the type of the print or export task. */
	void setTask(TaskFlags type);
	
	/** Saves the print or export settings. */
	void savePrinterConfig() const;
	
	/** 
	 * Sets the active state of the print widget.
	 * 
	 * When the widget becomes active, it saves the map view state and 
	 * activates a tool on the map editor which allows to move the print area.
	 * When the widget becomes inactive, the tool is removed, and the map view
	 * state is restored.
	 */
	void setActive(bool active);
	
	/** Sets the widget's (print/export) target. */
	void setTarget(const QPrinterInfo* target);
	
	/** Sets the format of a single page. */
	void setPageFormat(const MapPrinterPageFormat& format);
	
	/** Sets the exported area. */
	void setPrintArea(const QRectF& area);
	
	/** Sets output options: resolution, overprinting. */
	void setOptions(const MapPrinterOptions& options);
	
	/** Listens to view feature changes. */
	void onVisibilityChanged();
	
signals:
	/**
	 * This signal is emitted when the type of task changes.
	 * It may be used to set a window title.
	 */
	void taskChanged(const QString& name);
	
	/**
	 * This signal is emitted when a print or export job has been started 
	 * and finished. 
	 * 
	 * The signal is not emitted when the widget is hidden 
	 * (cf. QDialog::finished(int result) ).
	 */
	void finished(int result);
	
	/**
	 * This signal is emitted when the dialog's close button is clicked.
	 */
	void closeClicked();
	
protected slots:
	/** This slot reacts to changes of the target combobox. */
	void targetChanged(int index) const;
	
	/** Opens a dialog to change printer properties. */
	void propertiesClicked();
	
	/** This slot reacts to changes of the paper size combobox. */
	void paperSizeChanged(int index) const;
	
	/** This slot reacts to changes of the paper dimension widgets. */
	void paperDimensionsChanged() const;
	
	/** This slot reacts to changes of the page orientation widget. */
	void pageOrientationChanged(int id) const;
	
	/** This slot reacts to changes of the print area policy combobox. */
	void printAreaPolicyChanged(int index);
	
	/** This slot applies the map area policy to the current area. */
	void applyPrintAreaPolicy() const;
	
	/** This slot applies the center map area policy to the current area. */
	void applyCenterPolicy() const;
	
	/** This slot reacts to changes of print area position widgets. */
	void printAreaMoved();
	
	/** This slot reacts to changes of print area size widget. */
	void printAreaResized();
	
	/** This slot reacts to changes to the page overlap widget. */
	void overlapEdited(double overlap);
	
	/** This slot is called when the resolution widget signals that editing finished. */
	void resolutionEdited();
	
	/** This slot enables the alternative scale widget, or resets it. */
	void differentScaleClicked(bool checked);
	
	/** This slot reacts to changes in the alternative scale widget. */
	void differentScaleEdited(int value);
	
	/** This slot reacts to changes in the presence of spot colors in the map.
	 *  The following features are enabled only when spot colors are present:
	 *  - spot color separations
	 *  - overprinting simulation
	 */
	void spotColorPresenceChanged(bool has_spot_colors);
	
	/** This slot reacts to changes in the print mode buttons. */
	void printModeChanged(QAbstractButton* button);
	
	/** This slot reacts to changes of the "Show template" option. */
	void showTemplatesClicked(bool checked);
	
	/** This slot reacts to changes of the "Show grid" option. */
	void showGridClicked(bool checked);
	
	/** This slot reacts to changes of the "Simulate overprinting" option. */
	void overprintingClicked(bool checked);

	/** This slot reacts to changes of the "Color mode" option. */
	void colorModeChanged();
	
	/** Opens a preview window. */
	void previewClicked();
	
	/** Starts printing and terminates this dialog. */
	void printClicked();
	
protected:
	/** Alternative policies of handling the print area. */
	enum PrintAreaPolicy
	{
		SinglePage         = 2,
		CustomArea         = 4
	};
	
	/** Re-initializes the list of print/export targets. */
	void updateTargets();
	
	/** Updates the list of paper sizes from the given target. */
	void updatePaperSizes(const QPrinterInfo* target) const;
	
	/** Updates the list of resolutions from the given target. */
	void updateResolutions(const QPrinterInfo* target) const;
	
	/** Updates the color mode combobox from target and mode settings. */
	void updateColorMode();
	
	/** A list of paper sizes which is used when the target does not specify
	 *  supported paper sizes. */
	QList<QPrinter::PaperSize> defaultPaperSizes() const;
	
	/** Moves the given rectangle to a position where it is centered on the
	 *  map for the current output options. */
	void centerOnMap(QRectF& area) const;
	
	/** Shows a warning and returns true if the output would be empty. */
	bool checkForEmptyMap();
	
	/** Sets the enabled state of the page overlap field. */
	void setOverlapEditEnabled(bool state = true) const;
	
	/** Checks whether the template order warning needs to be displayed. */
	void checkTemplateConfiguration();
	
	/** Exports to an image file. */
	void exportToImage();

	/** Export a world file */
	void exportWorldFile(const QString& path) const;
	
	/** Exports to a PDF file. */
	void exportToPdf();
	
	/** Print to a printer. */
	void print();
	
private:
	enum Exporters
	{
		PdfExporter = -1,
		ImageExporter = -2
	};
	TaskFlags task;
	
	QFormLayout* layout;
	QWidget* scrolling_content;
	QScrollArea* scroll_area;
	QDialogButtonBox* button_box;
	
	QComboBox* target_combo;
	QToolButton* printer_properties_button;
	QComboBox* paper_size_combo;
	QWidget*   page_orientation_widget;
	QButtonGroup* page_orientation_group;
	QSpinBox* copies_edit;
	QDoubleSpinBox* page_width_edit;
	QDoubleSpinBox* page_height_edit;
	
	QComboBox* dpi_combo;
	QCheckBox* show_templates_check;
	QLabel* templates_warning_icon;
	QLabel* templates_warning_text;
	QCheckBox* show_grid_check;
	QCheckBox* overprinting_check;
	QCheckBox* world_file_check;
	QCheckBox* different_scale_check;
	QSpinBox* different_scale_edit;
	QComboBox* color_mode_combo;
	
	QComboBox* policy_combo;
	QCheckBox* center_check;
	QDoubleSpinBox* left_edit;
	QDoubleSpinBox* top_edit;
	QDoubleSpinBox* width_edit;
	QDoubleSpinBox* height_edit;
	QDoubleSpinBox* overlap_edit;
	
	QToolButton* vector_mode_button;
	QToolButton* raster_mode_button;
	QToolButton* separations_mode_button;
	
	QPushButton* preview_button;
	QPushButton* print_button;
	QPushButton* export_button;
	
	QStringList printers;
	
	PrintAreaPolicy policy;
	
	Map* map;
	MapPrinter* map_printer;
	MainWindow* main_window;
	MapView* main_view;
	MapEditorController* editor;
	PrintTool* print_tool;
	QString saved_view_state;
	bool active;
};


}  // namespace OpenOrienteering


Q_DECLARE_OPERATORS_FOR_FLAGS(OpenOrienteering::PrintWidget::TaskFlags)


#endif


#endif
