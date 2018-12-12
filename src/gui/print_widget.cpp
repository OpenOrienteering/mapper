/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2018  Kai Pastor
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
#include <memory>

#include <Qt>
#include <QtGlobal>
#include <QAbstractButton> // IWYU pragma: keep
#include <QButtonGroup>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QHash>
#include <QHBoxLayout>
#include <QIcon>
#include <QImage>
#include <QLabel>
#include <QLatin1Char>
#include <QLatin1String>
#include <QLayout>
#include <QLineEdit>
#include <QMargins>
#include <QMessageBox>
#include <QPagedPaintDevice>
#include <QPainter>
#include <QPoint>
#include <QPointF>
#include <QPrinterInfo>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPushButton>
#include <QRadioButton>
#include <QRectF>
#include <QRegExp>
#include <QRegExpValidator>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QSizeF>
#include <QSpacerItem>
#include <QSpinBox>
#include <QStringRef>
#include <QStyle>
#include <QStyleOption>
#include <QToolButton>
#include <QTransform>
#include <QVBoxLayout>
#include <QVariant>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <printer_properties.h>

#include "core/georeferencing.h"
#include "core/map.h"
#include "core/map_coord.h"
#include "core/map_printer.h"
#include "core/map_view.h"
#include "gui/file_dialog.h"
#include "gui/main_window.h"
#include "gui/print_progress_dialog.h"
#include "gui/print_tool.h"
#include "gui/util_gui.h"
#include "gui/map/map_editor.h"
#include "gui/map/map_widget.h"
#include "templates/template.h" // IWYU pragma: keep
#include "templates/world_file.h"
#include "util/backports.h"
#include "util/scoped_signals_blocker.h"


namespace OpenOrienteering {

namespace {
	
	QToolButton* createPrintModeButton(const QIcon& icon, const QString& label, QWidget* parent = nullptr)
	{
		static const QSize icon_size(48,48);
		auto button = new QToolButton(parent);
		button->setAutoRaise(true);
		button->setCheckable(true);
		button->setIconSize(icon_size);
		button->setIcon(icon);
		button->setText(label);
		button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
		return button;
	}
	
	
}  // namespace


//### PrintWidget ###

PrintWidget::PrintWidget(Map* map, MainWindow* main_window, MapView* main_view, MapEditorController* editor, QWidget* parent)
: QWidget     { parent }
, task        { UNDEFINED_TASK }
, map         { map }
, map_printer { new MapPrinter(*map, main_view) }
, main_window { main_window }
, main_view   { main_view }
, editor      { editor }
, print_tool  { nullptr }
, active      { false }
{
	Q_ASSERT(main_window);
	
	layout = new QFormLayout();
	
	target_combo = new QComboBox();
	target_combo->setMinimumWidth(1); // Not zero, but not as long as the items
	layout->addRow(Util::Headline::create(tr("Printer:")), target_combo);
	
	if (PlatformPrinterProperties::dialogSupported())
	{
		printer_properties_button = new QToolButton();
		printer_properties_button->setText(tr("Properties"));
		layout->addRow(nullptr, printer_properties_button);
	}
	else 
	{
		printer_properties_button = nullptr;
	}
	
	paper_size_combo = new QComboBox();
	layout->addRow(tr("Page format:"), paper_size_combo);
	
	auto page_size_widget = new QWidget();
	auto page_size_layout = new QHBoxLayout();
	page_size_widget->setLayout(page_size_layout);
	page_size_layout->setMargin(0);
	page_width_edit = Util::SpinBox::create(1, 0.1, 1000.0, tr("mm"), 1.0);
	page_width_edit->setEnabled(false);
	page_size_layout->addWidget(page_width_edit, 1);
	page_size_layout->addWidget(new QLabel(QString::fromLatin1("x")), 0);
	page_height_edit = Util::SpinBox::create(1, 0.1, 1000.0, tr("mm"), 1.0);
	page_height_edit->setEnabled(false);
	page_size_layout->addWidget(page_height_edit, 1);
	layout->addRow({}, page_size_widget);
	
	page_orientation_widget = new QWidget();
	auto page_orientation_layout = new QHBoxLayout();
	page_orientation_layout->setContentsMargins(QMargins());
	page_orientation_widget->setLayout(page_orientation_layout);
	auto portrait_button = new QRadioButton(tr("Portrait"));
	page_orientation_layout->addWidget(portrait_button);
	auto landscape_button = new QRadioButton(tr("Landscape"));
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
	
	auto mode_widget = new QWidget();
	auto mode_layout = new QHBoxLayout();
	mode_widget->setLayout(mode_layout);
	mode_layout->setMargin(0);
	
	vector_mode_button = createPrintModeButton(QIcon(QString::fromLatin1(":/images/print-mode-vector.png")), tr("Vector\ngraphics"));
	raster_mode_button = createPrintModeButton(QIcon(QString::fromLatin1(":/images/print-mode-raster.png")), tr("Raster\ngraphics"));
	separations_mode_button = createPrintModeButton(QIcon(QString::fromLatin1(":/images/print-mode-separations.png")), tr("Color\nseparations"));
	vector_mode_button->setChecked(true);
	
	auto mode_button_group = new QButtonGroup(this);
	mode_button_group->addButton(vector_mode_button);
	mode_button_group->addButton(raster_mode_button);
	mode_button_group->addButton(separations_mode_button);
	
	mode_layout->addWidget(vector_mode_button);
	mode_layout->addWidget(raster_mode_button);
	mode_layout->addWidget(separations_mode_button);
	mode_layout->addStretch(1);
	
	layout->addRow(tr("Mode:"), mode_widget);
	
	color_mode_combo = new QComboBox();
	color_mode_combo->setEditable(false);
	color_mode_combo->addItem(tr("Default"), QVariant());
	color_mode_combo->addItem(tr("Device CMYK"), QVariant(true));
	layout->addRow(tr("Color mode:"), color_mode_combo);
	
	dpi_combo = new QComboBox();
	dpi_combo->setEditable(true);
	dpi_combo->setValidator(new QRegExpValidator(QRegExp(QLatin1String("^[1-9]\\d{0,4}$|^[1-9]\\d{0,4} ")+tr("dpi")+QLatin1Char('$')), dpi_combo));
	// TODO: Implement spinbox-style " dpi" suffix
	layout->addRow(tr("Resolution:"), dpi_combo);
	
	different_scale_check = new QCheckBox(tr("Print in different scale:"));
	// Limit the difference between nominal and printing scale in order to limit the number of page breaks.
	int min_scale = qMax(1, int(map->getScaleDenominator() / 10000) * 100);
	different_scale_edit = Util::SpinBox::create(min_scale, std::numeric_limits<int>::max(), {}, 500);
	different_scale_edit->setPrefix(QString::fromLatin1("1 : "));
	different_scale_edit->setEnabled(false);
	int different_scale_height = qMax(
	  different_scale_edit->minimumSizeHint().height(),
	  different_scale_check->minimumSizeHint().height() );
	different_scale_check->setMinimumHeight(different_scale_height);
	different_scale_edit->setMinimumHeight(different_scale_height);
	layout->addRow(different_scale_check, different_scale_edit);
	
	// this must be created before its value is used to determine the default setting of page_orientation_combo
	show_templates_check = new QCheckBox(tr("Show templates"));
	auto templates_warning_layout = new QHBoxLayout();
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

	world_file_check = new QCheckBox(tr("Save world file"));
	layout->addRow(world_file_check);
	world_file_check->hide();
	
	scrolling_content = new QWidget();
	scrolling_content->setLayout(layout);
	
	auto outer_layout = new QVBoxLayout();
	outer_layout->setContentsMargins(QMargins());
	
	scroll_area = new QScrollArea();
	scroll_area->setWidget(scrolling_content);
	scroll_area->setWidgetResizable(true);
	scroll_area->setMinimumWidth((scrolling_content->sizeHint() + scroll_area->verticalScrollBar()->sizeHint()).width());
	outer_layout->addWidget(scroll_area);
	
	button_box = new QDialogButtonBox();
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
	auto close_button = button_box->addButton(QDialogButtonBox::Close);
	outer_layout->addWidget(button_box);
	
	setLayout(outer_layout);
	
	connect(target_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PrintWidget::targetChanged);
	if (printer_properties_button)
		connect(printer_properties_button, &QAbstractButton::clicked, this, &PrintWidget::propertiesClicked, Qt::QueuedConnection);
	connect(paper_size_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PrintWidget::paperSizeChanged);
	connect(page_width_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PrintWidget::paperDimensionsChanged);
	connect(page_orientation_group, QOverload<int>::of(&QButtonGroup::buttonClicked), this, &PrintWidget::pageOrientationChanged);
	connect(page_height_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PrintWidget::paperDimensionsChanged);
	
	connect(top_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PrintWidget::printAreaMoved);
	connect(left_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PrintWidget::printAreaMoved);
	connect(width_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PrintWidget::printAreaResized);
	connect(height_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PrintWidget::printAreaResized);
	connect(overlap_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PrintWidget::overlapEdited);
	
	connect(mode_button_group, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked), this, &PrintWidget::printModeChanged);
	connect(dpi_combo->lineEdit(), &QLineEdit::textEdited, this, &PrintWidget::resolutionEdited);
	connect(dpi_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PrintWidget::resolutionEdited);
	connect(different_scale_check, &QAbstractButton::clicked, this, &PrintWidget::differentScaleClicked);
	connect(different_scale_edit, QOverload<int>::of(&QSpinBox::valueChanged), this, &PrintWidget::differentScaleEdited);
	connect(show_templates_check, &QAbstractButton::clicked, this, &PrintWidget::showTemplatesClicked);
	connect(show_grid_check, &QAbstractButton::clicked, this, &PrintWidget::showGridClicked);
	connect(overprinting_check, &QAbstractButton::clicked, this, &PrintWidget::overprintingClicked);
	connect(color_mode_combo, &QComboBox::currentTextChanged, this, &PrintWidget::colorModeChanged);
	
	connect(preview_button, &QAbstractButton::clicked, this, &PrintWidget::previewClicked);
	connect(print_button, &QAbstractButton::clicked, this, &PrintWidget::printClicked);
	connect(export_button, &QAbstractButton::clicked, this, &PrintWidget::printClicked);
	connect(close_button, &QAbstractButton::clicked, this, &PrintWidget::closeClicked);
	
	policy = map_printer->config().single_page_print_area ? SinglePage : CustomArea;
	policy_combo->setCurrentIndex(policy_combo->findData(policy));
	connect(policy_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PrintWidget::printAreaPolicyChanged);
	
	center_check->setChecked(map_printer->config().center_print_area);
	connect(center_check, &QAbstractButton::clicked, this, &PrintWidget::applyCenterPolicy);
	
	setPageFormat(map_printer->getPageFormat());
	connect(map_printer, &MapPrinter::pageFormatChanged, this, &PrintWidget::setPageFormat);
	
	connect(map_printer, &MapPrinter::optionsChanged, this, &PrintWidget::setOptions);
	spotColorPresenceChanged(map->hasSpotColors());
	connect(map, &Map::spotColorPresenceChanged, this, &PrintWidget::spotColorPresenceChanged);
	
	setPrintArea(map_printer->getPrintArea());
	connect(map_printer, &MapPrinter::printAreaChanged, this, &PrintWidget::setPrintArea);
	
	connect(map_printer, &MapPrinter::targetChanged, this, &PrintWidget::setTarget);
	
	connect(this, &PrintWidget::finished, this, &PrintWidget::savePrinterConfig);
}

PrintWidget::~PrintWidget()
{
	delete map_printer;
}

QSize PrintWidget::sizeHint() const
{
	QSize size = QWidget::sizeHint();
	size.setHeight(scrolling_content->sizeHint().height() +
	               2 * scroll_area->frameWidth() +
	               button_box->sizeHint().height() +
	               layout->horizontalSpacing() );
	return size;
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
				if (active)
					setOptions(map_printer->getOptions());
				emit taskChanged(tr("PDF export"));
				break;
				
			case EXPORT_IMAGE_TASK:
				map_printer->setTarget(MapPrinter::imageTarget());
				if (active)
					setOptions(map_printer->getOptions());
				policy = SinglePage;
				if (policy_combo->itemData(policy_combo->currentIndex()) != policy)
				{
					map_printer->setCustomPaperSize(map_printer->getPrintAreaPaperSize());
					policy_combo->setCurrentIndex(policy_combo->findData(policy));
				}
				emit taskChanged(tr("Image export"));
				break;
				
			default:
				emit taskChanged(QString{});
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
			main_view->save(writer, QLatin1String("saved_view"), false);
			
			editor->setViewOptionsEnabled(false);
			
			// Printers may have been added or removed.
			updateTargets();
			
			// Update the map view from the current options
			setOptions(map_printer->getOptions());
			connect(main_view, &MapView::visibilityChanged, this, &PrintWidget::onVisibilityChanged);
			
			// Set reasonable zoom.
			bool zoom_to_map = true;
			if (zoom_to_map)
			{
				// Ensure the visibility of the whole map.
				auto map_extent = map->calculateExtent(true, !main_view->areAllTemplatesHidden(), main_view);
				editor->getMainWidget()->ensureVisibilityOfRect(map_extent, MapWidget::ContinuousZoom);
			}
			else
			{
				// Ensure the visibility of the print area.
				auto print_area(map_printer->getPrintArea());
				editor->getMainWidget()->ensureVisibilityOfRect(print_area, MapWidget::ContinuousZoom);
			}
			
			// Activate PrintTool.
			if (!print_tool)
			{
				print_tool = new PrintTool(editor, map_printer);
			}
			editor->setOverrideTool(print_tool);
			editor->setEditingInProgress(true);
		}
		else
		{
			disconnect(main_view, &MapView::visibilityChanged, this, &PrintWidget::onVisibilityChanged);
			
			editor->setEditingInProgress(false);
			editor->setOverrideTool(nullptr);
			print_tool = nullptr;
			
			// Restore view
			QXmlStreamReader reader(saved_view_state);
			reader.readNextStartElement();
			main_view->load(reader);
			
			editor->setViewOptionsEnabled(true);
		}
	}
}



void PrintWidget::updateTargets()
{
	QVariant current_target = target_combo->itemData(target_combo->currentIndex());
	const auto* saved_printer = map_printer->getTarget();
	const QString saved_printer_name = saved_printer ? saved_printer->printerName() : QString{};
	int saved_target_index = -1;
	int default_printer_index = -1;
	{
		const QSignalBlocker block(target_combo);
		target_combo->clear();
		
		if (task == PRINT_TASK)
		{
			// Exporters
			target_combo->addItem(tr("Save to PDF"), QVariant(int(PdfExporter)));
			target_combo->insertSeparator(target_combo->count());
			target_combo->setCurrentIndex(0);
			
			// Printers
			auto default_printer_name = QPrinterInfo::defaultPrinterName();
			printers = QPrinterInfo::availablePrinterNames();
			for (int i = 0; i < printers.size(); ++i)
			{
				const QString& name = printers[i];
				if (name == saved_printer_name)
					saved_target_index = target_combo->count();
				if (name == default_printer_name)
					default_printer_index = target_combo->count();
				target_combo->addItem(name, i);
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
			if (target && printers[target_index] == target->printerName())
				break;
			else if (!target)
				break;
		}
	}
	target_combo->setCurrentIndex(target_combo->findData(QVariant(target_index)));
	
	updatePaperSizes(target);
	updateResolutions(target);
	
	bool supports_pages = (target != MapPrinter::imageTarget());
	bool supports_copies = (supports_pages && target && QPrinter(*target).supportsMultipleCopies());
	copies_edit->setEnabled(supports_copies);
	layout->labelForField(copies_edit)->setEnabled(supports_copies);
	
	bool is_printer = map_printer->isPrinter();
	print_button->setVisible(is_printer);
	print_button->setDefault(is_printer);
	export_button->setVisible(!is_printer);
	export_button->setDefault(!is_printer);
	if (printer_properties_button)
		printer_properties_button->setEnabled(is_printer);

	bool is_image_target = target == MapPrinter::imageTarget();
	vector_mode_button->setEnabled(!is_image_target);
	separations_mode_button->setEnabled(!is_image_target && map->hasSpotColors());
	if (is_image_target)
	{
		raster_mode_button->setChecked(true);
		printModeChanged(raster_mode_button);
	}
	
	world_file_check->setVisible(is_image_target);
	// If MapCoord (0,0) maps to projected (0,0), then there is probably
	// no point in writing a world file.
	world_file_check->setChecked(!map->getGeoreferencing().toProjectedCoords(MapCoordF{}).isNull());
	
	updateColorMode();
}

// slot
void PrintWidget::targetChanged(int index) const
{
	if (index < 0)
		return;
	
	int target_index = target_combo->itemData(index).toInt();
	Q_ASSERT(target_index >= -2);
	Q_ASSERT(target_index < printers.size());
	
	if (target_index == PdfExporter)
		map_printer->setTarget(MapPrinter::pdfTarget());
	else if (target_index == ImageExporter)
		map_printer->setTarget(MapPrinter::imageTarget());
	else
	{
		auto info = QPrinterInfo::printerInfo(printers[target_index]);
		map_printer->setTarget(&info);
	}
}

// slot
void PrintWidget::propertiesClicked()
{
	if (map_printer && map_printer->isPrinter())
	{
		std::shared_ptr<void> buffer; // must not be destroyed before printer.
		auto printer = map_printer->makePrinter();
		Q_ASSERT(printer->outputFormat() == QPrinter::NativeFormat);
		if (PlatformPrinterProperties::execDialog(printer.get(), buffer, this) == QDialog::Accepted)
			map_printer->takePrinterSettings(printer.get());
	}
}

void PrintWidget::updatePaperSizes(const QPrinterInfo* target) const
{
	QString prev_paper_size_name = paper_size_combo->currentText();
	bool have_custom_size = false;
	
	{
		const QSignalBlocker block(paper_size_combo);
		
		paper_size_combo->clear();
		QList<QPrinter::PaperSize> size_list;
		if (target)
			size_list = target->supportedPaperSizes();
		if (size_list.isEmpty())
			size_list = defaultPaperSizes();
		
		for (auto size : qAsConst(size_list))
		{
			if (size == QPrinter::Custom)
				have_custom_size = true; // add it once after all other entires
			else
				paper_size_combo->addItem(toString(size), size);
		}
		
		if (have_custom_size)
			paper_size_combo->addItem(toString(QPrinter::Custom), QPrinter::Custom);
	
		int paper_size_index = paper_size_combo->findData(map_printer->getPageFormat().paper_size);
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
	ScopedMultiSignalsBlocker block(
	            paper_size_combo, page_orientation_group,
	            page_width_edit, page_height_edit,
	            overlap_edit
	);
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
		QPrinter::PaperSize paper_size = QPrinter::PaperSize(paper_size_combo->itemData(index).toInt());
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
	policy = PrintAreaPolicy(policy_combo->itemData(index).toInt());
	applyPrintAreaPolicy();
}


// slot
void PrintWidget::applyPrintAreaPolicy() const
{
	if (policy == SinglePage)
	{
		setOverlapEditEnabled(false);
		auto print_area = map_printer->getPrintArea();
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
		auto print_area = map_printer->getPrintArea();
		centerOnMap(print_area);
		map_printer->setPrintArea(print_area);
	}
}

void PrintWidget::centerOnMap(QRectF& area) const
{
	auto map_extent = map->calculateExtent(false, show_templates_check->isChecked(), main_view);
	area.moveLeft(map_extent.center().x() - area.width() / 2);
	area.moveTop(map_extent.center().y() - area.height() / 2);
}



// slot
void PrintWidget::setPrintArea(const QRectF& area)
{
	ScopedMultiSignalsBlocker block(
	            left_edit, top_edit,
	            width_edit, height_edit
	);
	
	left_edit->setValue(area.left());
	top_edit->setValue(-area.top()); // Flip sign!
	width_edit->setValue(area.width());
	height_edit->setValue(area.height());
	
	if (center_check->isChecked())
	{
		auto centered_area = map_printer->getPrintArea();
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
				policy = CustomArea;
				policy_combo->setCurrentIndex(policy_combo->findData(policy));
				center_check->setChecked(false);
				setOverlapEditEnabled(true);
			}
		}
	}
}

// slot
void PrintWidget::printAreaMoved()
{
	auto area = map_printer->getPrintArea();
	area.moveLeft(left_edit->value());
	area.moveTop(-top_edit->value()); // Flip sign!
	map_printer->setPrintArea(area);
}

// slot
void PrintWidget::printAreaResized()
{
	auto area = map_printer->getPrintArea();
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
	
	ScopedMultiSignalsBlocker block(
	            dpi_combo->lineEdit(),
	            show_templates_check,
	            show_grid_check,
	            overprinting_check,
	            color_mode_combo,
	            vector_mode_button,
	            raster_mode_button,
	            separations_mode_button,
	            different_scale_check,
	            different_scale_edit
	);
	
	switch (options.mode)
	{
	default:
		qWarning("Unhandled MapPrinterMode");
		// fall through in release build
	case MapPrinterOptions::Vector:
		vector_mode_button->setChecked(true);
		setEnabledAndChecked(show_templates_check, options.show_templates);
		setEnabledAndChecked(show_grid_check,      options.show_grid);
		setDisabledAndChecked(overprinting_check,  options.simulate_overprinting);
		main_view->setAllTemplatesHidden(!options.show_templates);
		main_view->setGridVisible(options.show_grid);
		main_view->setOverprintingSimulationEnabled(false);
		break;
	case MapPrinterOptions::Raster:
		raster_mode_button->setChecked(true);
		setEnabledAndChecked(show_templates_check, options.show_templates);
		setEnabledAndChecked(show_grid_check,      options.show_grid);
		setEnabledAndChecked(overprinting_check,   options.simulate_overprinting);
		main_view->setAllTemplatesHidden(!options.show_templates);
		main_view->setGridVisible(options.show_grid);
		main_view->setOverprintingSimulationEnabled(options.simulate_overprinting);
		break;
	case MapPrinterOptions::Separations:
		separations_mode_button->setChecked(true);
		setDisabledAndChecked(show_templates_check, options.show_templates);
		setDisabledAndChecked(show_grid_check,      options.show_grid);
		setDisabledAndChecked(overprinting_check,   options.simulate_overprinting);
		main_view->setAllTemplatesHidden(true);
		main_view->setGridVisible(false);
		main_view->setOverprintingSimulationEnabled(true);
		break;
	}
	
	switch (options.color_mode)
	{
	default:
		qWarning("Unhandled ColorMode");
		// fall through in release build
	case MapPrinterOptions::DefaultColorMode:
		color_mode_combo->setCurrentIndex(0);
		break;
	case MapPrinterOptions::DeviceCmyk:
		color_mode_combo->setCurrentIndex(1);
		break;
	}
	
	checkTemplateConfiguration();
	updateColorMode();
	
	static QString dpi_template(QLatin1String("%1 ") + tr("dpi"));
	dpi_combo->setEditText(dpi_template.arg(options.resolution));
	
	if (options.scale != map->getScaleDenominator())
	{
		different_scale_check->setChecked(true);
		different_scale_edit->setEnabled(true);
	}
	
	auto scale = int(options.scale);
	different_scale_edit->setValue(scale);
	differentScaleEdited(scale);
	
	if (options.mode != MapPrinterOptions::Raster
	    && map_printer->engineWillRasterize())
	{
		QMessageBox::warning(this, tr("Error"),
		                     tr("The map contains transparent elements"
		                        " which require the raster mode."));
		map_printer->setMode(MapPrinterOptions::Raster);
	}
}

void PrintWidget::onVisibilityChanged()
{
	map_printer->setPrintTemplates(!main_view->areAllTemplatesHidden());
	map_printer->setPrintGrid(main_view->isGridVisible());
	map_printer->setSimulateOverprinting(main_view->isOverprintingSimulationEnabled());
}

void PrintWidget::updateResolutions(const QPrinterInfo* target) const
{
	static const QList<int> default_resolutions(QList<int>() << 150 << 300 << 600 << 1200);
	
	// Numeric resolution list
	QList<int> supported_resolutions;
	if (target)
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
	static QString dpi_template(QLatin1String("%1 ") + tr("dpi"));
	QStringList resolutions;
	resolutions.reserve(supported_resolutions.size());
	for (auto resolution : qAsConst(supported_resolutions))
		resolutions << dpi_template.arg(resolution);
	
	QString dpi_text = dpi_combo->currentText();
	{
		const QSignalBlocker block(dpi_combo);
		dpi_combo->clear();
		dpi_combo->addItems(resolutions);
	}
	dpi_combo->lineEdit()->setText(dpi_text.isEmpty() ? dpi_template.arg(600) : dpi_text);
}

void PrintWidget::updateColorMode()
{
	bool enable = map_printer->getTarget() == MapPrinter::pdfTarget()
	              && !raster_mode_button->isChecked();
	color_mode_combo->setEnabled(enable);
	layout->labelForField(color_mode_combo)->setEnabled(enable);
	if (!enable)
		color_mode_combo->setCurrentIndex(0);
}

// slot
void PrintWidget::resolutionEdited()
{
	auto resolution_text = dpi_combo->currentText();
	auto index_of_space = resolution_text.indexOf(QLatin1Char(' '));
	auto dpi_value = resolution_text.leftRef(index_of_space).toInt();
	if (dpi_value > 0)
	{
		auto pos = dpi_combo->lineEdit()->cursorPosition();
		map_printer->setResolution(dpi_value);
		dpi_combo->lineEdit()->setCursorPosition(pos);
	}
}

// slot
void PrintWidget::differentScaleClicked(bool checked)
{
	if (!checked)
		different_scale_edit->setValue(int(map->getScaleDenominator()));
	
	different_scale_edit->setEnabled(checked);
}

// slot
void PrintWidget::differentScaleEdited(int value)
{
	map_printer->setScale(static_cast<unsigned int>(value));
	applyPrintAreaPolicy();
	
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
	map_printer->setPrintTemplates(checked);
	checkTemplateConfiguration();
}

void PrintWidget::checkTemplateConfiguration()
{
	bool visibility = map_printer->engineMayRasterize() && show_templates_check->isChecked();
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

void PrintWidget::colorModeChanged()
{
	if (color_mode_combo->currentData().toBool())
		map_printer->setColorMode(MapPrinterOptions::DeviceCmyk);
	else
		map_printer->setColorMode(MapPrinterOptions::DefaultColorMode);
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
	
	auto printer = map_printer->makePrinter();
	if (!printer)
	{
		QMessageBox::warning(this, tr("Error"), tr("Failed to prepare the preview."));
		return;
	}
	
	printer->setCreator(main_window->appName());
	printer->setDocName(QFileInfo(main_window->currentPath()).baseName());
	
	QPrintPreviewDialog preview(printer.get(), editor->getWindow());
	preview.setWindowModality(Qt::ApplicationModal); // Required for OSX, cf. QTBUG-40112
	
	PrintProgressDialog progress(map_printer, editor->getWindow());
	progress.setWindowTitle(tr("Print Preview Progress"));
	connect(&preview, &QPrintPreviewDialog::paintRequested, &progress, &PrintProgressDialog::paintRequested);
	// Doesn't work as expected, on OSX at least.
	//connect(&progress, &QProgressDialog::canceled, &preview, &QPrintPreviewDialog::reject);
	
	preview.exec();
#endif
}

// slot
void PrintWidget::printClicked()
{
	if (checkForEmptyMap())
		return;
	
	if (map->isAreaHatchingEnabled() || map->isBaselineViewEnabled())
	{
		if (QMessageBox::question(this, tr("Warning"), 
		                          tr("A non-standard view mode is activated. "
		                             "Are you sure to print / export the map like this?"),
		                          QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Cancel)
			return;
	}
	
	if (map_printer->getTarget() == MapPrinter::imageTarget())
		exportToImage();
	else if (map_printer->getTarget() == MapPrinter::pdfTarget())
		exportToPdf();
	else
		print();
}

void PrintWidget::exportToImage()
{
	static const QString filter_template(QString::fromLatin1("%1 (%2)"));
	QStringList filters = { filter_template.arg(tr("PNG"), QString::fromLatin1("*.png")),
	                        filter_template.arg(tr("BMP"), QString::fromLatin1("*.bmp")),
	                        filter_template.arg(tr("TIFF"), QString::fromLatin1("*.tif *.tiff")),
	                        filter_template.arg(tr("JPEG"), QString::fromLatin1("*.jpg *.jpeg")),
	                        tr("All files (*.*)") };
	QString path = FileDialog::getSaveFileName(this, tr("Export map ..."), {}, filters.join(QString::fromLatin1(";;")));
	if (path.isEmpty())
		return;
	
	if (!path.endsWith(QLatin1String(".png"), Qt::CaseInsensitive)
	    && !path.endsWith(QLatin1String(".bmp"), Qt::CaseInsensitive)
	    && !path.endsWith(QLatin1String(".tif"), Qt::CaseInsensitive) && !path.endsWith(QLatin1String(".tiff"), Qt::CaseInsensitive)
	    && !path.endsWith(QLatin1String(".jpg"), Qt::CaseInsensitive) && !path.endsWith(QLatin1String(".jpeg"), Qt::CaseInsensitive) )
	{
		path.append(QString::fromLatin1(".png"));
	}
	
	qreal pixel_per_mm = map_printer->getOptions().resolution / 25.4;
	int print_width = qRound(map_printer->getPrintAreaPaperSize().width() * pixel_per_mm);
	int print_height = qRound(map_printer->getPrintAreaPaperSize().height() * pixel_per_mm);
	QImage image(print_width, print_height, QImage::Format_ARGB32_Premultiplied);
	if (image.isNull())
	{
		QMessageBox::warning(this, tr("Error"), tr("Failed to prepare the image. Not enough memory."));
		return;
	}
	
	int dots_per_meter = qRound(pixel_per_mm * 1000);
	image.setDotsPerMeterX(dots_per_meter);
	image.setDotsPerMeterY(dots_per_meter);
	
	image.fill(QColor(Qt::white));
	
#if 0  // Pointless unless drawPage drives the event loop and sends progress
	PrintProgressDialog progress(map_printer, main_window);
	progress.setWindowTitle(tr("Export map ..."));
#endif
	
	// Export the map
	QPainter p(&image);
	map_printer->drawPage(&p, map_printer->getPrintArea(), &image);
	p.end();
	if (!image.save(path))
	{
		QMessageBox::warning(this, tr("Error"), tr("Failed to save the image. Does the path exist? Do you have sufficient rights?"));
	}
	else
	{
		main_window->showStatusBarMessage(tr("Exported successfully to %1").arg(path), 4000);
		if (world_file_check->isChecked())
			exportWorldFile(path);  /// \todo Handle errors
		emit finished(0);
	}
	return;
}

void PrintWidget::exportWorldFile(const QString& path) const
{
	const auto& georef = map->getGeoreferencing();
	const auto& ref_transform = georef.mapToProjected();

	qreal pixel_per_mm = (map_printer->getOptions().resolution / 25.4) * map_printer->getScaleAdjustment();
	const auto center_of_pixel = QPointF{0.5/pixel_per_mm, 0.5/pixel_per_mm};
	const auto top_left = georef.toProjectedCoords(MapCoord{map_printer->getPrintArea().topLeft() + center_of_pixel});


	const auto xscale = ref_transform.m11() / pixel_per_mm;
	const auto yscale = ref_transform.m22() / pixel_per_mm;
	const auto xskew  = ref_transform.m12() / pixel_per_mm;
	const auto yskew  = ref_transform.m21() / pixel_per_mm;

	QTransform final_wld(xscale, yskew, 0, xskew, yscale, 0, top_left.x(), top_left.y());
	WorldFile wld(final_wld);
	wld.save(WorldFile::pathForImage(path));
}

void PrintWidget::exportToPdf()
{
	auto printer = map_printer->makePrinter();
	if (!printer)
	{
		QMessageBox::warning(this, tr("Error"), tr("Failed to prepare the PDF export."));
		return;
	}
	
	printer->setOutputFormat(QPrinter::PdfFormat);
	printer->setNumCopies(copies_edit->value());
	printer->setCreator(main_window->appName());
	printer->setDocName(QFileInfo(main_window->currentPath()).baseName());
	
	static const QString filter_template(QString::fromLatin1("%1 (%2)"));
	QStringList filters = { filter_template.arg(tr("PDF"), QString::fromLatin1("*.pdf")),
	                        tr("All files (*.*)") };
	QString path = FileDialog::getSaveFileName(this, tr("Export map ..."), {}, filters.join(QString::fromLatin1(";;")));
	if (path.isEmpty())
	{
		return;
	}
	else if (!path.endsWith(QLatin1String(".pdf"), Qt::CaseInsensitive))
	{
		path.append(QLatin1String(".pdf"));
	}
	printer->setOutputFileName(path);
	
	PrintProgressDialog progress(map_printer, main_window);
	progress.setWindowTitle(tr("Export map ..."));
	
	// Export the map
	if (!map_printer->printMap(printer.get()))
	{
		QFile(path).remove();
		QMessageBox::warning(this, tr("Error"), tr("Failed to finish the PDF export."));
	}
	else if (!progress.wasCanceled())
	{
		main_window->showStatusBarMessage(tr("Exported successfully to %1").arg(path), 4000);
		emit finished(0);
	}
	else
	{
		QFile(path).remove();
		main_window->showStatusBarMessage(tr("Canceled."), 4000);
	}
}

void PrintWidget::print()
{
	auto printer = map_printer->makePrinter();
	if (!printer)
	{
		QMessageBox::warning(this, tr("Error"), tr("Failed to prepare the printing."));
		return;
	}
	
	printer->setNumCopies(copies_edit->value());
	printer->setCreator(main_window->appName());
	printer->setDocName(QFileInfo(main_window->currentPath()).baseName());
	
	PrintProgressDialog progress(map_printer, main_window);
	progress.setWindowTitle(tr("Printing Progress"));
	
	// Print the map
	if (!map_printer->printMap(printer.get()))
	{
		QMessageBox::warning(main_window, tr("Error"), tr("An error occurred during printing."));
	}
	else if (!progress.wasCanceled())
	{
		main_window->showStatusBarMessage(tr("Successfully created print job"), 4000);
		emit finished(0);
	}
	else if (printer->abort())
	{
		main_window->showStatusBarMessage(tr("Canceled."), 4000);
	}
	else
	{
		QMessageBox::warning(main_window, tr("Error"), tr("The print job could not be stopped."));
	}
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


}  // namespace OpenOrienteering

#endif  // QT_PRINTSUPPORT_LIB
