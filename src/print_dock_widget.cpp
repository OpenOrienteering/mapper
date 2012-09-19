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


#include "print_dock_widget.h"

#include <limits>

#if QT_VERSION < 0x050000
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QPrintPreviewDialog>

#include "util.h"
#include "map.h"
#include "global.h"
#include "main_window.h"
#include "map_widget.h"
#include "settings.h"

PrintWidget::PrintWidget(Map* map, MainWindow* main_window, MapView* main_view, MapEditorController* editor, QWidget* parent) : EditorDockWidgetChild(parent), map(map), main_window(main_window), main_view(main_view), editor(editor)
{
	react_to_changes = true;
	print_tool = NULL;
	
	bool params_set = map->arePrintParametersSet();
	int orientation;
	float dpi, left, top, width, height;
	bool show_templates, show_grid, center, different_scale_enabled;
	int different_scale;
	if (params_set)
	{
		map->getPrintParameters(orientation, prev_paper_size, dpi, show_templates, show_grid, center, left, top, width, height, different_scale_enabled, different_scale);
		have_prev_paper_size = true;
	}
	else
	{
		have_prev_paper_size = false;
		
		// By default, set the print area to the map bbox
		QRectF map_extent = map->calculateExtent(false, false, main_view);
		const float default_border_size = 2 * 3;
		print_width = map_extent.width() + default_border_size;
		print_height = map_extent.height() + default_border_size;
		width = print_width;
		height = print_height;
	}
	
	QLabel* device_label = new QLabel("<b>" + tr("Printer or exporter:") + "</b>");
	device_combo = new QComboBox();

    device_combo->setMaximumWidth(200);
	device_combo->addItem(tr("Export to PDF or PS"), QVariant((int)PdfExporter));
	device_combo->addItem(tr("Export to image"), QVariant((int)ImageExporter));
	device_combo->insertSeparator(device_combo->count());
	device_combo->setCurrentIndex(0);
	
	printers = QPrinterInfo::availablePrinters();
    for (int i = 0; i < printers.size(); ++i)
	{
		device_combo->addItem(printers[i].printerName(), i);
		if (printers[i].isDefault())
			device_combo->setCurrentIndex(device_combo->findData(i));
	}
	
	show_templates_check = new QCheckBox(tr("Show templates"));	// this must be created before its value is used to determine the default setting of page_orientation_combo
	if (params_set)
		show_templates_check->setChecked(show_templates);
	
	show_grid_check = new QCheckBox(tr("Show grid"));
	if (params_set)
		show_grid_check->setChecked(show_grid);
	
	QLabel* page_orientation_label = new QLabel(tr("Page orientation:"));
	page_orientation_combo = new QComboBox();
	page_orientation_combo->addItem(tr("Portrait"), QPrinter::Portrait);
	page_orientation_combo->addItem(tr("Landscape"), QPrinter::Landscape);
	if (params_set)
		page_orientation_combo->setCurrentIndex(page_orientation_combo->findData(orientation));
	else
	{
		QRectF map_extent = map->calculateExtent(false, show_templates_check->isChecked(), main_view);
		QPrinter::Orientation best_orientation = (map_extent.width() > map_extent.height()) ? QPrinter::Landscape : QPrinter::Portrait;
		page_orientation_combo->setCurrentIndex(page_orientation_combo->findData((int)best_orientation));
	}
	
	QLabel* page_format_label = new QLabel(tr("Page format:"));
	page_format_combo = new QComboBox();
	
	dpi_label = new QLabel(tr("Dots per inch (dpi):"), this);
	dpi_edit = new QLineEdit(params_set ? QString::number(dpi) : "600", this);
	dpi_edit->setValidator(new QIntValidator(1, 999999, dpi_edit));
	
	copies_label = new QLabel(tr("Copies:"), this);
	copies_edit = new QLineEdit("1", this);
	copies_edit->setValidator(new QIntValidator(1, 999999, copies_edit));
	
	QLabel* print_area_label = new QLabel("<b>" + tr("Print area") + "</b>");
	QLabel* left_label = new QLabel(tr("Left:"));
	left_edit = new QLineEdit(params_set ? QString::number(left) : "");
	left_edit->setValidator(new DoubleValidator(-999999, 999999, left_edit));
	QLabel* top_label = new QLabel(tr("Top:"));
	top_edit = new QLineEdit(params_set ? QString::number(top) : "");
	top_edit->setValidator(new DoubleValidator(-999999, 999999, top_edit));
	QLabel* width_label = new QLabel(tr("Width:"));
	width_edit = new QLineEdit(QString::number(width));
	width_edit->setValidator(new DoubleValidator(-999999, 999999, width_edit));
	QLabel* height_label = new QLabel(tr("Height:"));
	height_edit = new QLineEdit(QString::number(height));
	height_edit->setValidator(new DoubleValidator(-999999, 999999, height_edit));
	
	center_button = new QPushButton(tr("Center area on map"));
	center_button->setCheckable(true);
	center_button->setChecked(params_set ? center : true);
	
	different_scale_check = new QCheckBox("");
	different_scale_check->setChecked(params_set ? different_scale_enabled : false);
	different_scale_edit = new QLineEdit(params_set ? QString::number(different_scale) : "", this);
	different_scale_edit->setEnabled(different_scale_check->isChecked());
	different_scale_edit->setValidator(new QIntValidator(1, std::numeric_limits<int>::max(), different_scale_edit));
	
	preview_button = new QPushButton(tr("Preview..."));
	print_button = new QPushButton("");
	
	int row = 0;
	QGridLayout* layout = new QGridLayout();
	layout->addWidget(device_label, row, 0);
	layout->addWidget(device_combo, row, 1);
	++row;
	layout->addWidget(page_orientation_label, row, 0);
	layout->addWidget(page_orientation_combo, row, 1);
	++row;
	layout->addWidget(page_format_label, row, 0);
	layout->addWidget(page_format_combo, row, 1);
	++row;
	layout->addWidget(dpi_label, row, 0);
	layout->addWidget(dpi_edit, row, 1);
	++row;
	layout->addWidget(copies_label, row, 0);
	layout->addWidget(copies_edit, row, 1);
	++row;
	layout->addWidget(show_templates_check, row, 0, 1, 2);
	++row;
	layout->addWidget(show_grid_check, row, 0, 1, 2);
	++row;
	layout->addWidget(print_area_label, row, 0, 1, 2);
	++row;
	layout->addWidget(left_label, row, 0);
	layout->addWidget(left_edit, row, 1);
	++row;
	layout->addWidget(top_label, row, 0);
	layout->addWidget(top_edit, row, 1);
	++row;
	layout->addWidget(width_label, row, 0);
	layout->addWidget(width_edit, row, 1);
	++row;
	layout->addWidget(height_label, row, 0);
	layout->addWidget(height_edit, row, 1);
	++row;
	layout->addWidget(center_button, row, 0, 1, 2);
	++row;
	layout->addWidget(different_scale_check, row, 0);
	layout->addWidget(different_scale_edit, row, 1);
	++row;
	layout->setRowStretch(row, 1);
	++row;
	layout->addWidget(preview_button, row, 0);
	layout->addWidget(print_button, row, 1);
	setLayout(layout);
	
	connect(device_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(currentDeviceChanged()));
	connect(page_orientation_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(pageOrientationChanged()));
	connect(page_format_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(pageFormatChanged()));
	connect(show_templates_check, SIGNAL(clicked()), this, SLOT(showTemplatesClicked()));
	connect(show_grid_check, SIGNAL(clicked()), this, SLOT(showGridClicked()));
	connect(top_edit, SIGNAL(textEdited(QString)), this, SLOT(printAreaPositionChanged()));
	connect(left_edit, SIGNAL(textEdited(QString)), this, SLOT(printAreaPositionChanged()));
	connect(width_edit, SIGNAL(textEdited(QString)), this, SLOT(printAreaSizeChanged()));
	connect(height_edit, SIGNAL(textEdited(QString)), this, SLOT(printAreaSizeChanged()));
	connect(center_button, SIGNAL(clicked(bool)), this, SLOT(centerPrintAreaClicked()));
	connect(different_scale_check, SIGNAL(clicked(bool)), this, SLOT(differentScaleClicked(bool)));
	connect(different_scale_edit, SIGNAL(textEdited(QString)), this, SLOT(differentScaleEdited(QString)));
	connect(preview_button, SIGNAL(clicked(bool)), this, SLOT(previewClicked()));
	connect(print_button, SIGNAL(clicked(bool)), this, SLOT(printClicked()));
	
	currentDeviceChanged();
	activate();
}
PrintWidget::~PrintWidget()
{
}

QSize PrintWidget::sizeHint() const
{
	return QSize(200, 300);
}

void PrintWidget::activate()
{
	if (center_button->isChecked())
		centerPrintArea();
	
	if (!print_tool)
		print_tool = new PrintTool(editor, this);
	editor->setOverrideTool(print_tool);
}
void PrintWidget::closed()
{
	map->setPrintParameters(page_orientation_combo->itemData(page_orientation_combo->currentIndex()).toInt(),
							page_format_combo->itemData(page_format_combo->currentIndex()).toInt(),
							dpi_edit->text().toFloat(), show_templates_check->isChecked(), show_grid_check->isChecked(),
							center_button->isChecked(), getPrintAreaLeft(), getPrintAreaTop(), print_width, print_height,
							different_scale_check->isChecked(), qMax(1, different_scale_edit->text().toInt()));
	editor->setOverrideTool(NULL);
	print_tool = NULL;
}

float PrintWidget::getPrintAreaLeft()
{
	return left_edit->text().toFloat();
}
float PrintWidget::getPrintAreaTop()
{
	return top_edit->text().toFloat();
}
void PrintWidget::setPrintAreaLeft(float value)
{
	left_edit->setText(QString::number(value));
	center_button->setChecked(false);
}
void PrintWidget::setPrintAreaTop(float value)
{
	top_edit->setText(QString::number(value));
	center_button->setChecked(false);
}

void PrintWidget::getMargins(float& top, float& left, float& bottom, float& right)
{
	top = margin_top;
	left = margin_left;
	bottom = margin_bottom;
	right = margin_right;
}

QRectF PrintWidget::getEffectivePrintArea()
{
	float scale_factor = calcScaleFactor();
	return QRectF(getPrintAreaLeft() + (print_width / 2) * (1 - 1 / scale_factor),
				  getPrintAreaTop() + (print_height / 2) * (1 - 1 / scale_factor),
				  print_width / scale_factor,
				  print_height / scale_factor);
}

float PrintWidget::calcScaleFactor()
{
	if (different_scale_check->isChecked())
	{
		bool ok = false;
		int draw_scale_denominator = different_scale_edit->text().toInt(&ok);
		if (ok)
			return (map->getScaleDenominator() / (float)draw_scale_denominator);
		else
			return 1;
	}
	else
		return 1;
}

bool PrintWidget::exporterSelected()
{
	int index = device_combo->itemData(device_combo->currentIndex()).toInt();
	return index < 0;
}

void PrintWidget::printMap(QPrinter* printer)
{
	// Re-center (necessary for the print preview dialog)
	if (center_button->isChecked())
		centerPrintArea();
	
	// Print
	drawMap(printer, printer->resolution(), printer->paperRect(QPrinter::DevicePixel), false);
}
void PrintWidget::setPrinterSettings(QPrinter* printer)
{
	bool exporter = exporterSelected();
	
	printer->setDocName(QFileInfo(main_window->getCurrentFilePath()).fileName());
	printer->setFullPage(true);
	QPrinter::PaperSize paper_size = (QPrinter::PaperSize)page_format_combo->itemData(page_format_combo->currentIndex()).toInt();
	QPrinter::Orientation orientation = (QPrinter::Orientation)page_orientation_combo->itemData(page_orientation_combo->currentIndex()).toInt();
	printer->setOrientation((paper_size == QPrinter::Custom) ? QPrinter::Portrait : orientation);
	if (paper_size == QPrinter::Custom)
		printer->setPaperSize(QSizeF(print_width, print_height), QPrinter::Millimeter);
	else
		printer->setPaperSize(paper_size);
	printer->setColorMode(QPrinter::Color);
	if (!exporter)
	{
		int num_copies = qMin(1, copies_edit->text().toInt());
		#if (QT_VERSION >= 0x040700)
		printer->setCopyCount(num_copies);
		#else
		printer->setNumCopies(num_copies);
		#endif
	}
}
void PrintWidget::drawMap(QPaintDevice* paint_device, float dpi, const QRectF& page_rect, bool white_background)
{
	QRectF map_extent = map->calculateExtent(false, show_templates_check->isChecked(), main_view);
	
	QPainter device_painter;
	device_painter.begin(paint_device);
	device_painter.setRenderHint(QPainter::Antialiasing);
	device_painter.setRenderHint(QPainter::SmoothPixmapTransform);
	
	// If there is anything transparent to draw, use a temporary image which is drawn on the device printer as last step
	// because painting transparently seems to be unsupported when printing
	// TODO: This does convert all colors to RGB. Are colors printed as CMYK otherwise?
	bool have_transparency = main_view->getMapVisibility()->visible && main_view->getMapVisibility()->opacity < 1;
	for (int i = 0; i < map->getNumTemplates() && !have_transparency; ++i)
	{
		TemplateVisibility* visibility = main_view->getTemplateVisibility(map->getTemplate(i));
		have_transparency = visibility->visible && visibility->opacity < 1;
	}
	
	QPainter* painter = &device_painter;
	QImage print_buffer;
	QPainter print_buffer_painter;
	if (have_transparency)
	{
		print_buffer = QImage(paint_device->width(), paint_device->height(), QImage::Format_RGB32);
		print_buffer_painter.begin(&print_buffer);
		painter = &print_buffer_painter;
		white_background = true;
	}
	
	if (white_background)
		painter->fillRect(QRect(0, 0, paint_device->width(), paint_device->height()), Qt::white);
	
	// Need to convert mm to dots
	float scale_factor = calcScaleFactor();
	float scale = (1/25.4f * dpi) * scale_factor;
	painter->scale(scale, scale);
	QRectF print_area = getEffectivePrintArea();
	painter->translate(-print_area.left(), -print_area.top());
	
	if (show_templates_check->isChecked())
		map->drawTemplates(painter, map_extent, 0, map->getFirstFrontTemplate() - 1, false, QRect(0, 0, paint_device->width(), paint_device->height()), NULL, main_view);
	if (main_view->getMapVisibility()->visible)
	{
		if (main_view->getMapVisibility()->opacity == 1)
			map->draw(painter, map_extent, false, scale, false);
		else
		{
			// Draw map into a temporary buffer first which is printed with the map's opacity later.
			// This prevents artifacts with overlapping objects.
			QImage map_buffer(paint_device->width(), paint_device->height(), QImage::Format_ARGB32_Premultiplied);
			QPainter buffer_painter;
			buffer_painter.begin(&map_buffer);
			
			// Clear buffer
			QPainter::CompositionMode mode = buffer_painter.compositionMode();
			buffer_painter.setCompositionMode(QPainter::CompositionMode_Clear);
			buffer_painter.fillRect(map_buffer.rect(), Qt::transparent);
			buffer_painter.setCompositionMode(mode);
			
			// Draw map with full opacity
			buffer_painter.setTransform(painter->transform());
			map->draw(&buffer_painter, map_extent, false, scale, false);
			
			buffer_painter.end();
			
			// Print buffer with map opacity
			painter->save();
			painter->resetTransform();
			painter->setOpacity(main_view->getMapVisibility()->opacity);
			painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
			painter->drawImage(0, 0, map_buffer);
			painter->restore();
		}
	}
	if (show_grid_check->isChecked())
		map->drawGrid(painter, print_area);
	if (show_templates_check->isChecked())
		map->drawTemplates(painter, map_extent, map->getFirstFrontTemplate(), map->getNumTemplates() - 1, false, QRect(0, 0, paint_device->width(), paint_device->height()), NULL, main_view);
	
	// If a temporary buffer has been used, paint it on the device printer
	if (print_buffer_painter.isActive())
	{
		print_buffer_painter.end();
		painter = &device_painter;
		
		device_painter.save();
		device_painter.resetTransform();
		device_painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
		device_painter.drawImage(0, 0, print_buffer);
		device_painter.restore();
	}
	
	painter->end();
}

void PrintWidget::currentDeviceChanged()
{
	if (device_combo->currentIndex() < 0)
		return;
	react_to_changes = false;
	
	int index = device_combo->itemData(device_combo->currentIndex()).toInt();
	assert(index >= -2 && index < (int)printers.size());
	bool exporter = index < 0;
	bool image_exporter = index == (int)ImageExporter;
	
	// First hide everything, then show selected widgets again. This prevents the widget from resizing.
	dpi_label->hide();
	dpi_edit->hide();
	copies_label->hide();
	copies_edit->hide();
	dpi_label->setVisible(image_exporter);
	dpi_edit->setVisible(image_exporter);
	copies_label->setVisible(!exporter);
	copies_edit->setVisible(!exporter);
	
	preview_button->setEnabled(!exporter);
	different_scale_check->setText(exporter ? tr("Export in different scale 1 :") : tr("Print in different scale 1 :"));
	print_button->setText(exporter ? tr("Export") : tr("Print"));
	
	
	QString prev_paper_size_name = "";
	if (page_format_combo->count() > 0)
		prev_paper_size_name = page_format_combo->itemText(page_format_combo->currentIndex());
	page_format_combo->clear();
	
	QList<QPrinter::PaperSize> size_list;
	if (!exporter)
		size_list = printers[index].supportedPaperSizes();
	
	bool have_custom_size = false;
	if (size_list.isEmpty())
	{
		addPaperSize(QPrinter::A4);
		addPaperSize(QPrinter::B5);
		addPaperSize(QPrinter::Letter);
		addPaperSize(QPrinter::Legal);
		addPaperSize(QPrinter::Executive);
		addPaperSize(QPrinter::A0);
		addPaperSize(QPrinter::A1);
		addPaperSize(QPrinter::A3);
		addPaperSize(QPrinter::A5);
		addPaperSize(QPrinter::A6);
		addPaperSize(QPrinter::A7);
		addPaperSize(QPrinter::A8);
		addPaperSize(QPrinter::A9);
		addPaperSize(QPrinter::B0);
		addPaperSize(QPrinter::B1);
		addPaperSize(QPrinter::B10);
		addPaperSize(QPrinter::B2);
		addPaperSize(QPrinter::B3);
		addPaperSize(QPrinter::B4);
		addPaperSize(QPrinter::B6);
		addPaperSize(QPrinter::B7);
		addPaperSize(QPrinter::B8);
		addPaperSize(QPrinter::B9);
		addPaperSize(QPrinter::C5E);
		addPaperSize(QPrinter::Comm10E);
		addPaperSize(QPrinter::DLE);
		addPaperSize(QPrinter::Folio);
		addPaperSize(QPrinter::Ledger);
		addPaperSize(QPrinter::Tabloid);
		have_custom_size = true;
	}
	else
	{
		foreach (QPrinter::PaperSize size, size_list)
		{
			if (size == QPrinter::Custom)
				have_custom_size = true; // add it once after all other entires
			else
				addPaperSize(size);
		}
	}
	
	if (have_custom_size)
		addPaperSize(QPrinter::Custom);
	
	width_edit->setEnabled(have_custom_size);
	height_edit->setEnabled(have_custom_size);
	
	if (prev_paper_size_name != "")
	{
		int prev_paper_size_index = page_format_combo->findText(prev_paper_size_name);
		if (prev_paper_size_index >= 0)
			page_format_combo->setCurrentIndex(prev_paper_size_index);
		else
			page_format_combo->setCurrentIndex(0);
	}
	else if (have_prev_paper_size)
	{
		have_prev_paper_size = false;
		int index = page_format_combo->findData(prev_paper_size);
		if (index >= 0)
			page_format_combo->setCurrentIndex(index);
		else
			page_format_combo->setCurrentIndex(0);
	}
	else
		page_format_combo->setCurrentIndex(0);
	
	updatePrintAreaSize();
	if (print_tool)
		print_tool->updatePrintArea();
	react_to_changes = true;
}
void PrintWidget::pageOrientationChanged()
{
	if (!react_to_changes) return;
	updatePrintAreaSize();
	if (center_button->isChecked())
		centerPrintArea();
	else
		print_tool->updatePrintArea();
}
void PrintWidget::pageFormatChanged()
{
	if (!react_to_changes) return;
	updatePrintAreaSize();
	if (center_button->isChecked())
		centerPrintArea();
	else
		print_tool->updatePrintArea();
}
void PrintWidget::showTemplatesClicked()
{
	if (center_button->isChecked())
		centerPrintArea();
}
void PrintWidget::showGridClicked()
{
	// nothing yet
}
void PrintWidget::printAreaPositionChanged()
{
	center_button->setChecked(false);
	print_tool->updatePrintArea();
}
void PrintWidget::centerPrintAreaClicked()
{
	if (center_button->isChecked())
		centerPrintArea();
}
void PrintWidget::centerPrintArea()
{
	center_button->setChecked(true);
	
	QRectF map_extent = map->calculateExtent(false, show_templates_check->isChecked(), main_view);
	if (!map_extent.isValid())
	{
		left_edit->setText("0");
		top_edit->setText("0");
		return;
	}
	
	float left = map_extent.left() + 0.5f * map_extent.width() - 0.5f * print_width;
	float top = map_extent.top() + 0.5f * map_extent.height() - 0.5f * print_height;
	left_edit->setText(QString::number(left));
	top_edit->setText(QString::number(top));
	
	if (print_tool)
		print_tool->updatePrintArea();
}
void PrintWidget::printAreaSizeChanged()
{
	if (!react_to_changes) return;
	
	page_format_combo->setCurrentIndex(page_format_combo->findData((int)QPrinter::Custom));
	updatePrintAreaSize();
	
	if (center_button->isChecked())
		centerPrintArea();
	else
		print_tool->updatePrintArea();
}
void PrintWidget::updatePrintAreaSize()
{
	int index = page_format_combo->currentIndex();
	if (index < 0)
	{
		assert(false);
		return;
	}
	
	QPrinter::PaperSize size = (QPrinter::PaperSize)page_format_combo->itemData(index).toInt();
	
	if (size == QPrinter::Custom)
	{
		print_width = width_edit->text().toFloat();
		print_height = height_edit->text().toFloat();
	}
	else
	{
		QPrinter printer(QPrinter::HighResolution);
		printer.setPaperSize(size);
		printer.setOrientation((QPrinter::Orientation)page_orientation_combo->itemData(page_orientation_combo->currentIndex()).toInt());
		
		QRectF paper_rect = printer.paperRect(QPrinter::Millimeter);
		QRectF page_rect = printer.pageRect(QPrinter::Millimeter);
		
		print_width = printer.paperRect(QPrinter::Millimeter).width();
		width_edit->setText(QString::number(print_width));
		print_height = printer.paperRect(QPrinter::Millimeter).height();
		height_edit->setText(QString::number(print_height));
		
		margin_top = page_rect.top() - paper_rect.top();
		margin_left = page_rect.left() - paper_rect.left();
		margin_right = paper_rect.right() - page_rect.right();
		margin_bottom = paper_rect.bottom() - page_rect.bottom();
	}
}

void PrintWidget::differentScaleClicked(bool checked)
{
	different_scale_edit->setEnabled(checked);
	if (print_tool)
		print_tool->updatePrintArea();
}
void PrintWidget::differentScaleEdited(QString text)
{
	if (print_tool)
		print_tool->updatePrintArea();
}

void PrintWidget::previewClicked()
{
	if (checkForEmptyMap())
		return;
	
	int index = device_combo->itemData(device_combo->currentIndex()).toInt();
	QPrinter printer(printers[index], QPrinter::HighResolution);
	setPrinterSettings(&printer);
	
	QPrintPreviewDialog preview(&printer, this);
	connect(&preview, SIGNAL(paintRequested(QPrinter*)), this, SLOT(printMap(QPrinter*)));
	preview.exec();
	
	page_orientation_combo->setCurrentIndex(page_orientation_combo->findData((int)printer.orientation()));
	page_format_combo->setCurrentIndex(page_format_combo->findData((int)printer.paperSize()));
}
void PrintWidget::printClicked()
{
	if (checkForEmptyMap())
		return;
	
	if (map->isAreaHatchingEnabled() || map->isBaselineViewEnabled())
	{
		if (QMessageBox::question(this, tr("Warning"), tr("A non-standard view mode is activated. Are you sure to print / export the map like this?"), QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Cancel)
			return;
	}
	
	int index = device_combo->itemData(device_combo->currentIndex()).toInt();
	
	QString path;
	if (index < 0)
	{
		path = QFileDialog::getSaveFileName(this, tr("Export map ..."), QString(), tr("All files (*.*)"));
		if (path.isEmpty())
			return;
	}
	
	if (index == (int)ImageExporter)
	{
		if (!path.endsWith(".bmp", Qt::CaseInsensitive) && !path.endsWith(".png", Qt::CaseInsensitive) && !path.endsWith(".jpg", Qt::CaseInsensitive) &&
			!path.endsWith(".tiff", Qt::CaseInsensitive) && !path.endsWith(".jpeg", Qt::CaseInsensitive))
			path.append(".png");
		
		float dpi = dpi_edit->text().toFloat();
		float pixel_per_mm = dpi / 25.4f;
		
		QImage image(print_width * pixel_per_mm, print_height * pixel_per_mm, QImage::Format_ARGB32_Premultiplied);
		drawMap(&image, dpi, image.rect(), true);
		if (!image.save(path))
		{
			QMessageBox::warning(this, tr("Error"), tr("Failed to save the image. Does the path exist? Do you have sufficient rights?"));
			return;
		}
		
		main_window->statusBar()->showMessage(tr("Exported successfully to %1").arg(path), 4000);
		parentWidget()->close();
		return;
	}
	
	QPrinter* printer;
	if (index == (int)PdfExporter)
	{
		printer = new QPrinter(QPrinter::HighResolution);
		
		if (path.endsWith(".ps", Qt::CaseInsensitive))
;//			printer->setOutputFormat(QPrinter::PostScriptFormat);
		else
		{
			printer->setOutputFormat(QPrinter::PdfFormat);
			if (!path.endsWith(".pdf", Qt::CaseInsensitive))
				path.append(".pdf");
		}
		printer->setOutputFileName(path);
		printer->setCreator(APP_NAME);
	}
	else
		printer = new QPrinter(printers[index], QPrinter::HighResolution);
	
	// Print the map
	setPrinterSettings(printer);
	printMap(printer);
	delete printer;
	
	if (index < 0)
		main_window->statusBar()->showMessage(tr("Exported successfully to %1").arg(path), 4000);
	else
		main_window->statusBar()->showMessage(tr("Successfully created print job"), 4000);
	
	parentWidget()->close();
}

void PrintWidget::addPaperSize(QPrinter::PaperSize size)
{
	QString name;
	switch (size)
	{
	case QPrinter::A4:			name = "A4";			break;
	case QPrinter::B5:			name = "B5";			break;
	case QPrinter::Letter:		name = tr("Letter");	break;
	case QPrinter::Legal:		name = tr("Legal");		break;
	case QPrinter::Executive:	name = tr("Executive");	break;
	case QPrinter::A0:			name = "A0";			break;
	case QPrinter::A1:			name = "A1";			break;
	case QPrinter::A2:			name = "A2";			break;
	case QPrinter::A3:			name = "A3";			break;
	case QPrinter::A5:			name = "A5";			break;
	case QPrinter::A6:			name = "A6";			break;
	case QPrinter::A7:			name = "A7";			break;
	case QPrinter::A8:			name = "A8";			break;
	case QPrinter::A9:			name = "A9";			break;
	case QPrinter::B0:			name = "B0";			break;
	case QPrinter::B1:			name = "B1";			break;
	case QPrinter::B10:			name = "B10";			break;
	case QPrinter::B2:			name = "B2";			break;
	case QPrinter::B3:			name = "B3";			break;
	case QPrinter::B4:			name = "B4";			break;
	case QPrinter::B6:			name = "B6";			break;
	case QPrinter::B7:			name = "B7";			break;
	case QPrinter::B8:			name = "B8";			break;
	case QPrinter::B9:			name = "B9";			break;
	case QPrinter::C5E:			name = tr("C5E");		break;
	case QPrinter::Comm10E:		name = tr("Comm10E");	break;
	case QPrinter::DLE:			name = tr("DLE");		break;
	case QPrinter::Folio:		name = tr("Folio");		break;
	case QPrinter::Ledger:		name = tr("Ledger");	break;
	case QPrinter::Tabloid:		name = tr("Tabloid");	break;
	case QPrinter::Custom:		name = tr("Custom");	break;
	default:					name = tr("Unknown");	break;
	}
	
	page_format_combo->addItem(name, (int)size);
}
bool PrintWidget::checkForEmptyMap()
{
	if ((map->getNumObjects() == 0 || main_view->getMapVisibility()->visible == false || main_view->getMapVisibility()->opacity == 0) &&
		(!show_templates_check->isChecked() || map->getNumTemplates() == 0) &&
		!show_grid_check->isChecked())
	{
		QMessageBox::warning(this, tr("Error"), tr("The print area is empty, so there is nothing to print!"));
		return true;
	}
	return false;
}

// ### PrintTool ###

QCursor* PrintTool::cursor = NULL;

PrintTool::PrintTool(MapEditorController* editor, PrintWidget* widget) : MapEditorTool(editor, Other, NULL), widget(widget)
{
	dragging = false;
	
	if (!cursor)
		cursor = new QCursor(Qt::SizeAllCursor);
}

void PrintTool::init()
{
	setStatusBarText(tr("<b>Drag</b> to move the print area"));
	updatePrintArea();
}

bool PrintTool::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (event->button() != Qt::LeftButton)
		return false;
	
	dragging = true;
	click_pos_map = map_coord;
	return true;
}
bool PrintTool::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (!(event->buttons() & Qt::LeftButton) || !dragging)
		return false;
	
	updateDragging(map_coord);
	return true;
}
bool PrintTool::mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (event->button() != Qt::LeftButton)
		return false;
	
	updateDragging(map_coord);
	dragging = false;
	return true;
}

void PrintTool::draw(QPainter* painter, MapWidget* widget)
{
	QRectF effective_print_area = this->widget->getEffectivePrintArea();
	QRect outer_rect = widget->mapToViewport(effective_print_area).toRect();
	QRect view_rect = QRect(0, 0, widget->width(), widget->height());
	
	painter->setPen(Qt::NoPen);
	painter->setBrush(QColor(0, 0, 0, 160));
	drawBetweenRects(painter, view_rect, outer_rect);
	
	if (!this->widget->exporterSelected())
	{
		float margin_top, margin_left, margin_bottom, margin_right;
		this->widget->getMargins(margin_top, margin_left, margin_bottom, margin_right);
		if (margin_top > 0 || margin_left > 0 || margin_bottom > 0 || margin_right > 0)
		{
			QRect inner_rect = widget->mapToViewport(effective_print_area.adjusted(margin_left, margin_top, -margin_right, -margin_bottom)).toRect();
			
			painter->setBrush(QColor(0, 0, 0, 64));
			if (effective_print_area.width() < margin_left + margin_right || effective_print_area.height() < margin_top + margin_bottom)
				painter->drawRect(outer_rect);
			else
				drawBetweenRects(painter, outer_rect, inner_rect);
		}
	}
	
	/*painter->setBrush(QColor(255, 255, 0, 128));
	painter->drawRect(QRect(outer_rect.topLeft(), outer_rect.bottomRight() - QPoint(1, 1)));
	painter->setBrush(QColor(0, 255, 0, 128));
	painter->drawRect(QRect(inner_rect.topLeft(), inner_rect.bottomRight() - QPoint(1, 1)));*/
	
	/*QRect rect = widget->mapToViewport(this->widget->getEffectivePrintArea()).toRect();

	painter->setPen(active_color);
	painter->drawRect(QRect(rect.topLeft(), rect.bottomRight() - QPoint(1, 1)));
	painter->setPen(qRgb(255, 255, 255));
	painter->drawRect(QRect(rect.topLeft() + QPoint(1, 1), rect.bottomRight() - QPoint(2, 2)));*/
}

void PrintTool::drawBetweenRects(QPainter* painter, const QRect &outer, const QRect &inner) const {
	if (outer.isEmpty())
		return;
	QRect clipped_inner = outer.intersected(inner);
	if (clipped_inner.isEmpty())
	{
		painter->drawRect(outer);
	}
	else
	{
		if (outer.left() < clipped_inner.left())
			painter->drawRect(QRect(outer.left(), outer.top(), clipped_inner.left() - outer.left(), outer.height()));
		if (outer.right() > clipped_inner.right())
			painter->drawRect(QRect(clipped_inner.left() + clipped_inner.width(), outer.top(), outer.right() - clipped_inner.right(), outer.height()));
		if (outer.top() < clipped_inner.top())
			painter->drawRect(QRect(clipped_inner.left(), outer.top(), clipped_inner.width(), clipped_inner.top() - outer.top()));
		if (outer.bottom() > clipped_inner.bottom())
			painter->drawRect(QRect(clipped_inner.left(), clipped_inner.bottom(), clipped_inner.width(), outer.top() + outer.height() - clipped_inner.bottom()));
	}
}

void PrintTool::updatePrintArea()
{
	editor->getMap()->setDrawingBoundingBox(QRectF(-1000000, -1000000, 2000000, 2000000), 0);
	
	//editor->getMap()->setDrawingBoundingBox(widget->getEffectivePrintArea(), 1);
}
void PrintTool::updateDragging(MapCoordF mouse_pos_map)
{
	widget->setPrintAreaLeft(widget->getPrintAreaLeft() + mouse_pos_map.getX() - click_pos_map.getX());
	widget->setPrintAreaTop(widget->getPrintAreaTop() + mouse_pos_map.getY() - click_pos_map.getY());
	click_pos_map = mouse_pos_map;
	
	updatePrintArea();
}
