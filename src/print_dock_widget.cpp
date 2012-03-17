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

#include <QtGui>

#include "util.h"
#include "map.h"
#include "global.h"
#include "main_window.h"
#include "map_widget.h"

PrintWidget::PrintWidget(Map* map, MainWindow* main_window, MapView* main_view, MapEditorController* editor, QWidget* parent) : EditorDockWidgetChild(parent), map(map), main_window(main_window), main_view(main_view), editor(editor)
{
	react_to_changes = true;
	print_tool = NULL;
	
	bool params_set = map->arePrintParametersSet();
	int orientation;
	float dpi, left, top, width, height;
	bool show_templates, center;
	if (params_set)
	{
		map->getPrintParameters(orientation, prev_paper_size, dpi, show_templates, center, left, top, width, height);
		have_prev_paper_size = true;
	}
	else
		have_prev_paper_size = false;
	
	QLabel* device_label = new QLabel("<b>" + tr("Printer or exporter:") + "</b>");
	device_combo = new QComboBox();
	
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
	
	QLabel* page_orientation_label = new QLabel(tr("Page orientation:"));
	page_orientation_combo = new QComboBox();
	page_orientation_combo->addItem(tr("Portrait"), QPrinter::Portrait);
	page_orientation_combo->addItem(tr("Landscape"), QPrinter::Landscape);
	if (params_set)
		page_orientation_combo->setCurrentIndex(page_orientation_combo->findData(orientation));
	else
	{
		QRectF map_extent = map->calculateExtent(show_templates_check->isChecked(), main_view);
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
	width_edit = new QLineEdit(params_set ? QString::number(width) : "");
	width_edit->setValidator(new DoubleValidator(-999999, 999999, width_edit));
	QLabel* height_label = new QLabel(tr("Height:"));
	height_edit = new QLineEdit(params_set ? QString::number(height) : "");
	height_edit->setValidator(new DoubleValidator(-999999, 999999, height_edit));
	
	center_button = new QPushButton(tr("Center area on map"));
	center_button->setCheckable(true);
	center_button->setChecked(params_set ? center : true);
	preview_button = new QPushButton(tr("Preview..."));
	print_button = new QPushButton("");
	
	QGridLayout* layout = new QGridLayout();
	layout->addWidget(device_label, 0, 0);
	layout->addWidget(device_combo, 0, 1);
	layout->addWidget(page_orientation_label, 1, 0);
	layout->addWidget(page_orientation_combo, 1, 1);
	layout->addWidget(page_format_label, 2, 0);
	layout->addWidget(page_format_combo, 2, 1);
	layout->addWidget(dpi_label, 3, 0);
	layout->addWidget(dpi_edit, 3, 1);
	layout->addWidget(copies_label, 4, 0);
	layout->addWidget(copies_edit, 4, 1);
	layout->addWidget(show_templates_check, 5, 0, 1, 2);
	layout->addWidget(print_area_label, 6, 0, 1, 2);
	layout->addWidget(left_label, 7, 0);
	layout->addWidget(left_edit, 7, 1);
	layout->addWidget(top_label, 8, 0);
	layout->addWidget(top_edit, 8, 1);
	layout->addWidget(width_label, 9, 0);
	layout->addWidget(width_edit, 9, 1);
	layout->addWidget(height_label, 10, 0);
	layout->addWidget(height_edit, 10, 1);
	layout->addWidget(center_button, 11, 0, 1, 2);
	layout->setRowStretch(12, 1);
	layout->addWidget(preview_button, 13, 0);
	layout->addWidget(print_button, 13, 1);
	setLayout(layout);
	
	connect(device_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(currentDeviceChanged()));
	connect(page_orientation_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(pageOrientationChanged()));
	connect(page_format_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(pageFormatChanged()));
	connect(show_templates_check, SIGNAL(clicked()), this, SLOT(showTemplatesClicked()));
	connect(top_edit, SIGNAL(textEdited(QString)), this, SLOT(printAreaPositionChanged()));
	connect(left_edit, SIGNAL(textEdited(QString)), this, SLOT(printAreaPositionChanged()));
	connect(width_edit, SIGNAL(textEdited(QString)), this, SLOT(printAreaSizeChanged()));
	connect(height_edit, SIGNAL(textEdited(QString)), this, SLOT(printAreaSizeChanged()));
	connect(center_button, SIGNAL(clicked(bool)), this, SLOT(centerPrintAreaClicked()));
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
							dpi_edit->text().toFloat(), show_templates_check->isChecked(), center_button->isChecked(), getPrintAreaLeft(), getPrintAreaTop(), print_width, print_height);
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

QRectF PrintWidget::getPrintArea()
{
	return QRectF(getPrintAreaLeft(), getPrintAreaTop(), print_width, print_height);
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
	int index = device_combo->itemData(device_combo->currentIndex()).toInt();
	bool exporter = index < 0;
	
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
	QRectF map_extent = map->calculateExtent(show_templates_check->isChecked(), main_view);
	
	QPainter painter;
	painter.begin(paint_device);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::SmoothPixmapTransform);
	
	if (white_background)
		painter.fillRect(QRect(0, 0, paint_device->width(), paint_device->height()), Qt::white);
	
	// Need to convert mm to dots
	float scale = 1/25.4f * dpi;
	
	painter.scale(scale, scale);
	painter.translate(-getPrintAreaLeft(), -getPrintAreaTop());
	
	if (show_templates_check->isChecked())
		map->drawTemplates(&painter, map_extent, 0, map->getFirstFrontTemplate() - 1, false, QRect(0, 0, paint_device->width(), paint_device->height()), NULL, main_view);
	map->draw(&painter, map_extent, false, scale, false);
	if (show_templates_check->isChecked())
		map->drawTemplates(&painter, map_extent, map->getFirstFrontTemplate(), map->getNumTemplates() - 1, false, QRect(0, 0, paint_device->width(), paint_device->height()), NULL, main_view);
	
	painter.end();
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
	
	dpi_label->setVisible(image_exporter);
	dpi_edit->setVisible(image_exporter);
	copies_label->setVisible(!exporter);
	copies_edit->setVisible(!exporter);
	
	preview_button->setEnabled(!exporter);
	print_button->setText(exporter ? tr("Export") : tr("Print"));
	
	
	QString prev_paper_size_name = "";
	if (page_format_combo->count() > 0)
		prev_paper_size_name = page_format_combo->itemText(page_format_combo->currentIndex());
	page_format_combo->clear();
	
	QList<QPrinter::PaperSize> size_list;
	// NOTE: Using QPrinterInfo::supportedPaperSizes() leads to a crash for me on Windows as soon as the returned QList is destroyed on Qt 4.8. Maybe a QT bug? Disabling it for now.
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
		addPaperSize(QPrinter::Custom);
		have_custom_size = true;
	}
	else
	{
		foreach (QPrinter::PaperSize size, size_list)
		{
			addPaperSize(size);
			if (size == QPrinter::Custom)
				have_custom_size = true;
		}
	}
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
	
	QRectF map_extent = map->calculateExtent(show_templates_check->isChecked(), main_view);
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
		
		print_width = printer.paperRect(QPrinter::Millimeter).width();
		width_edit->setText(QString::number(print_width));
		print_height = printer.paperRect(QPrinter::Millimeter).height();
		height_edit->setText(QString::number(print_height));
	}
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
			printer->setOutputFormat(QPrinter::PostScriptFormat);
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
	if (map->getNumObjects() == 0 && (!show_templates_check->isChecked() || map->getNumTemplates() == 0))
	{
		QMessageBox::warning(this, tr("Error"), tr("The map is empty, there is nothing to print!"));
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
	QRect rect = widget->mapToViewport(this->widget->getPrintArea()).toRect();
	painter->setPen(active_color);
	painter->drawRect(QRect(rect.topLeft(), rect.bottomRight() - QPoint(1, 1)));
	painter->setPen(qRgb(255, 255, 255));
	painter->drawRect(QRect(rect.topLeft() + QPoint(1, 1), rect.bottomRight() - QPoint(2, 2)));
}

void PrintTool::updatePrintArea()
{
	editor->getMap()->setDrawingBoundingBox(widget->getPrintArea(), 1);
}
void PrintTool::updateDragging(MapCoordF mouse_pos_map)
{
	widget->setPrintAreaLeft(widget->getPrintAreaLeft() + mouse_pos_map.getX() - click_pos_map.getX());
	widget->setPrintAreaTop(widget->getPrintAreaTop() + mouse_pos_map.getY() - click_pos_map.getY());
	click_pos_map = mouse_pos_map;
	
	updatePrintArea();
}

#include "print_dock_widget.moc"
