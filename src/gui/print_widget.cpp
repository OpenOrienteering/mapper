/*
 *    Copyright 2012, 2013 Thomas Schöps, Kai Pastor
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

#include "print_widget.h"

#include <limits>

#include <QFileInfo>
#include <QPrintDialog>
#include <QPrintPreviewDialog>

#include "main_window.h"
#include "print_progress_dialog.h"
#include "../core/map_printer.h"
#include "../map.h"
#include "../map_editor.h"
#include "../map_widget.h"
#include "../settings.h"
#include "../util.h"
#include "../util_gui.h"
#include "../util/scoped_signals_blocker.h"
#include "print_tool.h"


namespace
{
	QToolButton* createPrintModeButton(const QIcon& icon, const QString& label, QWidget* parent = NULL)
	{
		static const QSize icon_size(48,48);
		QToolButton* button = new QToolButton(parent);
		button->setAutoRaise(true);
		button->setCheckable(true);
		button->setIconSize(icon_size);
		button->setIcon(icon);
		button->setText(label);
		button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
		return button;
	}
}

//### PrintWidget ###

PrintWidget::PrintWidget(Map* map, MainWindow* main_window, MapView* main_view, MapEditorController* editor, QWidget* parent)
: QWidget(parent), 
  task(UNDEFINED_TASK),
  map(map), 
  map_printer(new MapPrinter(*map, main_view)),
  main_window(main_window), 
  main_view(main_view), 
  editor(editor),
  print_tool(NULL),
  active(false)
{
	Q_ASSERT(main_window != NULL);
	
	layout = new QFormLayout();
	
	target_combo = new QComboBox();
	target_combo->setMinimumWidth(1); // Not zero, but not as long as the items
	layout->addRow(Util::Headline::create(tr("Printer:")), target_combo);
	
	paper_size_combo = new QComboBox();
	layout->addRow(tr("Page format:"), paper_size_combo);
	
	QWidget* page_size_widget = new QWidget();
	QHBoxLayout* page_size_layout = new QHBoxLayout();
	page_size_widget->setLayout(page_size_layout);
	page_size_layout->setMargin(0);
	page_width_edit = Util::SpinBox::create(1, 0.1, 1000.0, tr("mm"), 1.0);
	page_width_edit->setEnabled(false);
	page_size_layout->addWidget(page_width_edit, 1);
	page_size_layout->addWidget(new QLabel("x"), 0);
	page_height_edit = Util::SpinBox::create(1, 0.1, 1000.0, tr("mm"), 1.0);
	page_height_edit->setEnabled(false);
	page_size_layout->addWidget(page_height_edit, 1);
	layout->addRow("", page_size_widget);
	
	page_orientation_widget = new QWidget();
	QBoxLayout* page_orientation_layout = new QHBoxLayout();
	page_orientation_layout->setContentsMargins(QMargins());
	page_orientation_widget->setLayout(page_orientation_layout);
	QRadioButton* portrait_button = new QRadioButton(tr("Portrait"));
	page_orientation_layout->addWidget(portrait_button);
	QRadioButton* landscape_button = new QRadioButton(tr("Landscape"));
	page_orientation_layout->addWidget(landscape_button);
	page_orientation_group = new QButtonGroup(this);
	page_orientation_group->addButton(portrait_button, QPrinter::Portrait);
	page_orientation_group->addButton(landscape_button, QPrinter::Landscape);
	layout->addRow(tr("Page orientation:"), page_orientation_widget);
	
	copies_edit = Util::SpinBox::create(1, 99999);
	layout->addRow(tr("Copies:"), copies_edit);
	
	layout->addItem(Util::SpacerItem::create(this));
	
	policy_combo = new QComboBox();
	policy_combo->addItem(tr("Single page"), SinglePage);
	policy_combo->addItem(tr("Custom area"), CustomArea);
	layout->addRow(Util::Headline::create(tr("Map area:")), policy_combo); // or print/export area
	
	center_check = new QCheckBox(tr("Center print area"));
	layout->addRow(center_check);
	
	left_edit = Util::SpinBox::create(2, -999999.9, 999999.9, tr("mm"), 1.0);
	layout->addRow(tr("Left:"), left_edit);
	
	top_edit = Util::SpinBox::create(2, -999999.9, 999999.9, tr("mm"), 1.0);
	layout->addRow(tr("Top:"), top_edit);
	
	width_edit = Util::SpinBox::create(2, -999999.9, 999999.9, tr("mm"), 1.0);
	layout->addRow(tr("Width:"), width_edit);
	
	height_edit = Util::SpinBox::create(2, -999999.9, 999999.9, tr("mm"), 1.0);
	layout->addRow(tr("Height:"), height_edit);
	
	overlap_edit = Util::SpinBox::create(2, -999999.9, 999999.9, tr("mm"), 1.0);
	layout->addRow(tr("Page overlap:"), overlap_edit);
	
	layout->addItem(Util::SpacerItem::create(this));
	
	layout->addRow(Util::Headline::create(tr("Options")));
	
	QWidget* mode_widget = new QWidget();
	QBoxLayout* mode_layout = new QHBoxLayout();
	mode_widget->setLayout(mode_layout);
	mode_layout->setMargin(0);
	
	vector_mode_button = createPrintModeButton(QIcon(":/images/print-mode-vector.png"), tr("Vector\ngraphics"));
	raster_mode_button = createPrintModeButton(QIcon(":/images/print-mode-raster.png"), tr("Raster\ngraphics"));
	separations_mode_button = createPrintModeButton(QIcon(":/images/print-mode-separations.png"), tr("Color\nseparations"));
	vector_mode_button->setChecked(true);
	
	QButtonGroup* mode_button_group = new QButtonGroup(this);
	mode_button_group->addButton(vector_mode_button);
	mode_button_group->addButton(raster_mode_button);
	mode_button_group->addButton(separations_mode_button);
	
	mode_layout->addWidget(vector_mode_button);
	mode_layout->addWidget(raster_mode_button);
	mode_layout->addWidget(separations_mode_button);
	mode_layout->addStretch(1);
	
	layout->addRow(tr("Mode:"), mode_widget);
	
	dpi_combo = new QComboBox();
	dpi_combo->setEditable(true);
	dpi_combo->setValidator(new QRegExpValidator(QRegExp("^[1-9]\\d{1,4} dpi$|^[1-9]\\d{1,4}$"), dpi_combo));
	// TODO: Implement spinbox-style " dpi" suffix
	layout->addRow(tr("Resolution:"), dpi_combo);
	
	different_scale_check = new QCheckBox(tr("Print in different scale:"));
	// Limit the difference between nominal and printing scale in order to limit the number of page breaks.
	int min_scale = qMax((unsigned int)1, map->getScaleDenominator() / 10000 * 100);
	different_scale_edit = Util::SpinBox::create(min_scale, std::numeric_limits<int>::max(), "", 500);
	different_scale_edit->setPrefix("1 : ");
	different_scale_edit->setEnabled(false);
	int different_scale_height = qMax(
	  different_scale_edit->minimumSizeHint().height(),
	  different_scale_check->minimumSizeHint().height() );
	different_scale_check->setMinimumHeight(different_scale_height);
	different_scale_edit->setMinimumHeight(different_scale_height);
	layout->addRow(different_scale_check, different_scale_edit);
	
	// this must be created before its value is used to determine the default setting of page_orientation_combo
	show_templates_check = new QCheckBox(tr("Show templates"));
	QHBoxLayout* templates_warning_layout = new QHBoxLayout();
	QIcon warning_icon = style()->standardIcon(QStyle::SP_MessageBoxWarning);
	templates_warning_icon = new QLabel();
	int pixmap_size = qBound(8, style()->pixelMetric(QStyle::PM_IndicatorHeight), 32);
	templates_warning_icon->setPixmap(warning_icon.pixmap(QSize(pixmap_size, pixmap_size)));
	templates_warning_layout->addWidget(templates_warning_icon);
	templates_warning_text = new QLabel(tr("Template appearance may differ."));
	templates_warning_layout->addWidget(templates_warning_text, 1);
	layout->addRow(show_templates_check, templates_warning_layout);
	
	show_grid_check = new QCheckBox(tr("Show grid"));
	layout->addRow(show_grid_check);
	
	overprinting_check = new QCheckBox(tr("Simulate overprinting"));
	layout->addRow(overprinting_check);
	
	QWidget* scrolling_content = new QWidget();
	scrolling_content->setLayout(layout);
	
	QBoxLayout* outer_layout = new QVBoxLayout();
	outer_layout->setContentsMargins(QMargins());
	
	QScrollArea* scroll_area = new QScrollArea();
	scroll_area->setWidget(scrolling_content);
	scroll_area->setWidgetResizable(true);
	scroll_area->setFrameShape(QFrame::NoFrame);
	scroll_area->setMinimumWidth((scrolling_content->sizeHint() + scroll_area->verticalScrollBar()->sizeHint()).width());
	scroll_area->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContentsOnFirstShow);
	outer_layout->addWidget(scroll_area);
	
	QDialogButtonBox* button_box = new QDialogButtonBox();
	QStyleOption style_option(QStyleOption::Version, QStyleOption::SO_DockWidget);
	button_box->layout()->setContentsMargins(
	    style()->pixelMetric(QStyle::PM_LayoutLeftMargin, &style_option),
	    style()->pixelMetric(QStyle::PM_LayoutTopMargin, &style_option),
	    style()->pixelMetric(QStyle::PM_LayoutRightMargin, &style_option),
	    style()->pixelMetric(QStyle::PM_LayoutBottomMargin, &style_option)
	);
	preview_button = new QPushButton(tr("Preview..."));
	button_box->addButton(preview_button, QDialogButtonBox::ActionRole);
	print_button = new QPushButton(tr("Print"));
	button_box->addButton(print_button, QDialogButtonBox::ActionRole);
	// Use a distinct export button.
	// Changing the text at runtime causes distortions on Mac OS X.
	export_button = new QPushButton(tr("Export..."));
	export_button->hide();
	button_box->addButton(export_button, QDialogButtonBox::ActionRole);
	QPushButton* close_button = button_box->addButton(QDialogButtonBox::Close);
	outer_layout->addWidget(button_box);
	
	setLayout(outer_layout);
	
	connect(target_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(targetChanged(int)));
	connect(paper_size_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(paperSizeChanged(int)));
	connect(page_width_edit, SIGNAL(valueChanged(double)), this, SLOT(paperDimensionsChanged()));
	connect(page_orientation_group, SIGNAL(buttonClicked(int)), this, SLOT(pageOrientationChanged(int)));
	connect(page_height_edit, SIGNAL(valueChanged(double)), this, SLOT(paperDimensionsChanged()));
	
	connect(top_edit, SIGNAL(valueChanged(double)), this, SLOT(printAreaMoved()));
	connect(left_edit, SIGNAL(valueChanged(double)), this, SLOT(printAreaMoved()));
	connect(width_edit, SIGNAL(valueChanged(double)), this, SLOT(printAreaResized()));
	connect(height_edit, SIGNAL(valueChanged(double)), this, SLOT(printAreaResized()));
	connect(overlap_edit, SIGNAL(valueChanged(double)), this, SLOT(overlapEdited(double)));
	
	connect(mode_button_group, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(printModeChanged(QAbstractButton*)));
	connect(dpi_combo->lineEdit(), SIGNAL(editingFinished()), this, SLOT(resolutionEdited()));
	connect(different_scale_check, SIGNAL(clicked(bool)), this, SLOT(differentScaleClicked(bool)));
	connect(different_scale_edit, SIGNAL(valueChanged(int)), this, SLOT(differentScaleEdited(int)));
	connect(show_templates_check, SIGNAL(clicked(bool)), this, SLOT(showTemplatesClicked(bool)));
	connect(show_grid_check, SIGNAL(clicked(bool)), this, SLOT(showGridClicked(bool)));
	connect(overprinting_check, SIGNAL(clicked(bool)), this, SLOT(overprintingClicked(bool)));
	
	connect(preview_button, SIGNAL(clicked(bool)), this, SLOT(previewClicked()));
	connect(print_button, SIGNAL(clicked(bool)), this, SLOT(printClicked()));
	connect(export_button, SIGNAL(clicked(bool)), this, SLOT(printClicked()));
	connect(close_button, SIGNAL(clicked(bool)), this, SIGNAL(closeClicked()));
	
	policy = map->printerConfig().single_page_print_area ? SinglePage : CustomArea;
	policy_combo->setCurrentIndex(policy_combo->findData(policy));
	connect(policy_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(printAreaPolicyChanged(int)));
	
	center_check->setChecked(map->printerConfig().center_print_area);
	connect(center_check, SIGNAL(clicked(bool)), this, SLOT(applyCenterPolicy()));
	
	setPageFormat(map_printer->getPageFormat());
	connect(map_printer, SIGNAL(pageFormatChanged(const MapPrinterPageFormat&)),
	        this, SLOT(setPageFormat(const MapPrinterPageFormat&)));
	
	connect(map_printer, SIGNAL(optionsChanged(const MapPrinterOptions&)), this, SLOT(setOptions(const MapPrinterOptions&)));
	spotColorPresenceChanged(map->hasSpotColors());
	connect(map, SIGNAL(spotColorPresenceChanged(bool)), this, SLOT(spotColorPresenceChanged(bool)));
	
	setPrintArea(map_printer->getPrintArea());
	connect(map_printer, SIGNAL(printAreaChanged(QRectF)), this, SLOT(setPrintArea(QRectF)));
	
	connect(map_printer, SIGNAL(targetChanged(const QPrinterInfo*)), this, SLOT(setTarget(const QPrinterInfo*)));
	
	connect(this, SIGNAL(finished(int)), this, SLOT(savePrinterConfig()));
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
void PrintWidget::setTask(PrintWidget::TaskFlags type)
{
	if (task != type)
	{
		task = type;
		bool is_print_task = type==PRINT_TASK;
		bool is_multipage  = type.testFlag(MULTIPAGE_FLAG);
		layout->labelForField(target_combo)->setVisible(is_print_task);
		target_combo->setVisible(is_print_task);
		layout->labelForField(copies_edit)->setVisible(is_multipage);
		copies_edit->setVisible(is_multipage);
		policy_combo->setVisible(is_multipage);
		updateTargets();
		switch (type)
		{
			case PRINT_TASK:
				// Reset values which are typically modified for exporting
				if (policy == SinglePage && !map->printerConfig().single_page_print_area)
				{
					policy = CustomArea;
					policy_combo->setCurrentIndex(policy_combo->findData(policy));
				}
				if (map_printer->getPageFormat().paper_size == QPrinter::Custom)
				{
					map_printer->setPaperSize(map->printerConfig().page_format.paper_size);
				}
				// TODO: Set target to most recently used printer
				emit taskChanged(tr("Print"));
				break;
				
			case EXPORT_PDF_TASK:
				map_printer->setTarget(MapPrinter::pdfTarget());
				emit taskChanged(tr("PDF export"));
				break;
				
			case EXPORT_IMAGE_TASK:
				map_printer->setTarget(MapPrinter::imageTarget());
				policy = SinglePage;
				if (policy_combo->itemData(policy_combo->currentIndex()) != policy)
				{
					map_printer->setCustomPaperSize(map_printer->getPrintAreaPaperSize());
					policy_combo->setCurrentIndex(policy_combo->findData(policy));
				}
				emit taskChanged(tr("Image export"));
				break;
				
			default:
				emit taskChanged(QString::null);
		}
	}
}

// slot
void PrintWidget::savePrinterConfig() const
{
	MapPrinterConfig printer_config(map_printer->config());
	printer_config.center_print_area = center_check->isChecked();
	if (task.testFlag(MULTIPAGE_FLAG))
	{
		printer_config.single_page_print_area = policy == SinglePage;
	}
	if (task.testFlag(EXPORT_IMAGE_TASK))
	{
		// Don't override the printer page format from the custom image format.
		printer_config.page_format = map->printerConfig().page_format;
	}
	map->setPrinterConfig(printer_config);
}

// slot
void PrintWidget::setActive(bool active)
{
	if (this->active != active)
	{
		this->active = active;
		
		if (active)
		{
			// Save the current state of the map view.
			saved_view_state.clear();
			QXmlStreamWriter writer(&saved_view_state);
			main_view->save(writer, QLatin1String("saved_view"), true);
			
			editor->setViewOptionsEnabled(false);
			
			// Printers may have been added or removed.
			updateTargets();
			
			// Update the map view from the current options
			setOptions(map_printer->getOptions());
			
			// Set reasonable zoom.
			bool zoom_to_map = true;
			if (zoom_to_map)
			{
				// Ensure the visibility of the whole map.
				QRectF map_extent = map->calculateExtent(true, !main_view->areAllTemplatesHidden(), main_view);
				editor->getMainWidget()->ensureVisibilityOfRect(map_extent, true, false);
			}
			else
			{
				// Ensure the visibility of the print area.
				QRectF print_area(map_printer->getPrintArea());
				editor->getMainWidget()->ensureVisibilityOfRect(print_area, true, false);
			}
			
			// Activate PrintTool.
			if (!print_tool)
			{
				print_tool = new PrintTool(editor, map_printer);
			}
			editor->setOverrideTool(print_tool);
		}
		else
		{
			editor->setOverrideTool(NULL);
			print_tool = NULL;
			
			// Restore view
			QXmlStreamReader reader(saved_view_state);
			reader.readNextStartElement();
			main_view->load(reader);
			main_view->updateAllMapWidgets();
			
			editor->setViewOptionsEnabled(true);
		}
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
		const QSignalBlocker block(target_combo);
		target_combo->clear();
		
		if (task == PRINT_TASK)
		{
			// Exporters
			target_combo->addItem(tr("Save to PDF"), QVariant((int)PdfExporter));
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
	
	bool is_printer = map_printer->isPrinter();
	print_button->setVisible(is_printer);
	print_button->setDefault(is_printer);
	export_button->setVisible(!is_printer);
	export_button->setDefault(!is_printer);
	
	bool isImageTarget = target == MapPrinter::imageTarget();
	vector_mode_button->setEnabled(!isImageTarget);
	separations_mode_button->setEnabled(!isImageTarget && map->hasSpotColors());
	if (isImageTarget)
	{
		raster_mode_button->setChecked(true);
		printModeChanged(raster_mode_button);
	}
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
		const QSignalBlocker block(paper_size_combo);
		
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
	block << paper_size_combo << page_orientation_group << overlap_edit << page_width_edit << page_height_edit;
	paper_size_combo->setCurrentIndex(paper_size_combo->findData(format.paper_size));
	page_orientation_group->button(format.orientation)->setChecked(true);
	page_width_edit->setValue(format.paper_dimensions.width());
	page_width_edit->setEnabled(format.paper_size == QPrinter::Custom);
	page_height_edit->setValue(format.paper_dimensions.height());
	page_height_edit->setEnabled(format.paper_size == QPrinter::Custom);
	// We only have a single overlap edit field, but MapPrinter supports 
	// distinct horizontal and vertical overlap. Choose the minimum.
	overlap_edit->setValue(qMin(format.h_overlap, format.v_overlap));
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
void PrintWidget::paperDimensionsChanged() const
{
	const QSizeF dimensions(page_width_edit->value(), page_height_edit->value());
	map_printer->setCustomPaperSize(dimensions);
}

// slot
void PrintWidget::pageOrientationChanged(int id) const
{
	if (id == QPrinter::Portrait || id == QPrinter::Landscape)
	{
		map_printer->setPageOrientation((id == QPrinter::Portrait) ? MapPrinterPageFormat::Portrait : MapPrinterPageFormat::Landscape);
	}
}


// slot
void PrintWidget::printAreaPolicyChanged(int index)
{
	policy = (PrintAreaPolicy)policy_combo->itemData(index).toInt();
	applyPrintAreaPolicy();
}


// slot
void PrintWidget::applyPrintAreaPolicy() const
{
	if (policy == SinglePage)
	{
		setOverlapEditEnabled(false);
		QRectF print_area = map_printer->getPrintArea();
		print_area.setSize(map_printer->getPageRectPrintAreaSize());
		if (center_check->isChecked())
			centerOnMap(print_area);
		map_printer->setPrintArea(print_area);
	}
	else
	{
		setOverlapEditEnabled(true);
	}
}

// slot
void PrintWidget::applyCenterPolicy() const
{
	if (center_check->isChecked())
	{
		QRectF print_area = map_printer->getPrintArea();
		centerOnMap(print_area);
		map_printer->setPrintArea(print_area);
	}
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
		if ( qAbs(centered_area.left() - area.left()) > 0.005 ||
			 qAbs(centered_area.top()  - area.top())  > 0.005 )
		{
			// No longer centered.
			center_check->setChecked(false);
		}
	}
	
	if (policy == SinglePage)
	{
		if (map_printer->getPageFormat().paper_size == QPrinter::Custom || !task.testFlag(MULTIPAGE_FLAG))
		{
			// Update custom paper size from print area size
			QSizeF area_dimensions = area.size() * map_printer->getScaleAdjustment();
			if (map_printer->getPageFormat().page_rect.size() != area_dimensions)
			{
				// Don't force a custom paper size unless necessary
				map_printer->setCustomPaperSize(area_dimensions);
			}
		}
		else
		{
			QSizeF page_dimensions = map_printer->getPageRectPrintAreaSize();
			if ( qAbs(area.width()  - page_dimensions.width())  > 0.005 || 
				 qAbs(area.height() - page_dimensions.height()) > 0.005 )
			{
				// No longer single page.
				block << policy_combo;
				policy_combo->setCurrentIndex(policy_combo->findData(CustomArea));
				center_check->setChecked(false);
				setOverlapEditEnabled(true);
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
void PrintWidget::overlapEdited(double overlap)
{
	map_printer->setOverlap(overlap, overlap);
}

void PrintWidget::setOverlapEditEnabled(bool state) const
{
	overlap_edit->setEnabled(state);
	layout->labelForField(overlap_edit)->setEnabled(state);
}


// slot
void PrintWidget::setOptions(const MapPrinterOptions& options)
{
	using namespace Util::TristateCheckbox;
	
	ScopedMultiSignalsBlocker block;
	block << dpi_combo->lineEdit() << show_templates_check 
	      << show_grid_check << overprinting_check
	      << vector_mode_button << raster_mode_button
	      << separations_mode_button
	      << different_scale_check << different_scale_edit;
	
	switch (options.mode)
	{
	default:
		Q_ASSERT(false && "Unhandled MapPrinterOptions::MapPrinterMode");
		// fall through in release build
	case MapPrinterOptions::Vector:
		vector_mode_button->setChecked(true);
		setEnabledAndChecked(show_templates_check, options.show_templates);
		setEnabledAndChecked(show_grid_check,      options.show_grid);
		setDisabledAndChecked(overprinting_check,  options.simulate_overprinting);
		main_view->setHideAllTemplates(!options.show_templates);
		main_view->setGridVisible(options.show_grid);
		main_view->setOverprintingSimulationEnabled(false);
		break;
	case MapPrinterOptions::Raster:
		raster_mode_button->setChecked(true);
		setEnabledAndChecked(show_templates_check, options.show_templates);
		setEnabledAndChecked(show_grid_check,      options.show_grid);
		setEnabledAndChecked(overprinting_check,   options.simulate_overprinting);
		main_view->setHideAllTemplates(!options.show_templates);
		main_view->setGridVisible(options.show_grid);
		main_view->setOverprintingSimulationEnabled(options.simulate_overprinting);
		break;
	case MapPrinterOptions::Separations:
		separations_mode_button->setChecked(true);
		setDisabledAndChecked(show_templates_check, options.show_templates);
		setDisabledAndChecked(show_grid_check,      options.show_grid);
		setDisabledAndChecked(overprinting_check,   options.simulate_overprinting);
		main_view->setHideAllTemplates(true);
		main_view->setGridVisible(false);
		main_view->setOverprintingSimulationEnabled(true);
		break;
	}
	
	checkTemplateConfiguration();
	
	static QString dpi_template("%1 " + tr("dpi"));
	dpi_combo->setEditText(dpi_template.arg(options.resolution));
	
	if (different_scale_edit->value() != (int)options.scale)
	{
		different_scale_edit->setValue(options.scale);
		if (options.scale != map->getScaleDenominator())
		{
			different_scale_check->setChecked(true);
			different_scale_edit->setEnabled(true);
		}
		applyPrintAreaPolicy();
	}
	else
	{
		applyCenterPolicy();
	}
	main_view->updateAllMapWidgets();
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
		const QSignalBlocker block(dpi_combo);
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
	if (policy == SinglePage)
	{
		// Adjust the print area.
		applyPrintAreaPolicy();
	}
	
	if (different_scale_edit->value() < 500)
	{
		different_scale_edit->setSingleStep(500 - different_scale_edit->value());
	}
	else
	{
		different_scale_edit->setSingleStep(500);
	}
}

// slot
void PrintWidget::spotColorPresenceChanged(bool has_spot_colors)
{
	separations_mode_button->setEnabled(has_spot_colors);
	if (!has_spot_colors && separations_mode_button->isChecked())
	{
		map_printer->setMode(MapPrinterOptions::Vector);
	}
}

// slot
void PrintWidget::printModeChanged(QAbstractButton* button)
{
	if (button == vector_mode_button)
	{
		map_printer->setMode(MapPrinterOptions::Vector);
	}
	else if (button == raster_mode_button)
	{
		map_printer->setMode(MapPrinterOptions::Raster);
	}
	else
	{
		map_printer->setMode(MapPrinterOptions::Separations);
	}
}

// slot
void PrintWidget::showTemplatesClicked(bool checked)
{
	map_printer->setPrintTemplates(checked, main_view);
	checkTemplateConfiguration();
}

void PrintWidget::checkTemplateConfiguration()
{
	bool visibility = vector_mode_button->isChecked() && show_templates_check->isChecked();
	templates_warning_icon->setVisible(visibility);
	templates_warning_text->setVisible(visibility);
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
#if defined(Q_OS_ANDROID)
	// Qt for Android has no QPrintPreviewDialog
	QMessageBox::warning(this, tr("Error"), tr("Not supported on Android."));
#else
	if (checkForEmptyMap())
		return;
	
	PrintProgressDialog progress(main_window);
	progress.setWindowTitle(tr("Print Preview Progress"));
	progress.attach(map_printer);
	progress.show();
	
	QPrinter* printer = map_printer->makePrinter();
	printer->setCreator(main_window->appName());
	printer->setDocName(QFileInfo(main_window->currentPath()).baseName());
#if !defined(Q_OS_MAC)
	QPrintPreviewDialog preview(printer, this);
#else
	// https://bugreports.qt-project.org/browse/QTBUG-10206 :
	//   Mac QPrintPreviewDialog is missing Close icon
	QPrintPreviewDialog preview(printer, this, 
		Qt::Window | Qt::CustomizeWindowHint | Qt::WindowSystemMenuHint );
#endif
	connect(&preview, SIGNAL(paintRequested(QPrinter*)), map_printer, SLOT(printMap(QPrinter*)));
	preview.exec();
	
	delete printer;
#endif
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
		const QString filter_template("%1 (%2)");
		QStringList filters;
		if (map_printer->getTarget() == MapPrinter::imageTarget())
		{
			filters << filter_template.arg(tr("PNG")).arg("*.png");
			filters << filter_template.arg(tr("BMP")).arg("*.bmp");
			filters << filter_template.arg(tr("TIFF")).arg("*.tif *.tiff");
			filters << filter_template.arg(tr("JPEG")).arg("*.jpg *.jpeg");
		}
		else if (map_printer->getTarget() == MapPrinter::pdfTarget())
		{
			filters << filter_template.arg(tr("PDF")).arg("*.pdf");
		}
		filters << tr("All files (*.*)");
		path = QFileDialog::getSaveFileName(this, tr("Export map ..."), QString(), filters.join(";;"));
		if (path.isEmpty())
			return;
	}
	
	PrintProgressDialog progress(main_window);
	progress.setWindowTitle(tr("Printing Progress"));
	progress.setWindowModality(Qt::WindowModal);
	progress.attach(map_printer);
	progress.show();
	
	if (map_printer->getTarget() == MapPrinter::imageTarget())
	{
		if (!path.endsWith(".bmp", Qt::CaseInsensitive) && !path.endsWith(".png", Qt::CaseInsensitive) && !path.endsWith(".jpg", Qt::CaseInsensitive) &&
			!path.endsWith(".tiff", Qt::CaseInsensitive) && !path.endsWith(".jpeg", Qt::CaseInsensitive))
			path.append(".png");
		
		qreal pixel_per_mm = map_printer->getOptions().resolution / 25.4;
		
		int print_width = qRound(map_printer->getPrintAreaPaperSize().width() * pixel_per_mm);
		int print_height = qRound(map_printer->getPrintAreaPaperSize().height() * pixel_per_mm);
		QImage image(print_width, print_height, QImage::Format_ARGB32_Premultiplied);
		int dots_per_meter = qRound(pixel_per_mm * 1000);
		image.setDotsPerMeterX(dots_per_meter);
		image.setDotsPerMeterY(dots_per_meter);
		QPainter p(&image);
		map_printer->drawPage(&p, map_printer->getOptions().resolution, map_printer->getPrintArea(), true, &image);
		p.end();
		if (!image.save(path))
		{
			QMessageBox::warning(this, tr("Error"), tr("Failed to save the image. Does the path exist? Do you have sufficient rights?"));
			return;
		}
		
		main_window->showStatusBarMessage(tr("Exported successfully to %1").arg(path), 4000);
		emit finished(0);
		return;
	}
	
	QPrinter* printer = map_printer->makePrinter();
	printer->setNumCopies(copies_edit->value());
	printer->setCreator(main_window->appName());
	printer->setDocName(QFileInfo(main_window->currentPath()).baseName());
	if (map_printer->getTarget() == MapPrinter::pdfTarget())
	{
		printer->setOutputFormat(QPrinter::PdfFormat);
		if (!path.endsWith(".pdf", Qt::CaseInsensitive))
			path.append(".pdf");
		printer->setOutputFileName(path);
	}
	
	// Print the map
	map_printer->printMap(printer);
	
	if (progress.wasCanceled())
	{
		if (printer->abort() || !map_printer->isPrinter())
		{
			main_window->showStatusBarMessage(tr("Canceled."), 4000);
		}
		else
		{
			QMessageBox::warning(
			  main_window, tr("Printing"),
			  tr("The print job could not be stopped."),
			  QMessageBox::Ok, QMessageBox::Ok );
		}
	}
	else if (map_printer->isPrinter())
	{
		main_window->showStatusBarMessage(tr("Successfully created print job"), 4000);
	}
	else
	{
		main_window->showStatusBarMessage(tr("Exported successfully to %1").arg(path), 4000);
	}
	
	delete printer;
	emit finished(0);
}

QList<QPrinter::PaperSize> PrintWidget::defaultPaperSizes() const
{
	// TODO: Learn from user's past choices, present reduced list unless asked for more.
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

QString PrintWidget::toString(QPrinter::PaperSize size)
{
#if defined(Q_OS_ANDROID)
	// Qt for Android has no QPrintDialog
	Q_UNUSED(size);
	return tr("Unknown", "Paper size");
#else
	const QHash< int, const char*>& paper_size_names = MapPrinter::paperSizeNames();
	if (paper_size_names.contains(size))
		// These translations are not used in QPrintDialog, 
		// but in Qt's qpagesetupdialog_unix.cpp.
		return QPrintDialog::tr(paper_size_names[size]);
	else
		return tr("Unknown", "Paper size");
#endif
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

#endif
