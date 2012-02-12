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


#include "symbol_setting_dialog.h"

#include <QtGui>

#include "map.h"
#include "object.h"
#include "symbol.h"
#include "symbol_point.h"
#include "symbol_line.h"
#include "symbol_area.h"
#include "symbol_text.h"
#include "main_window.h"
#include "map_editor.h"
#include "map_widget.h"
#include "template.h"
#include "template_image.h"
#include "template_dock_widget.h"
#include "symbol_point_editor.h"
#include "symbol_combined.h"

SymbolSettingDialog::SymbolSettingDialog(Symbol* symbol, Symbol* in_map_symbol, Map* map, QWidget* parent) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
{
	setSizeGripEnabled(true);
	this->symbol = symbol;
	
	QGroupBox* general_group = new QGroupBox(tr("General"));
	
	QLabel* number_label = new QLabel(tr("Number:"));
	number_edit = new QLineEdit*[Symbol::number_components];
	for (int i = 0; i < Symbol::number_components; ++i)
	{
		number_edit[i] = new QLineEdit((symbol->getNumberComponent(i) < 0) ? "" : QString::number(symbol->getNumberComponent(i)));
		number_edit[i]->setMaximumWidth(60);
		number_edit[i]->setValidator(new QIntValidator(0, 99999, number_edit[i]));
	}
	QLabel* name_label = new QLabel(tr("Name:"));
	name_edit = new QLineEdit(symbol->getName());
	QLabel* description_label = new QLabel(tr("Description:"));
	description_edit = new QTextEdit(symbol->getDescription());
	helper_symbol_check = new QCheckBox(tr("Helper symbol (not shown in finished map)"));
	helper_symbol_check->setChecked(symbol->isHelperSymbol());
	
	QPushButton* cancel_button = new QPushButton(tr("Cancel"));
	ok_button = new QPushButton(QIcon(":/images/arrow-right.png"), tr("Ok"));
	ok_button->setDefault(true);
	
	preview_map = new Map();
	preview_map->useColorsFrom(map);
	preview_map->setScaleDenominator(map->getScaleDenominator());
	
	createPreviewMap();
	
	preview_widget = new MainWindow(false);
	MapEditorController* controller = new MapEditorController(MapEditorController::SymbolEditor, preview_map);
	preview_widget->setController(controller);
	preview_map_view = controller->getMainWidget()->getMapView();
	float zoom_factor = 1;
	if (symbol->getType() == Symbol::Point)
		zoom_factor = 8;
	else if (symbol->getType() == Symbol::Line || symbol->getType() == Symbol::Combined)
		zoom_factor = 2;
	preview_map_view->setZoom(zoom_factor * preview_map_view->getZoom());
	
	PointSymbolEditorWidget* point_symbol_editor = createPointSymbolEditor(controller);
	
	QWidget* type_specific_settings;
	Symbol::Type type = symbol->getType();
	if (type == Symbol::Point)
		type_specific_settings = new PointSymbolSettings(reinterpret_cast<PointSymbol*>(symbol), map, this);
	else if (type == Symbol::Line)
		type_specific_settings = new LineSymbolSettings(reinterpret_cast<LineSymbol*>(symbol), map, point_symbol_editor, this);
	else if (type == Symbol::Area)
		type_specific_settings = new AreaSymbolSettings(reinterpret_cast<AreaSymbol*>(symbol), map, this, point_symbol_editor);
	else if (type == Symbol::Text)
		type_specific_settings = new TextSymbolSettings(reinterpret_cast<TextSymbol*>(symbol), map, this);
	else if (type == Symbol::Combined)
		type_specific_settings = new CombinedSymbolSettings(reinterpret_cast<CombinedSymbol*>(symbol), reinterpret_cast<CombinedSymbol*>(in_map_symbol), map, this);
	else
		assert(false);
	
	QVBoxLayout* middle_layout = NULL;
	if (symbol->getType() == Symbol::Point)
	{
		QLabel* template_label = new QLabel(tr("<b>Template</b>: "));
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
		
		middle_layout = new QVBoxLayout();
		middle_layout->addWidget(preview_widget);
		middle_layout->addLayout(template_layout);
		
		connect(load_template_button, SIGNAL(clicked(bool)), this, SLOT(loadTemplateClicked()));
	}
	else
	{
		template_file_label = NULL;
		center_template_button = NULL;
	}
	
	QHBoxLayout* number_layout = new QHBoxLayout();
	number_layout->addWidget(number_label);
	for (int i = 0; i < Symbol::number_components; ++i)
	{
		number_layout->addWidget(number_edit[i]);
		if (i < Symbol::number_components - 1)
			number_layout->addWidget(new QLabel("."));
	}
	number_layout->addStretch(1);
	
	QHBoxLayout* name_layout = new QHBoxLayout();
	name_layout->addWidget(name_label);
	name_layout->addWidget(name_edit, 1);
	
	QHBoxLayout* buttons_layout = new QHBoxLayout();
	buttons_layout->addWidget(cancel_button);
	buttons_layout->addStretch(1);
	buttons_layout->addWidget(ok_button);
	
	QVBoxLayout* general_layout = new QVBoxLayout();
	general_layout->addLayout(number_layout);
	general_layout->addLayout(name_layout);
	general_layout->addWidget(description_label);
	general_layout->addWidget(description_edit);
	general_layout->addWidget(helper_symbol_check);
	general_group->setLayout(general_layout);
	
	QVBoxLayout* left_layout = new QVBoxLayout();
	left_layout->addWidget(general_group);
	left_layout->addWidget(type_specific_settings);
	left_layout->addLayout(buttons_layout);
	
	QHBoxLayout* layout = new QHBoxLayout();
	layout->addLayout(left_layout);
	if (middle_layout)
		layout->addLayout(middle_layout);
	else
		layout->addWidget(preview_widget, 1);
	if (point_symbol_editor)
		layout->addWidget(point_symbol_editor);
	setLayout(layout);
	
	for (int i = 0; i < Symbol::number_components; ++i)
		connect(number_edit[i], SIGNAL(textEdited(QString)), this, SLOT(numberChanged(QString)));
	connect(name_edit, SIGNAL(textEdited(QString)), this, SLOT(nameChanged(QString)));
	connect(description_edit, SIGNAL(textChanged()), this, SLOT(descriptionChanged()));
	connect(helper_symbol_check, SIGNAL(clicked(bool)), this, SLOT(helperSymbolClicked(bool)));
	
	connect(cancel_button, SIGNAL(clicked(bool)), this, SLOT(reject()));
	connect(ok_button, SIGNAL(clicked(bool)), this, SLOT(okClicked()));
	
	updateNumberEdits();
	updateOkButton();
	updateWindowTitle();
}
SymbolSettingDialog::~SymbolSettingDialog()
{
	delete[] number_edit;
}

void SymbolSettingDialog::updatePreview()
{
	createPreviewMap();
	
	for (int l = 0; l < preview_map->getNumLayers(); ++l)
		for (int i = 0; i < preview_map->getLayer(l)->getNumObjects(); ++i)
			preview_map->getLayer(l)->getObject(i)->update(true);
}

void SymbolSettingDialog::numberChanged(QString text)
{
	bool is_valid = true;
	for (int i = 0; i < Symbol::number_components; ++i)
	{
		QString number_text = number_edit[i]->text();
		if (is_valid && !number_text.isEmpty())
			symbol->setNumberComponent(i, number_text.toInt());
		else
		{
			symbol->setNumberComponent(i, -1);
			is_valid = false;
		}
	}
	
	updateNumberEdits();
	updateOkButton();
}
void SymbolSettingDialog::nameChanged(QString text)
{
	symbol->setName(text);
	updateOkButton();
	updateWindowTitle();
}
void SymbolSettingDialog::descriptionChanged()
{
	symbol->setDescription(description_edit->toPlainText());
}
void SymbolSettingDialog::helperSymbolClicked(bool checked)
{
	symbol->setIsHelperSymbol(checked);
}

void SymbolSettingDialog::loadTemplateClicked()
{
	Template* temp = TemplateWidget::showOpenTemplateDialog(this, preview_map_view);
	if (!temp)
		return;
	
	if (preview_map->getNumTemplates() > 0)
	{
		// Delete old template
		preview_map->setTemplateAreaDirty(0);
		preview_map->deleteTemplate(0);
	}
	
	preview_map->setFirstFrontTemplate(1);
	
	preview_map->addTemplate(temp, 0);
	TemplateVisibility* vis = preview_map_view->getTemplateVisibility(temp);
	vis->visible = true;
	vis->opacity = 1;
	preview_map->setTemplateAreaDirty(0);
	
	template_file_label->setText(temp->getTemplateFilename());
	center_template_button->setEnabled(temp->getTemplateType().compare("TemplateImage") == 0);
}
void SymbolSettingDialog::centerTemplateBBox()
{
	assert(preview_map->getNumTemplates() == 1);
	Template* temp = preview_map->getTemplate(0);
	
	QRectF extent = temp->getExtent();
	QPointF center = extent.center();
	MapCoordF map_center_current = temp->templateToMap(center);
	
	preview_map->setTemplateAreaDirty(0);
	temp->setTemplateX(temp->getTemplateX() - qRound64(1000 * map_center_current.getX()));
	temp->setTemplateY(temp->getTemplateY() - qRound64(1000 * map_center_current.getY()));
	preview_map->setTemplateAreaDirty(0);
}
void SymbolSettingDialog::centerTemplateGravity()
{
	assert(preview_map->getNumTemplates() == 1);
	Template* temp = preview_map->getTemplate(0);
	TemplateImage* image = reinterpret_cast<TemplateImage*>(temp);
	
	QColor background_color = QColorDialog::getColor(Qt::white, this, tr("Select background color"));
	if (!background_color.isValid())
		return;
	MapCoordF map_center_current = temp->templateToMap(image->calcCenterOfGravity(background_color.rgb()));
	
	preview_map->setTemplateAreaDirty(0);
	temp->setTemplateX(temp->getTemplateX() - qRound64(1000 * map_center_current.getX()));
	temp->setTemplateY(temp->getTemplateY() - qRound64(1000 * map_center_current.getY()));
	preview_map->setTemplateAreaDirty(0);
}

void SymbolSettingDialog::okClicked()
{
	// If editing a line symbol, strip unnecessary point symbols
	if (symbol->getType() == Symbol::Line)
	{
		LineSymbol* line = reinterpret_cast<LineSymbol*>(symbol);
		line->cleanupPointSymbols();
	}

	accept();
}

void SymbolSettingDialog::createPreviewMap()
{
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
			path->addCoordinate(0, MapCoordF(x_offset - length, y).toMapCoord());
			path->addCoordinate(1, MapCoordF(x_offset, y).toMapCoord());
			preview_map->addObject(path);
			
			preview_objects.push_back(path);
		}
		
		const int num_circular_lines = 12;
		const float inner_radius = 4;
		const float center_x = 2*inner_radius + 0.5f * max_length;
		const float center_y = 0.5f * y_start;
		
		for (int i = 0; i < num_circular_lines; ++i)
		{
			float angle = (i / (float)num_circular_lines) * 2*M_PI;
			float length = min_length + (i / (float)(num_circular_lines - 1)) * (max_length - min_length);
			
			PathObject* path = new PathObject(line);
			path->addCoordinate(0, ((MapCoordF(sin(angle), -cos(angle)) * inner_radius) + MapCoordF(center_x, center_y)).toMapCoord());
			path->addCoordinate(1, ((MapCoordF(sin(angle), -cos(angle)) * (inner_radius + length)) + MapCoordF(center_x, center_y)).toMapCoord());
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
		path->addCoordinate(0, MapCoordF(-half_radius, -half_radius + offset_y).toMapCoord());
		path->addCoordinate(1, MapCoordF(half_radius, -half_radius + offset_y).toMapCoord());
		path->addCoordinate(2, MapCoordF(half_radius, half_radius + offset_y).toMapCoord());
		path->addCoordinate(3, MapCoordF(-half_radius, half_radius + offset_y).toMapCoord());
		preview_map->addObject(path);
		
		preview_objects.push_back(path);
	}
	else if (symbol->getType() == Symbol::Text)
	{
		TextSymbol* text_symbol = reinterpret_cast<TextSymbol*>(symbol);
		
		// TODO: Suggestion for the German translation: "Franz, der OL für die Abkürzung\nvon Oldenbourg hält, jagt im komplett\nverwahrlosten Taxi quer durch Bayern\n1234567890"
		const QString string = tr("The quick brown fox\ntakes the routechoice\nto jump over the lazy dog\n1234567890");
		
		TextObject* object = new TextObject(text_symbol);
		object->setAnchorPosition(MapCoord(0, 0));
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
		path->setPathClosed(true);
		preview_map->addObject(path);
		
		preview_objects.push_back(path);
	}
}
PointSymbolEditorWidget* SymbolSettingDialog::createPointSymbolEditor(MapEditorController* controller)
{
	if (symbol->getType() == Symbol::Point)
	{
		PointSymbol* point = reinterpret_cast<PointSymbol*>(symbol);
		std::vector<PointSymbol*> point_vector;
		point_vector.push_back(point);
		return new PointSymbolEditorWidget(preview_map, controller, point_vector, 0, this);
	}
	else if (symbol->getType() == Symbol::Line)
	{
		LineSymbol* line = reinterpret_cast<LineSymbol*>(symbol);
		line->ensurePointSymbols(tr("Start symbol"), tr("Mid symbol"), tr("End symbol"), tr("Dash symbol"));
		std::vector<PointSymbol*> point_vector;
		point_vector.push_back(line->getStartSymbol());
		point_vector.push_back(line->getMidSymbol());
		point_vector.push_back(line->getEndSymbol());
		point_vector.push_back(line->getDashSymbol());
		PointSymbolEditorWidget* point_editor = new PointSymbolEditorWidget(preview_map, controller, point_vector, 16, this);
		connect(point_editor, SIGNAL(symbolEdited()), this, SLOT(createPreviewMap()));
		return point_editor;
	}
	else if (symbol->getType() == Symbol::Area)
	{
		AreaSymbol* area = reinterpret_cast<AreaSymbol*>(symbol);
		std::vector<PointSymbol*> point_vector;
		for (int i = 0; i < area->getNumFillPatterns(); ++i)
		{
			if (area->getFillPattern(i).type == AreaSymbol::FillPattern::PointPattern)
				point_vector.push_back(area->getFillPattern(i).point);
		}
		PointSymbolEditorWidget* point_editor = new PointSymbolEditorWidget(preview_map, controller, point_vector, 16, this);
		connect(point_editor, SIGNAL(symbolEdited()), this, SLOT(createPreviewMap()));
		return point_editor;
	}
	else if (symbol->getType() == Symbol::Text || symbol->getType() == Symbol::Combined)
		return NULL;
	
	assert(false);
	return NULL;
}

void SymbolSettingDialog::updateNumberEdits()
{
	bool enable = true;
	for (int i = 0; i < Symbol::number_components; ++i)
	{
		number_edit[i]->setEnabled(enable);
		if (enable && number_edit[i]->text().isEmpty())
			enable = false;
	}
}
void SymbolSettingDialog::updateOkButton()
{
	ok_button->setEnabled(!number_edit[0]->text().isEmpty() && !name_edit->text().isEmpty());
}
void SymbolSettingDialog::updateWindowTitle()
{
	if (symbol->getName().isEmpty())
		setWindowTitle(tr("Symbol settings - Please enter a symbol name!"));
	else
		setWindowTitle(tr("Symbol settings for %1").arg(symbol->getName()));
}

#include "moc_symbol_setting_dialog.cpp"
