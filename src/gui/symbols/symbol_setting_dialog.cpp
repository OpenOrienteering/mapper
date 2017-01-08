/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2016 Kai Pastor
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


#include "symbol_setting_dialog.h"

#include <QColorDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QSplitter>
#include <QToolButton>
#include <QVBoxLayout>

#include "gui/widgets/template_list_widget.h"
#include "core/map.h"
#include "core/objects/object.h"
#include "core/objects/text_object.h"
#include "core/symbols/symbol.h"
#include "core/symbols/point_symbol.h"
#include "core/symbols/line_symbol.h"
#include "core/symbols/area_symbol.h"
#include "core/symbols/text_symbol.h"
#include "gui/main_window.h"
#include "gui/map/map_editor.h"
#include "gui/map/map_widget.h"
#include "templates/template.h"
#include "templates/template_image.h"
#include "point_symbol_editor_widget.h"
#include "core/symbols/combined_symbol.h"
#include "symbol_properties_widget.h"
#include "util/util.h"

SymbolSettingDialog::SymbolSettingDialog(Symbol* source_symbol, Map* source_map, QWidget* parent)
: QDialog(parent, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowMaximizeButtonHint),
  source_map(source_map),
  source_symbol(source_symbol),
  source_symbol_copy(source_symbol->duplicate())	// don't rely on external entity
{
	setWindowTitle(tr("Symbol settings"));
	setSizeGripEnabled(true);
	symbol_modified = false;
	
	symbol = source_symbol->duplicate();
	symbol->setHidden(false);
	
	symbol_icon_label = new QLabel();
	symbol_icon_label->setPixmap(QPixmap::fromImage(symbol->getIcon(source_map)));
	
	symbol_text_label = new QLabel();
	updateSymbolLabel();
	
	QDialogButtonBox* button_box = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok | QDialogButtonBox::Reset | QDialogButtonBox::Help);
	ok_button = button_box->button(QDialogButtonBox::Ok);
	reset_button = button_box->button(QDialogButtonBox::Reset);
	connect(button_box, SIGNAL(rejected()), this, SLOT(reject()));
	connect(button_box, SIGNAL(accepted()), this, SLOT(accept()));
	connect(reset_button, SIGNAL(clicked(bool)), this, SLOT(reset()));
	connect(button_box->button(QDialogButtonBox::Help), SIGNAL(clicked(bool)), this, SLOT(showHelp()));
	
	preview_map = new Map();
	preview_map->useColorsFrom(source_map);
	preview_map->setScaleDenominator(source_map->getScaleDenominator());
	
	createPreviewMap();
	
	preview_widget = new MainWindow(false);
	preview_controller = new MapEditorController(MapEditorController::SymbolEditor, preview_map);
	preview_widget->setController(preview_controller);
	preview_map_view = preview_controller->getMainWidget()->getMapView();
	float zoom_factor = 1;
	if (symbol->getType() == Symbol::Point)
		zoom_factor = 8;
	else if (symbol->getType() == Symbol::Line || symbol->getType() == Symbol::Combined)
		zoom_factor = 2;
	preview_map_view->setZoom(zoom_factor * preview_map_view->getZoom());
	
	properties_widget = symbol->createPropertiesWidget(this);
	
	QVBoxLayout* preview_layout = NULL;
	if (symbol->getType() == Symbol::Point)
	{
		Q_UNUSED(QT_TR_NOOP("Template:")) /// \todo Switch the following line to Util::Headline::create
		QLabel* template_label = new QLabel(tr("<b>Template:</b> "));
		template_file_label = new QLabel(tr("(none)"));
		QPushButton* load_template_button = new QPushButton(tr("Open..."));
		
		center_template_button = new QToolButton();
		center_template_button->setText(tr("Center template..."));
		center_template_button->setToolButtonStyle(Qt::ToolButtonTextOnly);
		center_template_button->setPopupMode(QToolButton::InstantPopup);
		center_template_button->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
		QMenu* center_template_menu = new QMenu(center_template_button);
		center_template_menu->addAction(tr("bounding box on origin"), this, SLOT(centerTemplateBBox()));
		center_template_menu->addAction(tr("center of gravity on origin"), this, SLOT(centerTemplateGravity()));
		center_template_button->setMenu(center_template_menu);
		
		center_template_button->setEnabled(false);
		
		QHBoxLayout* template_layout = new QHBoxLayout();
		template_layout->addWidget(template_label);
		template_layout->addWidget(template_file_label, 1);
		template_layout->addWidget(load_template_button);
		template_layout->addWidget(center_template_button);
		
		preview_layout = new QVBoxLayout();
		preview_layout->setContentsMargins(0, 0, 0, 0);
		preview_layout->addLayout(template_layout);
		preview_layout->addWidget(preview_widget);
		
		connect(load_template_button, SIGNAL(clicked(bool)), this, SLOT(loadTemplateClicked()));
	}
	else
	{
		template_file_label = NULL;
		center_template_button = NULL;
	}
	
	QGridLayout* left_layout = new QGridLayout();
	left_layout->addWidget(symbol_icon_label, 0, 0);
	left_layout->addWidget(symbol_text_label, 0, 1);
	left_layout->addWidget(properties_widget, 1, 0, 1, 2);
	left_layout->addWidget(button_box,        2, 0, 1, 2);
	left_layout->setColumnStretch(1, 1);
	
	QWidget* left = new QWidget();
	left->setLayout(left_layout);
	left->layout();
	
	QWidget* right = preview_widget;
	if (preview_layout != NULL)
	{
		right = new QWidget();
		right->setLayout(preview_layout);
	}
	else
		right->setMinimumWidth(300);
	
	QSplitter* splitter = new QSplitter();
	splitter->addWidget(left);
	splitter->setCollapsible(0, false);
	splitter->addWidget(right);
	splitter->setCollapsible(1, true);
	
	QBoxLayout* layout = new QHBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(splitter);
	setLayout(layout);
	
	updateButtons();
}

SymbolSettingDialog::~SymbolSettingDialog()
{
	delete properties_widget; // must be deleted before the symbol!
	delete symbol;
	delete source_symbol_copy;
}

Symbol* SymbolSettingDialog::getNewSymbol() const
{
	Symbol* clone = symbol->duplicate();
	clone->setHidden(source_symbol_copy->isHidden());
	return clone;
}

void SymbolSettingDialog::updatePreview()
{
	symbol_icon_label->setPixmap(QPixmap::fromImage(symbol->getIcon(source_map, true)));
	preview_map->updateAllObjects();
}

void SymbolSettingDialog::loadTemplateClicked()
{
	auto new_template = TemplateListWidget::showOpenTemplateDialog(this, preview_controller);
	if (new_template)
	{
		if (preview_map->getNumTemplates() > 0)
		{
			// Delete old template
			preview_map->setTemplateAreaDirty(0);
			preview_map->deleteTemplate(0);
		}
		
		preview_map->setFirstFrontTemplate(1);
		
		auto temp = new_template.release(); // avoid double release after addTemplate
		preview_map->addTemplate(temp, 0);
		preview_map_view->setTemplateVisibility(temp, { 1, true });
		preview_map->setTemplateAreaDirty(0);
		
		template_file_label->setText(temp->getTemplateFilename());
		center_template_button->setEnabled(qstrcmp(temp->getTemplateType(), "TemplateImage") == 0);
	}
}

void SymbolSettingDialog::centerTemplateBBox()
{
	Q_ASSERT(preview_map->getNumTemplates() == 1);
	Template* temp = preview_map->getTemplate(0);
	
	preview_map->setTemplateAreaDirty(0);
	temp->setTemplatePosition(temp->templatePosition() - MapCoord { temp->calculateTemplateBoundingBox().center() } );
	preview_map->setTemplateAreaDirty(0);
}

void SymbolSettingDialog::centerTemplateGravity()
{
	Q_ASSERT(preview_map->getNumTemplates() == 1);
	Template* temp = preview_map->getTemplate(0);
	TemplateImage* image = reinterpret_cast<TemplateImage*>(temp);
	
	QColor background_color = QColorDialog::getColor(Qt::white, this, tr("Select background color"));
	if (!background_color.isValid())
		return;
	
	auto map_center_current = MapCoord { temp->templateToMap(image->calcCenterOfGravity(background_color.rgb())) };
	
	preview_map->setTemplateAreaDirty(0);
	temp->setTemplatePosition(temp->templatePosition() - map_center_current);
	preview_map->setTemplateAreaDirty(0);
}

void SymbolSettingDialog::createPreviewMap()
{
	symbol_icon_label->setPixmap(QPixmap::fromImage(symbol->getIcon(source_map)));
	
	for (int i = 0; i < (int)preview_objects.size(); ++i)
		preview_map->deleteObject(preview_objects[i], false);
	
	preview_objects.clear();
	
	if (symbol->getType() == Symbol::Line)
	{
		LineSymbol* line = reinterpret_cast<LineSymbol*>(symbol);
		
		const int num_lines = 15;
		const float min_length = 0.5f;
		const float max_length = 15;
		const float x_offset = -0.5f * max_length;
		const float y_offset = (0.001f * qMax(600, line->getLineWidth())) * 3.5f;
		
		float y_start = 0 - (y_offset * (num_lines - 1));
		
		for (int i = 0; i < num_lines; ++i)
		{
			float length = min_length + (i / (float)(num_lines - 1)) * (max_length - min_length);
			float y = y_start + i * y_offset;
			
			PathObject* path = new PathObject(line);
			path->addCoordinate(0, { x_offset - length, y });
			path->addCoordinate(1, { x_offset, y });
			preview_map->addObject(path);
			
			preview_objects.push_back(path);
		}
		
		const int num_circular_lines = 12;
		const auto inner_radius = 4;
		auto center = MapCoordF { 2*inner_radius + 0.5 * max_length, 0.5f * y_start };
		
		for (int i = 0; i < num_circular_lines; ++i)
		{
			auto angle = M_PI * 2 * i / num_circular_lines;
			auto direction = MapCoordF::fromPolar(1.0, angle);
			float length = min_length + (max_length - min_length) * i / (num_circular_lines - 1);
			
			PathObject* path = new PathObject(line);
			path->addCoordinate(0, MapCoord(center + direction * inner_radius));
			path->addCoordinate(1, MapCoord(center + direction * (inner_radius + length)));
			preview_map->addObject(path);
			
			preview_objects.push_back(path);
		}
		
		const float snake_min_x = -1.5f * max_length;
		const float snake_max_x = 1.5f * max_length;
		const float snake_max_y = qMin(0.5f * y_start - inner_radius - max_length, y_start) - 4;
		const float snake_min_y = snake_max_y - 6;
		const int snake_steps = 8;
		
		PathObject* path = new PathObject(line);
		for (int i = 0; i < snake_steps; ++i)
		{
			MapCoord coord(snake_min_x + (i / (float)(snake_steps-1)) * (snake_max_x - snake_min_x), snake_min_y + (i % 2) * (snake_max_y - snake_min_y));
			coord.setDashPoint(true);
			path->addCoordinate(i, coord);
		}
		preview_map->addObject(path);
		preview_objects.push_back(path);
		
		const float curve_min_x = snake_min_x;
		const float curve_max_x = snake_max_x;
		const float curve_max_y = snake_min_y - 4;
		const float curve_min_y = curve_max_y - 6;
		
		path = new PathObject(line);
		MapCoord coord = MapCoord(curve_min_x, curve_min_y);
		coord.setCurveStart(true);
		path->addCoordinate(coord);
		coord = MapCoord(curve_min_x + (curve_max_x - curve_min_x) / 6, curve_max_y);
		path->addCoordinate(coord);
		coord = MapCoord(curve_min_x + 2 * (curve_max_x - curve_min_x) / 6, curve_max_y);
		path->addCoordinate(coord);
		coord = MapCoord(curve_min_x + 3 * (curve_max_x - curve_min_x) / 6, 0.5f * (curve_min_y + curve_max_y));
		coord.setCurveStart(true);
		path->addCoordinate(coord);
		coord = MapCoord(curve_min_x + 4 * (curve_max_x - curve_min_x) / 6, curve_min_y);
		path->addCoordinate(coord);
		coord = MapCoord(curve_min_x + 5 * (curve_max_x - curve_min_x) / 6, curve_min_y);
		path->addCoordinate(coord);
		coord = MapCoord(curve_max_x, curve_max_y);
		path->addCoordinate(coord);
		preview_map->addObject(path);
		preview_objects.push_back(path);
		
		// Debug objects
		
		/*
		// Closed path, rectangular
		PathObject* path;
		MapCoord coord;
		
		path = new PathObject(preview_map, line);
		coord = MapCoord(-20, -10);
		//coord.setCurveStart(true);
		path->addCoordinate(coord);
		coord = MapCoord(20, -10);
		path->addCoordinate(coord);
		coord = MapCoord(20, 10);
		path->addCoordinate(coord);
		coord = MapCoord(-20, 10);
		path->addCoordinate(coord);
		path->setPathClosed(true);
		preview_map->addObject(path);
		preview_objects.push_back(path);*/
		
		// Closed path, curve
		/*PathObject* path;
		MapCoord coord;
		
		path = new PathObject(preview_map, line);
		path->setPathClosed(true);
		coord = MapCoord(-20, 10);
		coord.setCurveStart(true);
		path->addCoordinate(coord);
		coord = MapCoord(-10, 0);
		path->addCoordinate(coord);
		coord = MapCoord(-10, 10);
		path->addCoordinate(coord);
		
		preview_map->addObject(path);
		preview_objects.push_back(path);*/
		
		// Path with holes
		/*PathObject* path;
		MapCoord coord;
		
		path = new PathObject(preview_map, line);
		coord = MapCoord(-20, 10);
		coord.setCurveStart(true);
		path->addCoordinate(coord);
		coord = MapCoord(-10, 0);
		path->addCoordinate(coord);
		coord = MapCoord(0, -10);
		path->addCoordinate(coord);
		coord = MapCoord(10, 10);
		path->addCoordinate(coord);
		coord = MapCoord(20, 10);
		coord.setHolePoint(true);
		path->addCoordinate(coord);
		coord = MapCoord(30, 10);
		path->addCoordinate(coord);
		coord = MapCoord(40, 10);
		path->addCoordinate(coord);

		preview_map->addObject(path);
		preview_objects.push_back(path);*/
	}
	else if (symbol->getType() == Symbol::Area)
	{
		const float half_radius = 8;
		const float offset_y = 0;
		
		PathObject* path = new PathObject(symbol);
		path->addCoordinate(0, { -half_radius, -half_radius + offset_y });
		path->addCoordinate(1, { half_radius,  -half_radius + offset_y });
		path->addCoordinate(2, { half_radius,  half_radius + offset_y });
		path->addCoordinate(3, { -half_radius, half_radius + offset_y });
		preview_map->addObject(path);
		
		preview_objects.push_back(path);
	}
	else if (symbol->getType() == Symbol::Text)
	{
		TextSymbol* text_symbol = reinterpret_cast<TextSymbol*>(symbol);
		
		const QString string = tr("The quick brown fox\ntakes the routechoice\nto jump over the lazy dog\n1234567890");
		
		TextObject* object = new TextObject(text_symbol);
		object->setAnchorPosition(0, 0);
		object->setText(string);
		object->setHorizontalAlignment(TextObject::AlignHCenter);
		object->setVerticalAlignment(TextObject::AlignVCenter);
		preview_map->addObject(object);
		
		preview_objects.push_back(object);
	}
	else if (symbol->getType() == Symbol::Combined)
	{
		const float radius = 5;
		
		PathObject* path = new PathObject(symbol);
		for (int i = 0; i < 5; ++i)
			path->addCoordinate(i, MapCoord(sin(2*M_PI * i/5.0) * radius, -cos(2*M_PI * i/5.0) * radius));
		path->parts().front().setClosed(true, false);
		preview_map->addObject(path);
		
		preview_objects.push_back(path);
	}
}

void SymbolSettingDialog::showHelp()
{
	QByteArray fragment;
	fragment.reserve(100);
	fragment = "editor";
	if (properties_widget->currentIndex() > 0)
	{
		fragment = "symbol-type-";
		fragment.append(QByteArray::number(symbol->getType()));
	}
	Util::showHelp(preview_controller->getWindow(), "symbol_dock_widget.html", fragment);
}

void SymbolSettingDialog::reset()
{
	Symbol* old_symbol = symbol;
	symbol = source_symbol_copy->duplicate();
	symbol->setHidden(false);
	createPreviewMap();
	properties_widget->reset(symbol);
	delete old_symbol;
	
	setSymbolModified(false);
}

void SymbolSettingDialog::setSymbolModified(bool modified)
{
	symbol_modified = modified;
	updateSymbolLabel();
	updatePreview();
	updateButtons();
}

void SymbolSettingDialog::updateSymbolLabel()
{
	QString number = symbol->getNumberAsString();
	if (number.isEmpty())
		number = QString::fromLatin1("???");
	QString name = symbol->getName();
	if (name.isEmpty())
		name = tr("- unnamed -");
	symbol_text_label->setText(QLatin1String("<b>") + number + QLatin1String("  ") + name + QLatin1String("</b>"));
}

void SymbolSettingDialog::updateButtons()
{
	ok_button->setEnabled(symbol_modified && symbol->getNumberComponent(0)>=0 && !symbol->getName().isEmpty());
	reset_button->setEnabled(symbol_modified);
}
