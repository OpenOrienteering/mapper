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


#include "symbol_point_editor.h"

#include <assert.h>

#include <QtGui>

#include "symbol_point.h"
#include "symbol_line.h"
#include "symbol_area.h"
#include "map_color.h"
#include "object.h"
#include "util.h"
#include "map_widget.h"

PointSymbolEditorWidget::PointSymbolEditorWidget(Map* map, MapEditorController* controller, std::vector< PointSymbol* > symbols, float offset_y, QWidget* parent) : QWidget(parent), offset_y(offset_y), map(map)
{
	react_to_changes = true;
	
	QLabel* editor_label = new QLabel("<b>" + tr("Point editor") + "</b>");
	
	symbol_info.resize(symbols.size());
	for (int i = 0; i < (int)symbols.size(); ++i)
	{
		symbol_info[i].midpoint_object = new PointObject(symbols[i]);
		map->addObject(symbol_info[i].midpoint_object);
	}
	
	current_symbol_label = new QLabel(tr("Edit:"));
	current_symbol_combo = new QComboBox();
	for (int i = 0; i < (int)symbols.size(); ++i)
		current_symbol_combo->addItem(symbols[i]->getName(), qVariantFromValue<void*>(symbols[i]));
	current_symbol = NULL;
	
	QLabel* elements_label = new QLabel(tr("Elements:"));
	element_list = new QListWidget();
	updateElementList();
	
	delete_element_button = new QPushButton(QIcon(":/images/minus.png"), "");
	QToolButton* add_element_button = new QToolButton();
	add_element_button->setIcon(QIcon(":/images/plus.png"));
	add_element_button->setToolButtonStyle(Qt::ToolButtonIconOnly);
	add_element_button->setPopupMode(QToolButton::InstantPopup);
	add_element_button->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
	add_element_button->setMinimumWidth(delete_element_button->sizeHint().width());
	QMenu* add_element_button_menu = new QMenu(add_element_button);
	add_element_button_menu->addAction(tr("Point"), this, SLOT(addPointClicked()));
	add_element_button_menu->addAction(tr("Line"), this, SLOT(addLineClicked()));
	add_element_button_menu->addAction(tr("Area"), this, SLOT(addAreaClicked()));
	add_element_button->setMenu(add_element_button_menu);
	
	QLabel* current_element_label = new QLabel("<b>" + tr("Current element") + "</b>");
	
	element_properties_widget = new QStackedWidget();
	
	// Point
	QWidget* point_properties = new QWidget();
	QLabel* point_inner_radius_label = new QLabel(tr("Diameter <b>a</b>:"));
	point_inner_radius_edit = new QLineEdit();
	point_inner_radius_edit->setValidator(new DoubleValidator(0, 99999, point_inner_radius_edit));
	
	QLabel* point_inner_color_label = new QLabel(tr("Inner color:"));
	point_inner_color_edit = new ColorDropDown(map);
	
	QLabel* point_outer_width_label = new QLabel(tr("Outer width <b>b</b>:"));
	point_outer_width_edit = new QLineEdit();
	point_outer_width_edit->setValidator(new DoubleValidator(0, 99999, point_outer_width_edit));
	
	QLabel* point_outer_color_label = new QLabel(tr("Outer color:"));
	point_outer_color_edit = new ColorDropDown(map);
	
	QLabel* explanation_label = new QLabel();
	explanation_label->setPixmap(QPixmap(":/images/symbol_point_explanation.png"));
	
	QGridLayout* point_layout = new QGridLayout();
	point_layout->addWidget(point_inner_radius_label, 0, 0);
	point_layout->addWidget(point_inner_radius_edit, 0, 1);
	point_layout->addWidget(point_inner_color_label, 1, 0);
	point_layout->addWidget(point_inner_color_edit, 1, 1);
	point_layout->addWidget(point_outer_width_label, 2, 0);
	point_layout->addWidget(point_outer_width_edit, 2, 1);
	point_layout->addWidget(point_outer_color_label, 3, 0);
	point_layout->addWidget(point_outer_color_edit, 3, 1);
	point_layout->addWidget(explanation_label, 0, 2, 4, 1);
	point_properties->setLayout(point_layout);
	element_properties_widget->addWidget(point_properties);
	
	// Line
	QWidget* line_properties = new QWidget();
	QLabel* line_width_label = new QLabel(tr("Line width:"));
	line_width_edit = new QLineEdit();
	line_width_edit->setValidator(new DoubleValidator(0, 99999, line_width_edit));
	
	QLabel* line_color_label = new QLabel(tr("Line color:"));
	line_color_edit = new ColorDropDown(map);
	
	QLabel* line_cap_label = new QLabel(tr("Line cap:"));
	line_cap_edit = new QComboBox();
	line_cap_edit->addItem(tr("flat"), QVariant(LineSymbol::FlatCap));
	line_cap_edit->addItem(tr("round"), QVariant(LineSymbol::RoundCap));
	line_cap_edit->addItem(tr("square"), QVariant(LineSymbol::SquareCap));
	//line_cap_edit->addItem(tr("pointed"), QVariant(LineSymbol::PointedCap));	// this would require another input field for the cap length
	
	QLabel* line_join_label = new QLabel(tr("Line join:"));
	line_join_edit = new QComboBox();
	line_join_edit->addItem(tr("miter"), QVariant(LineSymbol::MiterJoin));
	line_join_edit->addItem(tr("round"), QVariant(LineSymbol::RoundJoin));
	line_join_edit->addItem(tr("bevel"), QVariant(LineSymbol::BevelJoin));
	
	line_closed_check = new QCheckBox(tr("Line closed"));
	
	QGridLayout* line_layout = new QGridLayout();
	line_layout->addWidget(line_width_label, 0, 0);
	line_layout->addWidget(line_width_edit, 0, 1);
	line_layout->addWidget(line_color_label, 1, 0);
	line_layout->addWidget(line_color_edit, 1, 1);
	line_layout->addWidget(line_cap_label, 2, 0);
	line_layout->addWidget(line_cap_edit, 2, 1);
	line_layout->addWidget(line_join_label, 3, 0);
	line_layout->addWidget(line_join_edit, 3, 1);
	line_layout->addWidget(line_closed_check, 4, 0, 1, 2);
	line_properties->setLayout(line_layout);
	element_properties_widget->addWidget(line_properties);
	
	// Area
	QWidget* area_properties = new QWidget();
	QLabel* area_color_label = new QLabel(tr("Area color:"));
	area_color_edit = new ColorDropDown(map);
	
	QGridLayout* area_layout = new QGridLayout();
	area_layout->addWidget(area_color_label, 0, 0);
	area_layout->addWidget(area_color_edit, 0, 1);
	area_layout->setRowStretch(1, 1);
	area_properties->setLayout(area_layout);
	element_properties_widget->addWidget(area_properties);
	
	// Coordinates
	QLabel* coordinates_label = new QLabel(tr("Coordinates:"));
	
	coords_table = new QTableWidget(0, 3);
	coords_table->setEditTriggers(QAbstractItemView::AllEditTriggers);
	coords_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	coords_table->setHorizontalHeaderLabels(QStringList() << tr("X") << tr("Y") << tr("Curve start"));
	coords_table->verticalHeader()->setVisible(false);
	
	QHeaderView* header_view = coords_table->horizontalHeader();
	header_view->setResizeMode(0, QHeaderView::Interactive);
	header_view->setResizeMode(1, QHeaderView::Interactive);
	header_view->setResizeMode(2, QHeaderView::ResizeToContents);
	header_view->setClickable(false);
	
	add_coord_button = new QPushButton(QIcon(":/images/plus.png"), "");
	delete_coord_button = new QPushButton(QIcon(":/images/minus.png"), "");
	center_coords_button = new QPushButton(tr("Center by coordinate average"));
	
	// Layout
	QVBoxLayout* layout = new QVBoxLayout();
	layout->setMargin(0);
	
	layout->addWidget(editor_label);
	QHBoxLayout* current_symbol_layout = new QHBoxLayout();
	current_symbol_layout->addWidget(current_symbol_label);
	current_symbol_layout->addWidget(current_symbol_combo);
	layout->addLayout(current_symbol_layout);
	layout->addSpacing(16);
	
	layout->addWidget(elements_label);
	layout->addWidget(element_list);
	QHBoxLayout* element_buttons_layout = new QHBoxLayout();
	element_buttons_layout->addStretch(1);
	element_buttons_layout->addWidget(add_element_button);
	element_buttons_layout->addWidget(delete_element_button);
	element_buttons_layout->addStretch(1);
	layout->addLayout(element_buttons_layout);
	layout->addSpacing(16);
	
	layout->addWidget(current_element_label);
	layout->addWidget(element_properties_widget);
	layout->addSpacing(16);
	
	layout->addWidget(coordinates_label);
	layout->addWidget(coords_table);
	QHBoxLayout* coords_buttons_layout = new QHBoxLayout();
	coords_buttons_layout->addWidget(add_coord_button);
	coords_buttons_layout->addWidget(delete_coord_button);
	coords_buttons_layout->addStretch(1);
	coords_buttons_layout->addWidget(center_coords_button);
	layout->addLayout(coords_buttons_layout);
	setLayout(layout);
	
	if (symbols.size() <= 1)
	{
		current_symbol_label->hide();
		current_symbol_combo->hide();
		
		if (symbols.size() == 0)
			hide();
	}
	
	// Connections
	connect(current_symbol_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(currentSymbolChanged(int)));
	connect(element_list, SIGNAL(currentRowChanged(int)), this, SLOT(elementChanged(int)));
	connect(delete_element_button, SIGNAL(clicked(bool)), this, SLOT(deleteElementClicked()));
	
	connect(point_inner_radius_edit, SIGNAL(textEdited(QString)), this, SLOT(pointInnerRadiusChanged(QString)));
	connect(point_inner_color_edit, SIGNAL(currentIndexChanged(int)), this, SLOT(pointInnerColorChanged()));
	connect(point_outer_width_edit, SIGNAL(textEdited(QString)), this, SLOT(pointOuterWidthChanged(QString)));
	connect(point_outer_color_edit, SIGNAL(currentIndexChanged(int)), this, SLOT(pointOuterColorChanged()));
	
	connect(line_width_edit, SIGNAL(textEdited(QString)), this, SLOT(lineWidthChanged(QString)));
	connect(line_color_edit, SIGNAL(currentIndexChanged(int)), this, SLOT(lineColorChanged()));
	connect(line_cap_edit, SIGNAL(currentIndexChanged(int)), this, SLOT(lineCapChanged(int)));
	connect(line_join_edit, SIGNAL(currentIndexChanged(int)), this, SLOT(lineJoinChanged(int)));
	connect(line_closed_check, SIGNAL(clicked(bool)), this, SLOT(lineClosedClicked(bool)));
	
	connect(area_color_edit, SIGNAL(currentIndexChanged(int)), this, SLOT(areaColorChanged()));
	
	connect(coords_table, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(currentCoordChanged()));
	connect(coords_table, SIGNAL(cellChanged(int,int)), this, SLOT(coordinateChanged(int,int)));
	connect(add_coord_button, SIGNAL(clicked(bool)), this, SLOT(addCoordClicked()));
	connect(delete_coord_button, SIGNAL(clicked(bool)), this, SLOT(deleteCoordClicked()));
	connect(center_coords_button, SIGNAL(clicked(bool)), this, SLOT(centerCoordsClicked()));
	
	if (current_symbol_combo->count() > 0)
	{
		current_symbol_combo->setCurrentIndex(0);
		currentSymbolChanged(current_symbol_combo->currentIndex());
	}
	else
	{
		if (element_list->count() > 0)
			element_list->setCurrentRow(0);
		elementChanged(element_list->currentRow());
	}
	
	controller->setTool(new PointSymbolEditorTool(controller, this));
	activity = new PointSymbolEditorActivity(map, this);
	controller->setEditorActivity(activity);
	updateSymbolPositions();
}

void PointSymbolEditorWidget::addSymbol(PointSymbol* symbol)
{
	int index = (int)symbol_info.size();
	symbol_info.insert(symbol_info.begin() + index, SymbolInfo());
	symbol_info[index].midpoint_object = new PointObject(symbol);
	map->addObject(symbol_info[index].midpoint_object);
	current_symbol_combo->insertItem(index, symbol->getName(), qVariantFromValue<void*>(symbol));
	current_symbol_combo->setCurrentIndex(index);
	
	if (current_symbol_combo->count() == 1)
		show();
	else if (current_symbol_combo->count() == 2)
	{
		current_symbol_combo->show();
		current_symbol_label->show();
	}
	
	updateSymbolPositions();
}
void PointSymbolEditorWidget::removeSymbol(PointSymbol* symbol)
{
	int index = current_symbol_combo->findData(qVariantFromValue<void*>(symbol));
	map->deleteObject(symbol_info[index].midpoint_object, false);
	symbol_info.erase(symbol_info.begin() + index);
	current_symbol_combo->removeItem(index);
	
	if (current_symbol_combo->count() == 1)
	{
		current_symbol_combo->hide();
		current_symbol_label->hide();
	}
	else if (current_symbol_combo->count() == 0)
		hide();
	
	updateSymbolPositions();
}
void PointSymbolEditorWidget::setCurrentSymbol(PointSymbol* symbol)
{
	current_symbol_combo->setCurrentIndex(current_symbol_combo->findData(qVariantFromValue<void*>(symbol)));
}

bool PointSymbolEditorWidget::changeCurrentCoordinate(MapCoordF new_coord)
{
	Object* object = getCurrentElementObject();
	if (object == getMidpointObject())
		return false;
	
	SymbolInfo& info = symbol_info[current_symbol_combo->currentIndex()];
	
	if (object->getType() == Object::Point)
	{
		PointObject* point = reinterpret_cast<PointObject*>(object);
		MapCoord coord = point->getPosition();
		coord.setX(new_coord.getX() - info.origin_x);
		coord.setY(new_coord.getY() - info.origin_y);
		point->setPosition(coord);
	}
	else
	{
		int row = coords_table->currentRow();
		if (row < 0)
			return false;
		assert(object->getType() == Object::Path);
		assert(row < object->getCoordinateCount());
		PathObject* path = reinterpret_cast<PathObject*>(object);
		MapCoord coord = path->getCoordinate(row);
		coord.setX(new_coord.getX() - info.origin_x);
		coord.setY(new_coord.getY() - info.origin_y);
		path->setCoordinate(row, coord);
	}
	
	updateCoordsTable();
	getMidpointObject()->update(true);
	emit(symbolEdited());
	return true;
}
bool PointSymbolEditorWidget::addCoordinate(MapCoordF new_coord)
{
	Object* object = getCurrentElementObject();
	if (object == getMidpointObject())
		return false;
	
	if (object->getType() == Object::Point)
		return changeCurrentCoordinate(new_coord);
	assert(object->getType() == Object::Path);
	PathObject* path = reinterpret_cast<PathObject*>(object);
	
	int row = coords_table->currentRow();
	if (row < 0)
		row = coords_table->rowCount();
	else
		++row;
	
	SymbolInfo& info = symbol_info[current_symbol_combo->currentIndex()];
	path->addCoordinate(row, MapCoordF(new_coord.getX() - info.origin_x, new_coord.getY() - info.origin_y).toMapCoord());
	
	updateCoordsTable();
	coords_table->setCurrentItem(coords_table->item(row, (coords_table->currentColumn() < 0) ? 0 : coords_table->currentColumn()));
	getMidpointObject()->update(true);
	emit(symbolEdited());
	return true;
}

void PointSymbolEditorWidget::updateSymbolPositions()
{
	const float symbol_distance = 10;
	
	for (int i = 0; i < (int)symbol_info.size(); ++i)
	{
		symbol_info[i].origin_x = -(symbol_distance / 2) * ((int)symbol_info.size() - 1) + symbol_distance * i;
		symbol_info[i].origin_y = offset_y;
		
		symbol_info[i].midpoint_object->setPosition(MapCoord(symbol_info[i].origin_x, symbol_info[i].origin_y));
		symbol_info[i].midpoint_object->update(true);
	}
	
	activity->update();
}

void PointSymbolEditorWidget::updateElementList()
{
	element_list->clear();
	if (!current_symbol)
		return;
	
	element_list->addItem(tr("[Midpoint]"));
	for (int i = 0; i < current_symbol->getNumElements(); ++i)
	{
		Symbol* symbol = current_symbol->getElementSymbol(i);
		element_list->addItem(getLabelForSymbol(symbol));
	}
	element_list->setCurrentRow(0);
}
void PointSymbolEditorWidget::updateSymbolNames()
{
	for (int i = 0; i < current_symbol_combo->count(); ++i)
		current_symbol_combo->setItemText(i, symbol_info[i].midpoint_object->getSymbol()->getName());
}

void PointSymbolEditorWidget::currentSymbolChanged(int index)
{
	if (index < 0)
		current_symbol = NULL;
	else
		current_symbol = reinterpret_cast<PointSymbol*>(current_symbol_combo->itemData(index, Qt::UserRole).value<void*>());
	updateElementList();
}
void PointSymbolEditorWidget::elementChanged(int row)
{
	bool enable = row >= 0;
	coords_table->setEnabled(enable);
	add_coord_button->setEnabled(enable);
	delete_coord_button->setEnabled(enable);
	center_coords_button->setEnabled(enable);
	delete_element_button->setEnabled(enable && row > 0);	// cannot delete first row
	
	if (row < 0)
		return;
	
	Symbol* symbol = (row == 0) ? current_symbol : current_symbol->getElementSymbol(row - 1);
	Object* object = getCurrentElementObject();
	
	react_to_changes = false;
	int stack_index;
	if (symbol->getType() == Symbol::Line)
	{
		stack_index = 1;
		LineSymbol* line = reinterpret_cast<LineSymbol*>(symbol);
		line_width_edit->setText(QString::number(0.001 * line->getLineWidth()));
		line_color_edit->setColor(line->getColor());
		line_cap_edit->setCurrentIndex(line_cap_edit->findData(QVariant(line->getCapStyle())));
		line_join_edit->setCurrentIndex(line_join_edit->findData(QVariant(line->getJoinStyle())));
		line_closed_check->setChecked(object->isPathClosed());
	}
	else if (symbol->getType() == Symbol::Area)
	{
		stack_index = 2;
		AreaSymbol* area = reinterpret_cast<AreaSymbol*>(symbol);
		area_color_edit->setColor(area->getColor());
	}
	else
	{
		stack_index = 0;
		PointSymbol* point = reinterpret_cast<PointSymbol*>(symbol);
		point_inner_radius_edit->setText(QString::number(2 * 0.001 * point->getInnerRadius()));
		point_inner_color_edit->setColor(point->getInnerColor());
		point_outer_width_edit->setText(QString::number(0.001 * point->getOuterWidth()));
		point_outer_color_edit->setColor(point->getOuterColor());
	}
	element_properties_widget->setCurrentIndex(stack_index);
	
	coords_table->clearContents();
	coords_table->setEnabled(row > 0);
	add_coord_button->setEnabled(symbol->getType() != Symbol::Point);
	updateDeleteCoordButton();
	center_coords_button->setEnabled(coords_table->rowCount() > 0);
	
	coords_table->setColumnHidden(2, symbol->getType() == Symbol::Point);
	if (row > 0)
		updateCoordsTable();
	else
		coords_table->setRowCount(0);
	
	react_to_changes = true;
}

void PointSymbolEditorWidget::addPointClicked()
{
	PointSymbol* new_point = new PointSymbol();
	PointObject* new_object = new PointObject(new_point);
	
	insertElement(new_object, new_point);
}
void PointSymbolEditorWidget::addLineClicked()
{
	LineSymbol* new_line = new LineSymbol();
	PathObject* new_object = new PathObject(new_line);
	
	insertElement(new_object, new_line);
}
void PointSymbolEditorWidget::addAreaClicked()
{
	AreaSymbol* new_area = new AreaSymbol();
	PathObject* new_object = new PathObject(new_area);
	new_object->setPathClosed(true);
	
	insertElement(new_object, new_area);
}
void PointSymbolEditorWidget::deleteElementClicked()
{
	int row = element_list->currentRow();
	assert(row > 0);
	int pos = row - 1;
	
	current_symbol->deleteElement(pos);
	delete element_list->item(row);
	
	getMidpointObject()->update(true);
	emit(symbolEdited());
}

void PointSymbolEditorWidget::pointInnerRadiusChanged(QString text)
{
	if (!react_to_changes) return;
	PointSymbol* symbol = reinterpret_cast<PointSymbol*>(getCurrentElementSymbol());
	symbol->inner_radius = qRound(1000 * 0.5 * text.toDouble());
	getMidpointObject()->update(true);
	emit(symbolEdited());
}
void PointSymbolEditorWidget::pointInnerColorChanged()
{
	if (!react_to_changes) return;
	PointSymbol* symbol = reinterpret_cast<PointSymbol*>(getCurrentElementSymbol());
	symbol->inner_color = point_inner_color_edit->color();
	getMidpointObject()->update(true);
	emit(symbolEdited());
}
void PointSymbolEditorWidget::pointOuterWidthChanged(QString text)
{
	if (!react_to_changes) return;
	PointSymbol* symbol = reinterpret_cast<PointSymbol*>(getCurrentElementSymbol());
	symbol->outer_width = qRound(1000 * text.toDouble());
	getMidpointObject()->update(true);
	emit(symbolEdited());
}
void PointSymbolEditorWidget::pointOuterColorChanged()
{
	if (!react_to_changes) return;
	PointSymbol* symbol = reinterpret_cast<PointSymbol*>(getCurrentElementSymbol());
	symbol->outer_color = point_outer_color_edit->color();
	getMidpointObject()->update(true);
	emit(symbolEdited());
}

void PointSymbolEditorWidget::lineWidthChanged(QString text)
{
	if (!react_to_changes) return;
	LineSymbol* symbol = reinterpret_cast<LineSymbol*>(getCurrentElementSymbol());
	symbol->line_width = qRound(1000 * text.toDouble());
	getMidpointObject()->update(true);
	emit(symbolEdited());
}
void PointSymbolEditorWidget::lineColorChanged()
{
	if (!react_to_changes) return;
	LineSymbol* symbol = reinterpret_cast<LineSymbol*>(getCurrentElementSymbol());
	symbol->color = line_color_edit->color();
	getMidpointObject()->update(true);
	emit(symbolEdited());
}
void PointSymbolEditorWidget::lineCapChanged(int index)
{
	if (!react_to_changes) return;
	LineSymbol* symbol = reinterpret_cast<LineSymbol*>(getCurrentElementSymbol());
	symbol->cap_style = static_cast<LineSymbol::CapStyle>(line_cap_edit->itemData(index).toInt());
	getMidpointObject()->update(true);
	emit(symbolEdited());
}
void PointSymbolEditorWidget::lineJoinChanged(int index)
{
	if (!react_to_changes) return;
	LineSymbol* symbol = reinterpret_cast<LineSymbol*>(getCurrentElementSymbol());
	symbol->join_style = static_cast<LineSymbol::JoinStyle>(line_join_edit->itemData(index).toInt());
	getMidpointObject()->update(true);
	emit(symbolEdited());
}
void PointSymbolEditorWidget::lineClosedClicked(bool checked)
{
	if (!react_to_changes) return;
	Object* object = getCurrentElementObject();
	if (!checked && object->getCoordinateCount() >= 3 && object->getCoordinate(object->getCoordinateCount() - 3).isCurveStart())
	{
		PathObject* path = reinterpret_cast<PathObject*>(object);
		MapCoord coord = path->getCoordinate(object->getCoordinateCount() - 3);
		coord.setCurveStart(false);
		path->setCoordinate(object->getCoordinateCount() - 3, coord);
	}
	object->setPathClosed(checked);
	updateCoordsTable();
	getMidpointObject()->update(true);
	emit(symbolEdited());
}

void PointSymbolEditorWidget::areaColorChanged()
{
	if (!react_to_changes) return;
	AreaSymbol* symbol = reinterpret_cast<AreaSymbol*>(getCurrentElementSymbol());
	symbol->color = area_color_edit->color();
	getMidpointObject()->update(true);
	emit(symbolEdited());
}

void PointSymbolEditorWidget::currentCoordChanged()
{
	updateDeleteCoordButton();
	center_coords_button->setEnabled(coords_table->rowCount() > 0);
}
void PointSymbolEditorWidget::coordinateChanged(int row, int column)
{
	if (!react_to_changes) return;
	Object* object = getCurrentElementObject();
	
	if (column < 2)
	{
		bool ok = false;
		double new_value = coords_table->item(row, column)->text().toDouble(&ok);
		
		if (ok)
		{
			if (object->getType() == Object::Point)
			{
				PointObject* point = reinterpret_cast<PointObject*>(object);
				MapCoord coord = point->getPosition();
				if (column == 0)
					coord.setX(new_value);
				else
					coord.setY(new_value);
				point->setPosition(coord);
			}
			else
			{
				assert(object->getType() == Object::Path);
				assert(row < object->getCoordinateCount());
				PathObject* path = reinterpret_cast<PathObject*>(object);
				MapCoord coord = path->getCoordinate(row);
				if (column == 0)
					coord.setX(new_value);
				else
					coord.setY(new_value);
				path->setCoordinate(row, coord);
			}
			
			getMidpointObject()->update(true);
			emit(symbolEdited());
		}
		else
		{
			react_to_changes = false;
			coords_table->item(row, column)->setText(QString::number((column == 0) ? object->getCoordinate(row).xd() : object->getCoordinate(row).yd()));
			react_to_changes = true;
		}
	}
	else
	{
		assert(object->getType() == Object::Path);
		assert(row < object->getCoordinateCount());
		PathObject* path = reinterpret_cast<PathObject*>(object);
		MapCoord coord = path->getCoordinate(row);
		coord.setCurveStart(coords_table->item(row, column)->checkState() == Qt::Checked);
		path->setCoordinate(row, coord);
		
		updateCoordsTable();
		getMidpointObject()->update(true);
		emit(symbolEdited());
	}
}
void PointSymbolEditorWidget::addCoordClicked()
{
	Object* object = getCurrentElementObject();
	assert(object->getType() == Object::Path);
	PathObject* path = reinterpret_cast<PathObject*>(object);
	
	if (coords_table->currentRow() < 0)
		path->addCoordinate(coords_table->rowCount(), MapCoord(0, 0));
	else
		path->addCoordinate(coords_table->currentRow() + 1, path->getCoordinate(coords_table->currentRow()));
	
	int row = (coords_table->currentRow() < 0) ? coords_table->rowCount() : (coords_table->currentRow() + 1);
	updateCoordsTable();	// NOTE: incremental updates (to the curve start boxes) would be possible but mean some implementation effort
	coords_table->setCurrentItem(coords_table->item(row, coords_table->currentColumn()));
}
void PointSymbolEditorWidget::deleteCoordClicked()
{
	Object* object = getCurrentElementObject();
	assert(object->getType() == Object::Path);
	PathObject* path = reinterpret_cast<PathObject*>(object);
	
	int row = coords_table->currentRow();
	assert(row >= 0);
	
	path->deleteCoordinate(row);
	
	updateCoordsTable();	// NOTE: incremental updates (to the curve start boxes) would be possible but mean some implementation effort
	getMidpointObject()->update(true);
	emit(symbolEdited());
	updateDeleteCoordButton();
}
void PointSymbolEditorWidget::centerCoordsClicked()
{
	Object* object = getCurrentElementObject();
	
	if (object->getType() == Object::Point)
	{
		PointObject* point = reinterpret_cast<PointObject*>(object);
		point->setPosition(MapCoord(0, 0));
	}
	else
	{
		assert(object->getType() == Object::Path);
		PathObject* path = reinterpret_cast<PathObject*>(object);
		
		MapCoordF center = MapCoordF(0, 0);
		int size = path->getCoordinateCount();
		assert(size > 0);
		for (int i = 0; i < size; ++i)
			center = MapCoordF(center.getX() + path->getCoordinate(i).xd(), center.getY() + path->getCoordinate(i).yd());
		center = MapCoordF(center.getX() / path->getCoordinateCount(), center.getY() / path->getCoordinateCount());
		
		for (int i = 0; i < size; ++i)
		{
			MapCoord coord = path->getCoordinate(i);
			coord.setX(coord.xd() - center.getX());
			coord.setY(coord.yd() - center.getY());
			path->setCoordinate(i, coord);
		}
		
		updateCoordsTable();
		getMidpointObject()->update(true);
		emit(symbolEdited());
	}
}

void PointSymbolEditorWidget::updateCoordsTable()
{
	bool temp = react_to_changes;
	react_to_changes = false;
	
	int num_rows = getCurrentElementObject()->getCoordinateCount();
	coords_table->setRowCount(num_rows);
	for (int i = 0; i < num_rows; ++i)
		addCoordsRow(i);
	
	center_coords_button->setEnabled(num_rows > 0);
	
	react_to_changes = temp;
}
void PointSymbolEditorWidget::addCoordsRow(int row)
{
	coords_table->setRowHeight(row, coords_table->fontMetrics().height() + 2);
	
	for (int c = 0; c < 3; ++c)
	{
		if (!coords_table->item(row, c))
		{
			QTableWidgetItem* item = new QTableWidgetItem();
			if (c < 2)
			{
				item->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
				item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
			}
			coords_table->setItem(row, c, item);
		}
	}
	
	updateCoordsRow(row);
}
void PointSymbolEditorWidget::updateCoordsRow(int row)
{
	assert(element_list->currentRow() > 0);
	Object* object = getCurrentElementObject();
	
	coords_table->item(row, 0)->setText(QString::number(object->getCoordinate(row).xd()));
	coords_table->item(row, 1)->setText(QString::number(object->getCoordinate(row).yd()));
	
	bool has_curve_start_box = (object->getType() != Object::Point) &&
	                           (row < object->getCoordinateCount() - (object->isPathClosed() ? 2 : 3)) &&
	                           (!object->getCoordinate(row+1).isCurveStart() && !object->getCoordinate(row+2).isCurveStart()) &&
	                           (row <= 0 || !object->getCoordinate(row-1).isCurveStart()) &&
	                           (row <= 1 || !object->getCoordinate(row-2).isCurveStart());
	if (has_curve_start_box)
	{
		coords_table->item(row, 2)->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
		coords_table->item(row, 2)->setCheckState(object->getCoordinate(row).isCurveStart() ? Qt::Checked : Qt::Unchecked);
	}
	else
	{
		if (coords_table->item(row, 2)->flags() & Qt::ItemIsUserCheckable)
			coords_table->setItem(row, 2, new QTableWidgetItem());	// remove checkbox by replacing the item with a new one - is there a better way?
		coords_table->item(row, 2)->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	}
}
void PointSymbolEditorWidget::updateDeleteCoordButton()
{
	Object* object = getCurrentElementObject();
	delete_coord_button->setEnabled(object->getType() == Object::Path && coords_table->currentRow() >= 0);
}

void PointSymbolEditorWidget::insertElement(Object* object, Symbol* symbol)
{
	int row = (element_list->currentRow() < 0) ? element_list->count() : (element_list->currentRow() + 1);
	int pos = row - 1;
	current_symbol->addElement(pos, object, symbol);
	element_list->insertItem(row, getLabelForSymbol(symbol));
	element_list->setCurrentRow(row);
	getMidpointObject()->update(true);
	emit(symbolEdited());
}
QString PointSymbolEditorWidget::getLabelForSymbol(Symbol* symbol)
{
	if (symbol->getType() == Symbol::Point)
		return tr("Point");
	else if (symbol->getType() == Symbol::Line)
		return tr("Line");
	else if (symbol->getType() == Symbol::Area)
		return tr("Area");
	
	assert(false);
	return tr("Unknown");
}

Object* PointSymbolEditorWidget::getMidpointObject()
{
	if (current_symbol_combo->currentIndex() < 0)
		return NULL;
	else
		return symbol_info[current_symbol_combo->currentIndex()].midpoint_object;
}
Object* PointSymbolEditorWidget::getCurrentElementObject()
{
	if (element_list->currentRow() > 0)
		return current_symbol->getElementObject(element_list->currentRow() - 1);
	else
		return getMidpointObject();
}
Symbol* PointSymbolEditorWidget::getCurrentElementSymbol()
{
	if (element_list->currentRow() > 0)
		return current_symbol->getElementSymbol(element_list->currentRow() - 1);
	else
		return current_symbol;
}

// ### PointSymbolEditorTool ###

QCursor* PointSymbolEditorTool::cursor = NULL;

PointSymbolEditorTool::PointSymbolEditorTool(MapEditorController* editor, PointSymbolEditorWidget* widget): MapEditorTool(editor, Other)
{
	this->widget = widget;
	
	if (!cursor)
		cursor = new QCursor(QPixmap(":/images/cursor-crosshair.png"), 11, 11);
}
void PointSymbolEditorTool::init()
{
    setStatusBarText(tr("<b>Click</b> to add a coordinate, <b>Ctrl+Click</b> to change the selected coordinate"));
}
bool PointSymbolEditorTool::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (event->button() == Qt::LeftButton)
	{
		if (event->modifiers() & Qt::CTRL)
			this->widget->changeCurrentCoordinate(map_coord);
		else
			this->widget->addCoordinate(map_coord);
		return true;
	}
	return false;
}

// ### PointSymbolEditorActivity ###

const int PointSymbolEditorActivity::cross_radius = 5;

PointSymbolEditorActivity::PointSymbolEditorActivity(Map* map, PointSymbolEditorWidget* widget): MapEditorActivity(), map(map), widget(widget)
{
}
void PointSymbolEditorActivity::init()
{
	update();
}
void PointSymbolEditorActivity::update()
{
	if (widget->symbol_info.size() == 0)
	{
		map->clearActivityBoundingBox();
		return;	
	}
	
	QRectF rect = QRectF(widget->symbol_info[0].origin_x, widget->symbol_info[0].origin_y, 0, 0);
	for (int i = 1; i < (int)widget->symbol_info.size(); ++i)
		rectInclude(rect, QPointF(widget->symbol_info[i].origin_x, widget->symbol_info[i].origin_y));
	
	map->setActivityBoundingBox(rect, cross_radius + 1);
}
void PointSymbolEditorActivity::draw(QPainter* painter, MapWidget* widget)
{
    for (int i = 0; i < (int)this->widget->symbol_info.size(); ++i)
	{
		PointSymbolEditorWidget::SymbolInfo& info = this->widget->symbol_info[i];
		QPoint midpoint = widget->mapToViewport(MapCoordF(info.origin_x, info.origin_y)).toPoint();
		
		QPen pen = QPen(Qt::white);
		pen.setWidth(3);
		painter->setPen(pen);
		painter->drawLine(midpoint + QPoint(0, -cross_radius), midpoint + QPoint(0, cross_radius));
		painter->drawLine(midpoint + QPoint(-cross_radius, 0), midpoint + QPoint(cross_radius, 0));
		
		pen.setWidth(0);
		pen.setColor(Qt::black);
		painter->setPen(pen);
		painter->drawLine(midpoint + QPoint(0, -cross_radius), midpoint + QPoint(0, cross_radius));
		painter->drawLine(midpoint + QPoint(-cross_radius, 0), midpoint + QPoint(cross_radius, 0));
	}
}

#include "moc_symbol_point_editor.cpp"
