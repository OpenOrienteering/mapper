/*
 *    Copyright 2012 Thomas Schöps, Kai Pastor
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


#include "print_widget.h"

#include <limits>

#if QT_VERSION < 0x050000
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QPrintPreviewDialog>

#include "main_window.h"
#include "../core/map_printer.h"
#include "../map.h"
#include "../map_editor.h"
#include "../map_widget.h"
#include "../settings.h"
#include "../util.h"
#include "../util_gui.h"
#include "../util/scoped_signals_blocker.h"
#include "print_tool.h"


const QString toString(QPrinter::PaperSize size);


PrintWidget::PrintWidget(Map* map, MainWindow* main_window, MapView* main_view, MapEditorController* editor, QWidget* parent)
: QWidget(parent), 
  map(map), 
  map_printer(new MapPrinter(*map)),
  main_window(main_window), 
  main_view(main_view), 
  editor(editor),
  print_tool(NULL)
{
	layout = new QFormLayout();
	
	target_combo = new QComboBox();
	target_combo->setMinimumWidth(1); // Not zero, but not as long as the items
	layout->addRow(Util::Headline::create(tr("Printer or exporter:")), target_combo);
	
	paper_size_combo = new QComboBox();
	layout->addRow(tr("Page format:"), paper_size_combo);
	
	page_orientation_combo = new QComboBox();
	page_orientation_combo->addItem(tr("Portrait"), QPrinter::Portrait);
	page_orientation_combo->addItem(tr("Landscape"), QPrinter::Landscape);
	layout->addRow(tr("Page orientation:"), page_orientation_combo);
	
	copies_edit = Util::SpinBox::create(1, 99999);
	layout->addRow(tr("Copies:"), copies_edit);
	
	layout->addItem(Util::SpacerItem::create(this));
	
	policy_combo = new QComboBox();
	policy_combo->addItem(tr("Single page"), SinglePage);
	policy_combo->addItem(tr("Custom area"), CustomArea);
	layout->addRow(Util::Headline::create(tr("Map area:")), policy_combo); // or print/export area
	
	center_check = new QCheckBox(tr("Center page on map"));
	center_check->setCheckable(true);
	layout->addRow(center_check);
	
	left_edit = Util::SpinBox::create(2, -999999.9, 999999.9, tr("mm"), 1.0);
	layout->addRow(tr("Left:"), left_edit);
	
	top_edit = Util::SpinBox::create(2, -999999.9, 999999.9, tr("mm"), 1.0);
	layout->addRow(tr("Top:"), top_edit);
	
	width_edit = Util::SpinBox::create(2, -999999.9, 999999.9, tr("mm"), 1.0);
	layout->addRow(tr("Width:"), width_edit);
	
	height_edit = Util::SpinBox::create(2, -999999.9, 999999.9, tr("mm"), 1.0);
	layout->addRow(tr("Height:"), height_edit);
	
	layout->addItem(Util::SpacerItem::create(this));
	
	layout->addRow(Util::Headline::create(tr("Options")));
	
	dpi_combo = new QComboBox();
	dpi_combo->setEditable(true);
	dpi_combo->setValidator(new QRegExpValidator(QRegExp("^[1-9]\\d{1,4} dpi$|^[1-9]\\d{1,4}$"), dpi_combo));
	// TODO: Implement spinbox-style " dpi" suffix
	layout->addRow(tr("Resolution:"), dpi_combo);
	
	different_scale_check = new QCheckBox(tr("Alternative scale:"));
	different_scale_edit = Util::SpinBox::create(1, std::numeric_limits<int>::max(), "", 500);
	different_scale_edit->setPrefix("1 : ");
	different_scale_edit->setEnabled(false);
	int different_scale_height = qMax(
	  different_scale_edit->minimumSizeHint().height(),
	  different_scale_check->minimumSizeHint().height() );
	different_scale_check->setMinimumHeight(different_scale_height);
	different_scale_edit->setMinimumHeight(different_scale_height);
	layout->addRow(different_scale_check, different_scale_edit);
	
	show_templates_check = new QCheckBox(tr("Show templates"));	// this must be created before its value is used to determine the default setting of page_orientation_combo
	layout->addRow(show_templates_check);
	
	show_grid_check = new QCheckBox(tr("Show grid"));
	layout->addRow(show_grid_check);
	
	overprinting_check = new QCheckBox(tr("Simulate overprinting"));
	layout->addRow(overprinting_check);
	
	layout->addItem(Util::SpacerItem::create(this));
	
	QDialogButtonBox* button_box = new QDialogButtonBox();
	preview_button = new QPushButton(tr("Preview..."));
	button_box->addButton(preview_button, QDialogButtonBox::ActionRole);
	print_button = new QPushButton(tr("Print"));
	button_box->addButton(print_button, QDialogButtonBox::AcceptRole);
	layout->addRow(button_box);
	
	setLayout(layout);
	
	connect(target_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(targetChanged(int)));
	connect(paper_size_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(paperSizeChanged(int)));
	connect(page_orientation_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(pageOrientationChanged(int)));
	
	connect(policy_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(applyPrintAreaPolicy()));
	connect(center_check, SIGNAL(clicked(bool)), this, SLOT(applyPrintAreaPolicy()));
	connect(top_edit, SIGNAL(valueChanged(double)), this, SLOT(printAreaMoved()));
	connect(left_edit, SIGNAL(valueChanged(double)), this, SLOT(printAreaMoved()));
	connect(width_edit, SIGNAL(valueChanged(double)), this, SLOT(printAreaResized()));
	connect(height_edit, SIGNAL(valueChanged(double)), this, SLOT(printAreaResized()));
	
	connect(dpi_combo->lineEdit(), SIGNAL(editingFinished()), this, SLOT(resolutionEdited()));
	connect(different_scale_check, SIGNAL(clicked(bool)), this, SLOT(differentScaleClicked(bool)));
	connect(different_scale_edit, SIGNAL(valueChanged(int)), this, SLOT(differentScaleEdited(int)));
	connect(show_templates_check, SIGNAL(clicked(bool)), this, SLOT(showTemplatesClicked(bool)));
	connect(show_grid_check, SIGNAL(clicked(bool)), this, SLOT(showGridClicked(bool)));
	connect(overprinting_check, SIGNAL(clicked(bool)), this, SLOT(overprintingClicked(bool)));
	
	connect(preview_button, SIGNAL(clicked(bool)), this, SLOT(previewClicked()));
	connect(print_button, SIGNAL(clicked(bool)), this, SLOT(printClicked()));
	
	setPageFormat(map_printer->getPageFormat());
	connect(map_printer, SIGNAL(pageFormatChanged(const MapPrinterPageFormat&)),
	        this, SLOT(setPageFormat(const MapPrinterPageFormat&)));
	
	setOptions(map_printer->getOptions());
	connect(map_printer, SIGNAL(optionsChanged(const MapPrinterOptions&)), this, SLOT(setOptions(const MapPrinterOptions&)));
	
	setPrintArea(map_printer->getPrintArea());
	connect(map_printer, SIGNAL(printAreaChanged(QRectF)), this, SLOT(setPrintArea(QRectF)));
	
// 	setTarget(exporter->getSettings().target);
	connect(map_printer, SIGNAL(targetChanged(const QPrinterInfo*)), this, SLOT(setTarget(const QPrinterInfo*)));
	
	connect(this, SIGNAL(finished(int)), map_printer, SLOT(saveParameters()));
}

PrintWidget::~PrintWidget()
{
	delete map_printer;
}

QSize PrintWidget::sizeHint() const
{
	return QSize(200, 300);
}

// slot
void PrintWidget::setActive(bool state)
{
	if (state)
	{
		// Printers may have been added or removed.
		updateTargets();
		
		// The map may have been modified.
		applyPrintAreaPolicy();
		
		if (!print_tool)
			print_tool = new PrintTool(editor, map_printer);
		editor->setOverrideTool(print_tool);
	}
	else
	{
		editor->setOverrideTool(NULL);
		print_tool = NULL;
	}
}



void PrintWidget::updateTargets()
{
	QVariant current_target = target_combo->itemData(target_combo->currentIndex());
	const QPrinterInfo* saved_printer = map_printer->getTarget();
	const QString saved_printer_name = (saved_printer == NULL) ? "" : saved_printer->printerName();
	int saved_target_index = -1;
	int default_printer_index = -1;
	{
		ScopedSignalsBlocker block(target_combo);
		target_combo->clear();
		
		// Exporters
#if QT_VERSION < 0x050000
		target_combo->addItem(tr("Export to PDF or PS"), QVariant((int)PdfExporter));
#else
		target_combo->addItem(tr("Export to PDF"), QVariant((int)PdfExporter));
#endif
		target_combo->addItem(tr("Export to image"), QVariant((int)ImageExporter));
		target_combo->insertSeparator(target_combo->count());
		target_combo->setCurrentIndex(0);
		
		// Printers
		printers = QPrinterInfo::availablePrinters();
		for (int i = 0; i < printers.size(); ++i)
		{
			if (printers[i].printerName() == saved_printer_name)
				saved_target_index = target_combo->count();
			if (printers[i].isDefault())
				default_printer_index = target_combo->count();
			target_combo->addItem(printers[i].printerName(), i);
		}
		
		// Restore selected target if possible and exit on success
		if (current_target.isValid())
		{
			int index = target_combo->findData(current_target);
			if (index >= 0)
			{
				target_combo->setCurrentIndex(index);
				return;
			}
		}
		
		if (saved_target_index >= 0)
			// Restore saved target if possible
			target_combo->setCurrentIndex(saved_target_index);
		else if (default_printer_index >= 0)
			// Set default printer as current target
			target_combo->setCurrentIndex(default_printer_index);
		
		// Explicitly invoke signal handler
		targetChanged(target_combo->currentIndex());
	}
}

// slot
void PrintWidget::setTarget(const QPrinterInfo* target)
{
	int target_index = printers.size()-1;
	if (target == MapPrinter::pdfTarget())
		target_index = PdfExporter;
	else if (target == MapPrinter::imageTarget())
		target_index = ImageExporter;
	else
	{
		for (; target_index >= 0; target_index--)
		{
			if (target != NULL && printers[target_index].printerName() == target->printerName())
				break;
			else if (target == NULL)
				break;
		}
	}
	target_combo->setCurrentIndex(target_combo->findData(QVariant(target_index)));
	
	updatePaperSizes(target);
	updateResolutions(target);
	
	bool supports_pages = (target != MapPrinter::imageTarget());
	bool supports_copies = (supports_pages && target != NULL && QPrinter(*target).supportsMultipleCopies());
	copies_edit->setEnabled(supports_copies);
	layout->labelForField(copies_edit)->setEnabled(supports_copies);
	
	print_button->setText(map_printer->isPrinter() ? tr("Print") : tr("Export"));
}

// slot
void PrintWidget::targetChanged(int index) const
{
	if (index < 0)
		return;
	
	int target_index = target_combo->itemData(index).toInt();
	Q_ASSERT(target_index >= -2);
	Q_ASSERT(target_index < (int)printers.size());
	
	const QPrinterInfo* target = NULL;
	if (target_index == PdfExporter)
		target = MapPrinter::pdfTarget();
	else if (target_index == ImageExporter)
		target = MapPrinter::imageTarget();
	else
		target = &printers[target_index];
	
	map_printer->setTarget(target);
}



void PrintWidget::updatePaperSizes(const QPrinterInfo* target) const
{
	QString prev_paper_size_name = paper_size_combo->currentText();
	bool have_custom_size = false;
	
	{
		ScopedSignalsBlocker block(paper_size_combo);
		
		paper_size_combo->clear();
		QList<QPrinter::PaperSize> size_list;
		if (target != NULL)
			size_list = target->supportedPaperSizes();
		if (size_list.isEmpty())
			size_list = defaultPaperSizes();
		
		foreach (QPrinter::PaperSize size, size_list)
		{
			if (size == QPrinter::Custom)
				have_custom_size = true; // add it once after all other entires
			else
				paper_size_combo->addItem(toString(size), size);
		}
		
		if (have_custom_size)
			paper_size_combo->addItem(toString(QPrinter::Custom), QPrinter::Custom);
	
		int paper_size_index = paper_size_combo->findData((int)map_printer->getPageFormat().paper_size);
		if (!prev_paper_size_name.isEmpty())
		{
			paper_size_index = paper_size_combo->findText(prev_paper_size_name);
		}
		paper_size_combo->setCurrentIndex(qMax(0, paper_size_index));
		paperSizeChanged(paper_size_combo->currentIndex());
	}
}

// slot
void PrintWidget::setPageFormat(const MapPrinterPageFormat& format)
{
	ScopedMultiSignalsBlocker block;
	block << paper_size_combo << page_orientation_combo;
	paper_size_combo->setCurrentIndex(paper_size_combo->findData(format.paper_size));
	page_orientation_combo->setCurrentIndex(page_orientation_combo->findData(format.orientation));
	applyPrintAreaPolicy();
}

// slot
void PrintWidget::paperSizeChanged(int index) const
{
	if (index >= 0)
	{
		QPrinter::PaperSize paper_size = (QPrinter::PaperSize)paper_size_combo->itemData(index).toInt();
		map_printer->setPaperSize(paper_size);
	}
}

// slot
void PrintWidget::pageOrientationChanged(int index) const
{
	if (index >= 0)
	{
		QPrinter::Orientation orientation = (QPrinter::Orientation)page_orientation_combo->itemData(index).toInt();
		map_printer->setPageOrientation(orientation);
	}
}



// slot
void PrintWidget::applyPrintAreaPolicy() const
{
	QRectF print_area = map_printer->getPrintArea();
	
	PrintAreaPolicy policy = (PrintAreaPolicy)policy_combo->itemData(policy_combo->currentIndex()).toInt();
	if (policy == SinglePage)
		print_area.setSize(map_printer->getPageFormat().page_rect.size() / map_printer->getScaleAdjustment());
	
	if (center_check->isChecked())
		centerOnMap(print_area);

	map_printer->setPrintArea(print_area);
}

void PrintWidget::centerOnMap(QRectF& area) const
{
	QRectF map_extent = map->calculateExtent(false, show_templates_check->isChecked(), main_view);
	area.moveLeft(map_extent.center().x() - 0.5f * area.width());
	area.moveTop(map_extent.center().y() - 0.5f * area.height());
}



// slot
void PrintWidget::setPrintArea(const QRectF& area)
{
	ScopedMultiSignalsBlocker block;
	block << left_edit << top_edit << width_edit << height_edit;
	
	left_edit->setValue(area.left());
	top_edit->setValue(-area.top()); // Flip sign!
	width_edit->setValue(area.width());
	height_edit->setValue(area.height());
	
	if (center_check->isChecked())
	{
		QRectF centered_area = map_printer->getPrintArea();
		centerOnMap(centered_area);
		if (qAbs(centered_area.left() - area.left()) > 0.005f ||
			qAbs(centered_area.top() - area.top()) > 0.005f)
		{
			// No longer centered.
			center_check->setChecked(false);
		}
	}
	
	PrintAreaPolicy policy = (PrintAreaPolicy)policy_combo->itemData(policy_combo->currentIndex()).toInt();
	if (policy == SinglePage)
	{
		if (map_printer->getPageFormat().paper_size != QPrinter::Custom)
		{
			QSizeF page_dimensions = map_printer->getPageFormat().page_rect.size() / map_printer->getScaleAdjustment();
			if ( qAbs(area.width() - page_dimensions.width()) > 0.05 || 
				 qAbs(area.height() - page_dimensions.height()) > 0.05 )
			{
				// No longer single page.
				block << policy_combo;
				policy_combo->setCurrentIndex(policy_combo->findData(CustomArea));
				center_check->setChecked(false);
			}
		}
	}
}

// slot
void PrintWidget::printAreaMoved()
{
	QRectF area = map_printer->getPrintArea();
	area.moveLeft(left_edit->value());
	area.moveTop(-top_edit->value()); // Flip sign!
	map_printer->setPrintArea(area);
}

// slot
void PrintWidget::printAreaResized()
{
	QRectF area = map_printer->getPrintArea();
	area.setWidth(width_edit->value());
	area.setHeight(height_edit->value());
	map_printer->setPrintArea(area);
}



// slot
void PrintWidget::setOptions(const MapPrinterOptions& options)
{
	ScopedMultiSignalsBlocker block;
	block << dpi_combo->lineEdit() << show_templates_check 
	      << show_grid_check << overprinting_check
	      << different_scale_check << different_scale_edit;
	
	static QString dpi_template("%1 " + tr("dpi"));
	dpi_combo->setEditText(dpi_template.arg(options.resolution));
	show_templates_check->setChecked(options.show_templates);
	show_grid_check->setChecked(options.show_grid);
	overprinting_check->setChecked(options.simulate_overprinting);
	different_scale_edit->setValue(options.scale);
	if (options.scale != map->getScaleDenominator())
	{
		different_scale_check->setChecked(true);
		different_scale_edit->setEnabled(true);
	}
	applyPrintAreaPolicy();
}

void PrintWidget::updateResolutions(const QPrinterInfo* target) const
{
	static QList<int> default_resolutions(QList<int>() << 150 << 300 << 600 << 1200);
	
	// Numeric resolution list
	QList<int> supported_resolutions;
	if (target != NULL)
	{
		QPrinter pr(*target, QPrinter::HighResolution);
		supported_resolutions = pr.supportedResolutions();
		if (supported_resolutions.size() == 1 && supported_resolutions[0] == 72)
		{
			// X11/CUPS
			supported_resolutions.clear();
		}
	}
	if (supported_resolutions.isEmpty())
		supported_resolutions = default_resolutions;
	
	// Resolution list item with unit "dpi"
	static QString dpi_template("%1 " + tr("dpi"));
	QStringList resolutions;
	Q_FOREACH(int resolution, supported_resolutions)
		resolutions << dpi_template.arg(resolution);
	
	QString dpi_text = dpi_combo->currentText();
	{
		ScopedSignalsBlocker block(dpi_combo);
		dpi_combo->clear();
		dpi_combo->addItems(resolutions);
	}
	dpi_combo->lineEdit()->setText(dpi_text.isEmpty() ? dpi_template.arg(600) : dpi_text);
}

// slot
void PrintWidget::resolutionEdited()
{
	QString resolution_text = dpi_combo->currentText();
	int index_of_space = resolution_text.indexOf(" ");
	int dpi_value = resolution_text.left(index_of_space).toInt();
	if (dpi_value > 0)
		map_printer->setResolution(dpi_value);
}

// slot
void PrintWidget::differentScaleClicked(bool checked)
{
	if (!checked)
		different_scale_edit->setValue(map->getScaleDenominator());
	
	different_scale_edit->setEnabled(checked);
}

// slot
void PrintWidget::differentScaleEdited(int value)
{
	map_printer->setScale(value);
}

// slot
void PrintWidget::showTemplatesClicked(bool checked)
{
	map_printer->setPrintTemplates(checked, main_view);
	applyPrintAreaPolicy();
}

// slot
void PrintWidget::showGridClicked(bool checked)
{
	map_printer->setPrintGrid(checked);
}

// slot
void PrintWidget::overprintingClicked(bool checked)
{
	map_printer->setSimulateOverprinting(checked);
}

// slot
void PrintWidget::previewClicked()
{
	if (checkForEmptyMap())
		return;
	
	QPrinter* printer = map_printer->makePrinter();
	QPrintPreviewDialog preview(printer, this);
	connect(&preview, SIGNAL(paintRequested(QPrinter*)), map_printer, SLOT(printMap(QPrinter*)));
	preview.exec();
	
	delete printer;
}

// slot
void PrintWidget::printClicked()
{
	if (checkForEmptyMap())
		return;
	
	if (map->isAreaHatchingEnabled() || map->isBaselineViewEnabled())
	{
		if (QMessageBox::question(this, tr("Warning"), tr("A non-standard view mode is activated. Are you sure to print / export the map like this?"), QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Cancel)
			return;
	}
	
	QString path;
	if (!map_printer->isPrinter())
	{
		path = QFileDialog::getSaveFileName(this, tr("Export map ..."), QString(), tr("All files (*.*)"));
		if (path.isEmpty())
			return;
	}
	
	if (map_printer->getTarget() == MapPrinter::imageTarget())
	{
		if (!path.endsWith(".bmp", Qt::CaseInsensitive) && !path.endsWith(".png", Qt::CaseInsensitive) && !path.endsWith(".jpg", Qt::CaseInsensitive) &&
			!path.endsWith(".tiff", Qt::CaseInsensitive) && !path.endsWith(".jpeg", Qt::CaseInsensitive))
			path.append(".png");
		
		qreal pixel_per_mm = map_printer->getOptions().resolution / 25.4;
		
		int print_width = qRound(map_printer->getPrintArea().width() * pixel_per_mm);
		int print_height = qRound(map_printer->getPrintArea().height() * pixel_per_mm);
		QImage image(print_width, print_height, QImage::Format_ARGB32_Premultiplied);
		int dots_per_meter = qRound(pixel_per_mm * 1000);
		image.setDotsPerMeterX(dots_per_meter);
		image.setDotsPerMeterY(dots_per_meter);
		QPainter p(&image);
		map_printer->drawPage(&p, map_printer->getOptions().resolution, map_printer->getPrintArea(), true);
		p.end();
		if (!image.save(path))
		{
			QMessageBox::warning(this, tr("Error"), tr("Failed to save the image. Does the path exist? Do you have sufficient rights?"));
			return;
		}
		
		main_window->statusBar()->showMessage(tr("Exported successfully to %1").arg(path), 4000);
		emit finished(0);
		return;
	}
	
	QPrinter* printer = map_printer->makePrinter();
	printer->setNumCopies(copies_edit->value());
	if (map_printer->getTarget() == MapPrinter::pdfTarget())
	{
#if QT_VERSION < 0x050000		
		if (path.endsWith(".ps", Qt::CaseInsensitive))
			printer->setOutputFormat(QPrinter::PostScriptFormat);
		else
#endif
		{
			printer->setOutputFormat(QPrinter::PdfFormat);
			if (!path.endsWith(".pdf", Qt::CaseInsensitive))
				path.append(".pdf");
		}
		printer->setOutputFileName(path);
		printer->setCreator(main_window->appName());
	}
	
	// Print the map
	map_printer->printMap(printer);
	delete printer;
	
	if (map_printer->isPrinter())
		main_window->statusBar()->showMessage(tr("Successfully created print job"), 4000);
	else
		main_window->statusBar()->showMessage(tr("Exported successfully to %1").arg(path), 4000);
	
	emit finished(0);
}

QList<QPrinter::PaperSize> PrintWidget::defaultPaperSizes() const
{
	static QList<QPrinter::PaperSize> default_paper_sizes(QList<QPrinter::PaperSize>()
	  << QPrinter::A4
	  << QPrinter::Letter
	  << QPrinter::Legal
	  << QPrinter::Executive
	  << QPrinter::A0
	  << QPrinter::A1
	  << QPrinter::A2
	  << QPrinter::A3
	  << QPrinter::A5
	  << QPrinter::A6
	  << QPrinter::A7
	  << QPrinter::A8
	  << QPrinter::A9
	  << QPrinter::B0
	  << QPrinter::B1
	  << QPrinter::B10
	  << QPrinter::B2
	  << QPrinter::B3
	  << QPrinter::B4
	  << QPrinter::B5
	  << QPrinter::B6
	  << QPrinter::B7
	  << QPrinter::B8
	  << QPrinter::B9
	  << QPrinter::C5E
	  << QPrinter::Comm10E
	  << QPrinter::DLE
	  << QPrinter::Folio
	  << QPrinter::Ledger
	  << QPrinter::Tabloid
	  << QPrinter::Custom
	);
	return default_paper_sizes;
}

const QString toString(QPrinter::PaperSize size)
{
	QString name;
	switch (size)
	{
	case QPrinter::A4:			name = "A4";			break;
	case QPrinter::B5:			name = "B5";			break;
	case QPrinter::Letter:		name = QObject::tr("Letter");	break;
	case QPrinter::Legal:		name = QObject::tr("Legal");		break;
	case QPrinter::Executive:	name = QObject::tr("Executive");	break;
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
	case QPrinter::C5E:			name = QObject::tr("C5E");		break;
	case QPrinter::Comm10E:		name = QObject::tr("Comm10E");	break;
	case QPrinter::DLE:			name = QObject::tr("DLE");		break;
	case QPrinter::Folio:		name = QObject::tr("Folio");		break;
	case QPrinter::Ledger:		name = QObject::tr("Ledger");	break;
	case QPrinter::Tabloid:		name = QObject::tr("Tabloid");	break;
	case QPrinter::Custom:		name = QObject::tr("Custom");	break;
	default:					name = QObject::tr("Unknown");	break;
	}
	
	return name;
}

bool PrintWidget::checkForEmptyMap()
{
	if (map_printer->isOutputEmpty())
	{
		QMessageBox::warning(this, tr("Error"), tr("The map area is empty. Output canceled."));
		return true;
	}
	return false;
}
