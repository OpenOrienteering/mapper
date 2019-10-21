/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2015, 2017 Kai Pastor
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


#include "template_adjust.h"

#include <QAbstractItemView>
#include <QAction>
#include <QCheckBox>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QTableWidget>
#include <QToolBar>
#include <QVBoxLayout>

#include "core/map.h"
#include "gui/main_window.h"
#include "gui/modifier_key.h"
#include "gui/util_gui.h"
#include "gui/map/map_editor.h"
#include "gui/map/map_widget.h"
#include "templates/template.h"
#include "util/transformation.h"
#include "util/util.h"


namespace OpenOrienteering {

float TemplateAdjustActivity::cross_radius = 4;

TemplateAdjustActivity::TemplateAdjustActivity(Template* temp, MapEditorController* controller) : controller(controller)
{
	setActivityObject(temp);
	connect(controller->getMap(), &Map::templateChanged, this, &TemplateAdjustActivity::templateChanged);
	connect(controller->getMap(), &Map::templateDeleted, this, &TemplateAdjustActivity::templateDeleted);
}

TemplateAdjustActivity::~TemplateAdjustActivity()
{
	widget->stopTemplateAdjust();
	delete dock;
}

void TemplateAdjustActivity::init()
{
	auto temp = reinterpret_cast<Template*>(activity_object);
	
	dock = new TemplateAdjustDockWidget(tr("Template adjustment"), controller, controller->getWindow());
	widget = new TemplateAdjustWidget(temp, controller, dock);
	dock->setWidget(widget);
	
	// Show dock in floating state
	dock->setFloating(true);
	dock->show();
	dock->setGeometry(controller->getWindow()->geometry().left() + 40, controller->getWindow()->geometry().top() + 100, dock->width(), dock->height());
}

void TemplateAdjustActivity::draw(QPainter* painter, MapWidget* widget)
{
	auto temp = reinterpret_cast<Template*>(activity_object);
	bool adjusted = temp->isAdjustmentApplied();
    
	for (int i = 0; i < temp->getNumPassPoints(); ++i)
	{
		auto point = temp->getPassPoint(i);
		QPointF start = widget->mapToViewport(adjusted ? point->calculated_coords : point->src_coords);
		QPointF end = widget->mapToViewport(point->dest_coords);
		
		drawCross(painter, start.toPoint(), QColor(Qt::red));
		painter->drawLine(start, end);
		drawCross(painter, end.toPoint(), QColor(Qt::green));
	}
}

void TemplateAdjustActivity::drawCross(QPainter* painter, const QPoint& midpoint, QColor color)
{
	painter->setPen(color);
	painter->drawLine(midpoint + QPoint(0, -TemplateAdjustActivity::cross_radius), midpoint + QPoint(0, TemplateAdjustActivity::cross_radius));
	painter->drawLine(midpoint + QPoint(-TemplateAdjustActivity::cross_radius, 0), midpoint + QPoint(TemplateAdjustActivity::cross_radius, 0));
}

int TemplateAdjustActivity::findHoverPoint(Template* temp, const QPoint& mouse_pos, MapWidget* widget, bool& point_src)
{
	bool adjusted = temp->isAdjustmentApplied();
	const float hover_distance_sq = 10*10;
	
	float minimum_distance_sq = 999999;
	int point_number = -1;
	
	for (int i = 0; i < temp->getNumPassPoints(); ++i)
	{
		auto point = temp->getPassPoint(i);
		
		QPointF display_pos_src = adjusted ? widget->mapToViewport(point->calculated_coords) : widget->mapToViewport(point->src_coords);
		float distance_sq = (display_pos_src.x() - mouse_pos.x())*(display_pos_src.x() - mouse_pos.x()) + (display_pos_src.y() - mouse_pos.y())*(display_pos_src.y() - mouse_pos.y());
		if (distance_sq < hover_distance_sq && distance_sq < minimum_distance_sq)
		{
			minimum_distance_sq = distance_sq;
			point_number = i;
			point_src = true;
		}
		
		QPointF display_pos_dest = widget->mapToViewport(point->dest_coords);
		distance_sq = (display_pos_dest.x() - mouse_pos.x())*(display_pos_dest.x() - mouse_pos.x()) + (display_pos_dest.y() - mouse_pos.y())*(display_pos_dest.y() - mouse_pos.y());
		if (distance_sq < hover_distance_sq && distance_sq <= minimum_distance_sq)	// NOTE: using <= here so dest positions have priority above other positions when at the same position
		{
			minimum_distance_sq = distance_sq;
			point_number = i;
			point_src = false;
		}
	}
	
	return point_number;
}

bool TemplateAdjustActivity::calculateTemplateAdjust(Template* temp, TemplateTransform& out, QWidget* dialog_parent)
{
	// Get original transformation
	if (temp->isAdjustmentApplied())
		temp->getOtherTransform(out);
	else
		temp->getTransform(out);
	
	if (!temp->getPassPointList().estimateSimilarityTransformation(&out))
	{
		QMessageBox::warning(dialog_parent, tr("Error"), tr("Failed to calculate adjustment!"));
		return false;
	}
	
	return true;
}

void TemplateAdjustActivity::templateChanged(int index, const Template* temp)
{
	Q_UNUSED(index);
	if (static_cast<Template*>(activity_object) == temp)
	{
		widget->updateDirtyRect(true);
		widget->updateAllRows();
	}
}
void TemplateAdjustActivity::templateDeleted(int index, const Template* temp)
{
	Q_UNUSED(index);
	if (static_cast<Template*>(activity_object) == temp)
		controller->setEditorActivity(nullptr);
}



// ### TemplateAdjustDockWidget ###

TemplateAdjustDockWidget::TemplateAdjustDockWidget(const QString& title, MapEditorController* controller, QWidget* parent)
: QDockWidget(title, parent)
, controller(controller)
{
	// nothing else
}

bool TemplateAdjustDockWidget::event(QEvent* event)
{
	if (event->type() == QEvent::ShortcutOverride && controller->getWindow()->shortcutsBlocked())
		event->accept();
    return QDockWidget::event(event);
}

void TemplateAdjustDockWidget::closeEvent(QCloseEvent* event)
{
	Q_UNUSED(event);
	emit closed();
	controller->setEditorActivity(nullptr);
}

TemplateAdjustWidget::TemplateAdjustWidget(Template* temp, MapEditorController* controller, QWidget* parent): QWidget(parent), temp(temp), controller(controller)
{
	react_to_changes = true;
	
	auto toolbar = new QToolBar();
	toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	toolbar->setFloatable(false);
	
	auto passpoint_label = new QLabel(tr("Pass points:"));
	
	new_act = new QAction(QIcon(QString::fromLatin1(":/images/cursor-georeferencing-add.png")), tr("New"), this);
	new_act->setCheckable(true);
	toolbar->addAction(new_act);

	move_act = new QAction(QIcon(QString::fromLatin1(":/images/move.png")), tr("Move"), this);
	move_act->setCheckable(true);
	toolbar->addAction(move_act);
	
	delete_act = new QAction(QIcon(QString::fromLatin1(":/images/delete.png")), tr("Delete"), this);
	delete_act->setCheckable(true);
	toolbar->addAction(delete_act);
	
	table = new QTableWidget(temp->getNumPassPoints(), 5);
	table->setEditTriggers(QAbstractItemView::AllEditTriggers);
	table->setSelectionBehavior(QAbstractItemView::SelectRows);
	table->setHorizontalHeaderLabels(QStringList() << tr("Template X") << tr("Template Y") << tr("Map X") << tr("Map Y") << tr("Error"));
	table->verticalHeader()->setVisible(false);
	
	auto header_view = table->horizontalHeader();
	for (int i = 0; i < 5; ++i)
		header_view->setSectionResizeMode(i, QHeaderView::ResizeToContents);
	header_view->setSectionsClickable(false);
	
	for (int i = 0; i < temp->getNumPassPoints(); ++i)
		addRow(i);
	
	apply_check = new QCheckBox(tr("Apply pass points"));
	apply_check->setChecked(temp->isAdjustmentApplied());
	auto help_button = new QPushButton(QIcon(QString::fromLatin1(":/images/help.png")), tr("Help"));
	clear_and_apply_button = new QPushButton(tr("Apply && clear all"));
	clear_and_revert_button = new QPushButton(tr("Clear all"));
	
	auto buttons_layout = new QHBoxLayout();
	buttons_layout->addWidget(help_button);
	buttons_layout->addStretch(1);
	buttons_layout->addWidget(clear_and_revert_button);
	buttons_layout->addWidget(clear_and_apply_button);

	
	auto layout = new QVBoxLayout();
	layout->addWidget(passpoint_label);
	layout->addWidget(toolbar);
	layout->addWidget(table, 1);
	layout->addWidget(apply_check);
	layout->addSpacing(16);
	layout->addLayout(buttons_layout);
	setLayout(layout);
	
	updateActions();
	
	connect(new_act, &QAction::triggered, this, &TemplateAdjustWidget::newClicked);
	connect(move_act, &QAction::triggered, this, &TemplateAdjustWidget::moveClicked);
	connect(delete_act, &QAction::triggered, this, &TemplateAdjustWidget::deleteClicked);
	
	connect(apply_check, &QAbstractButton::clicked, this, &TemplateAdjustWidget::applyClicked);
	connect(help_button, &QAbstractButton::clicked, this, &TemplateAdjustWidget::showHelp);
	connect(clear_and_apply_button, &QAbstractButton::clicked, this, &TemplateAdjustWidget::clearAndApplyClicked);
	connect(clear_and_revert_button, &QAbstractButton::clicked, this, &TemplateAdjustWidget::clearAndRevertClicked);
	
	updateDirtyRect();
}

TemplateAdjustWidget::~TemplateAdjustWidget() = default;



void TemplateAdjustWidget::addPassPoint(const MapCoordF& src, const MapCoordF& dest)
{
	bool adjusted = temp->isAdjustmentApplied();
	PassPoint new_point;
	
	if (adjusted)
	{
		MapCoordF src_coords_template = temp->mapToTemplate(src);
		new_point.src_coords = temp->templateToMapOther(src_coords_template);
	}
	else
	{
		new_point.src_coords = src;
	}
	new_point.dest_coords = dest;
	new_point.error = -1;
	
	int row = temp->getNumPassPoints();
	temp->addPassPoint(new_point, row);
	table->insertRow(row);
	addRow(row);
	
	temp->setAdjustmentDirty(true);
	if (adjusted)
	{
		TemplateTransform transformation;
		if (TemplateAdjustActivity::calculateTemplateAdjust(temp, transformation, this))
		{
			updatePointErrors();
			temp->setTransform(transformation);
			temp->setAdjustmentDirty(false);
		}
	}
	updateDirtyRect();
	updateActions();
}

void TemplateAdjustWidget::deletePassPoint(int number)
{
	Q_ASSERT(number >= 0 && number < temp->getNumPassPoints());
	
	temp->deletePassPoint(number);
	table->removeRow(number);
	
	temp->setAdjustmentDirty(true);
	if (temp->isAdjustmentApplied())
	{
		TemplateTransform transformation;
		if (TemplateAdjustActivity::calculateTemplateAdjust(temp, transformation, this))
		{
			updatePointErrors();
			temp->setTransform(transformation);
			temp->setAdjustmentDirty(false);
		}
	}
	updateDirtyRect(false);
	updateActions();
}

void TemplateAdjustWidget::stopTemplateAdjust()
{
	// If one of these is checked, the corresponding tool should be set. The last condition is just to be sure.
	if ((new_act->isChecked() || move_act->isChecked() || delete_act->isChecked()) && controller->getTool())
	{
		controller->setTool(nullptr);
	}
}

void TemplateAdjustWidget::updateActions()
{
	bool has_pass_points = temp->getNumPassPoints() > 0;
	
	clear_and_apply_button->setEnabled(has_pass_points);
	clear_and_revert_button->setEnabled(has_pass_points);
	move_act->setEnabled(has_pass_points);
	delete_act->setEnabled(has_pass_points);
	if (! has_pass_points)
	{
		if (move_act->isChecked())
		{
			move_act->setChecked(false);
			moveClicked(false);
		}
		if (delete_act->isChecked())
		{
			delete_act->setChecked(false);
			deleteClicked(false);
		}
	}
}

void TemplateAdjustWidget::newClicked(bool checked)
{
	if (checked)
		controller->setTool(new TemplateAdjustAddTool(controller, new_act, this));
	else
		new_act->setChecked(true);
}

void TemplateAdjustWidget::moveClicked(bool checked)
{
	if (checked)
		controller->setTool(new TemplateAdjustMoveTool(controller, move_act, this));
	else
		move_act->setChecked(true);
}

void TemplateAdjustWidget::deleteClicked(bool checked)
{
	if (checked)
		controller->setTool(new TemplateAdjustDeleteTool(controller, delete_act, this));
	else
		delete_act->setChecked(true);
}

void TemplateAdjustWidget::applyClicked(bool checked)
{
	if (checked)
	{
		if (temp->isAdjustmentDirty())
		{
			TemplateTransform transformation;
			if (!TemplateAdjustActivity::calculateTemplateAdjust(temp, transformation, this))
			{
				apply_check->setChecked(false);
				return;
			}
			updatePointErrors();
			temp->setOtherTransform(transformation);
			temp->setAdjustmentDirty(false);
		}
	}
	
	temp->switchTransforms();
	react_to_changes = false;
	controller->getMap()->emitTemplateChanged(temp);
	react_to_changes = true;
	updateDirtyRect();
}

void TemplateAdjustWidget::clearAndApplyClicked(bool checked)
{
	Q_UNUSED(checked);
	
	if (!temp->isAdjustmentApplied())
		applyClicked(true);
	
	clearPassPoints();
}

void TemplateAdjustWidget::clearAndRevertClicked(bool checked)
{
	Q_UNUSED(checked);
	
	if (temp->isAdjustmentApplied())
		applyClicked(false);
	
	clearPassPoints();
}

void TemplateAdjustWidget::showHelp()
{
	Util::showHelp(controller->getWindow(), "template_adjust.html");
}

void TemplateAdjustWidget::clearPassPoints()
{
	temp->clearPassPoints();
	apply_check->setChecked(false);
	table->clearContents();
	table->setRowCount(0);
	updateDirtyRect();
	updateActions();
}

void TemplateAdjustWidget::addRow(int row)
{
	react_to_changes = false;
	
	for (int i = 0; i < 4; ++i)
	{
		auto item = new QTableWidgetItem();
		item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);	// TODO: make editable    Qt::ItemIsEditable | 
		table->setItem(row, i, item);
	}
	
	auto item = new QTableWidgetItem();
	item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	table->setItem(row, 4, item);
	
	updateRow(row);
	
	react_to_changes = true;
}

void TemplateAdjustWidget::updatePointErrors()
{
	react_to_changes = false;
	
	for (int row = 0; row < temp->getNumPassPoints(); ++row)
	{
		PassPoint& point = *temp->getPassPoint(row);
		table->item(row, 4)->setText((point.error > 0) ? QString::number(point.error) : QString(QLatin1Char{'?'}));
	}
	
	react_to_changes = true;
}

void TemplateAdjustWidget::updateAllRows()
{
	if (!react_to_changes) return;
	for (int row = 0; row < temp->getNumPassPoints(); ++row)
		updateRow(row);
}

void TemplateAdjustWidget::updateRow(int row)
{
	react_to_changes = false;
	
	PassPoint& point = *temp->getPassPoint(row);
	MapCoordF src_coords_template;
	if (temp->isAdjustmentApplied())
		src_coords_template = temp->mapToTemplateOther(point.src_coords);
	else
		src_coords_template = temp->mapToTemplate(point.src_coords);
	
	table->item(row, 0)->setText(QString::number(src_coords_template.x()));
	table->item(row, 1)->setText(QString::number(src_coords_template.y()));
	table->item(row, 2)->setText(QString::number(point.dest_coords.x()));
	table->item(row, 3)->setText(QString::number(point.dest_coords.y()));
	table->item(row, 4)->setText((point.error > 0) ? QString::number(point.error) : QString(QLatin1Char{'?'}));
	
	react_to_changes = true;
}

void TemplateAdjustWidget::updateDirtyRect(bool redraw)
{
	bool adjusted = temp->isAdjustmentApplied();
	
	if (temp->getNumPassPoints() == 0)
		temp->getMap()->clearActivityBoundingBox();
	else
	{
		QRectF rect = QRectF(adjusted ? temp->getPassPoint(0)->calculated_coords : temp->getPassPoint(0)->src_coords, QSizeF(0, 0));
		rectInclude(rect, temp->getPassPoint(0)->dest_coords);
		for (int i = 1; i < temp->getNumPassPoints(); ++i)
		{
			rectInclude(rect, adjusted ? temp->getPassPoint(i)->calculated_coords : temp->getPassPoint(i)->src_coords);
			rectInclude(rect, temp->getPassPoint(i)->dest_coords);
		}
		temp->getMap()->setActivityBoundingBox(rect, TemplateAdjustActivity::cross_radius, redraw);
	}
}



// ### TemplateAdjustEditTool ###

TemplateAdjustEditTool::TemplateAdjustEditTool(MapEditorController* editor, QAction* tool_button, TemplateAdjustWidget* widget): MapEditorTool(editor, Other, tool_button), widget(widget)
{
	active_point = -1;
}

void TemplateAdjustEditTool::draw(QPainter* painter, MapWidget* widget)
{
	bool adjusted = this->widget->getTemplate()->isAdjustmentApplied();
	
	if (active_point >= 0)
	{
		auto point = this->widget->getTemplate()->getPassPoint(active_point);
		MapCoordF position = active_point_is_src ? (adjusted ? point->calculated_coords : point->src_coords) : point->dest_coords;
		QPoint viewport_pos = widget->mapToViewport(position).toPoint();
		
		painter->setPen(active_point_is_src ? Qt::red : Qt::green);
		painter->setBrush(Qt::NoBrush);
		painter->drawRect(viewport_pos.x() - TemplateAdjustActivity::cross_radius, viewport_pos.y() - TemplateAdjustActivity::cross_radius,
						  2*TemplateAdjustActivity::cross_radius, 2*TemplateAdjustActivity::cross_radius);
	}
}

void TemplateAdjustEditTool::findHoverPoint(const QPoint& mouse_pos, MapWidget* map_widget)
{
	bool adjusted = this->widget->getTemplate()->isAdjustmentApplied();
	bool new_active_point_is_src;
	int new_active_point = TemplateAdjustActivity::findHoverPoint(this->widget->getTemplate(), mouse_pos, map_widget, new_active_point_is_src);
	
	if (new_active_point != active_point || (new_active_point >= 0 && new_active_point_is_src != active_point_is_src))
	{
		// Hovering over a different point than before (or none)
		active_point = new_active_point;
		active_point_is_src = new_active_point_is_src;
		
		if (active_point >= 0)
		{
			auto point = this->widget->getTemplate()->getPassPoint(active_point);
			if (active_point_is_src)
			{
				if (adjusted)
					map()->setDrawingBoundingBox(QRectF(point->calculated_coords.x(), point->calculated_coords.y(), 0, 0), TemplateAdjustActivity::cross_radius);
				else
					map()->setDrawingBoundingBox(QRectF(point->src_coords.x(), point->src_coords.y(), 0, 0), TemplateAdjustActivity::cross_radius);
			}
			else
				map()->setDrawingBoundingBox(QRectF(point->dest_coords.x(), point->dest_coords.y(), 0, 0), TemplateAdjustActivity::cross_radius);
		}
		else
			map()->clearDrawingBoundingBox();
	}
}



// ### TemplateAdjustAddTool ###

TemplateAdjustAddTool::TemplateAdjustAddTool(MapEditorController* editor, QAction* tool_button, TemplateAdjustWidget* widget): MapEditorTool(editor, Other, tool_button), widget(widget)
{
	first_point_set = false;
}

void TemplateAdjustAddTool::init()
{
	// NOTE: this is called by other methods to set this text again. Change that behavior if adding stuff here
	setStatusBarText(tr("<b>Click</b>: Set the template position of the pass point. "));
	
	MapEditorTool::init();
}

const QCursor& TemplateAdjustAddTool::getCursor() const
{
	static auto const cursor = scaledToScreen(QCursor{ QPixmap(QString::fromLatin1(":/images/cursor-georeferencing-add.png")), 11, 11 });
	return cursor;
}

bool TemplateAdjustAddTool::mousePressEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget)
{
	Q_UNUSED(widget);
	
	if (event->button() != Qt::LeftButton)
		return false;
	
	if (!first_point_set)
	{
		first_point = map_coord;
		mouse_pos = map_coord;
		first_point_set = true;
		setDirtyRect(map_coord);
		
		setStatusBarText(tr("<b>Click</b>: Set the map position of the pass point. ") +
		                 OpenOrienteering::MapEditorTool::tr("<b>%1</b>: Abort. ").arg(ModifierKey::escape()) );
	}
	else
	{
		this->widget->addPassPoint(first_point, map_coord);
		
		first_point_set = false;
		map()->clearDrawingBoundingBox();
		
		init();
	}
	
	return true;
}
bool TemplateAdjustAddTool::mouseMoveEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget)
{
	Q_UNUSED(event);
	Q_UNUSED(widget);
	
	if (first_point_set)
	{
		mouse_pos = map_coord;
		setDirtyRect(map_coord);
	}
	
    return true;
}
bool TemplateAdjustAddTool::keyPressEvent(QKeyEvent* event)
{
	if (first_point_set && event->key() == Qt::Key_Escape)
	{
		first_point_set = false;
		map()->clearDrawingBoundingBox();
		
		init();
		return true;
	}
    return false;
}

void TemplateAdjustAddTool::draw(QPainter* painter, MapWidget* widget)
{
	if (first_point_set)
	{
		QPointF start = widget->mapToViewport(first_point);
		QPointF end = widget->mapToViewport(mouse_pos);
		
		TemplateAdjustActivity::drawCross(painter, start.toPoint(), Qt::red);
		
		MapCoordF to_end = MapCoordF((end - start).x(), (end - start).y());
		if (to_end.lengthSquared() > 3*3)
		{
			auto length = to_end.length();
			to_end *= (length - 3) / length;
			painter->drawLine(start, start + to_end);
		}
	}
}

void TemplateAdjustAddTool::setDirtyRect(const MapCoordF& mouse_pos)
{
	QRectF rect = QRectF(first_point.x(), first_point.y(), 0, 0);
	rectInclude(rect, mouse_pos);
	map()->setDrawingBoundingBox(rect, TemplateAdjustActivity::cross_radius);
}



// ### TemplateAdjustMoveTool ###

QCursor* TemplateAdjustMoveTool::cursor = nullptr;
QCursor* TemplateAdjustMoveTool::cursor_invisible = nullptr;

TemplateAdjustMoveTool::TemplateAdjustMoveTool(MapEditorController* editor, QAction* tool_button, TemplateAdjustWidget* widget): TemplateAdjustEditTool(editor, tool_button, widget)
{
	dragging = false;
	
	if (!cursor)
	{
		cursor = new QCursor(QPixmap(QString::fromLatin1(":/images/cursor-georeferencing-move.png")), 1, 1);
		cursor_invisible = new QCursor(QPixmap(QString::fromLatin1(":/images/cursor-invisible.png")), 0, 0);
	}
}

void TemplateAdjustMoveTool::init()
{
	setStatusBarText(tr("<b>Drag</b>: Move pass points. "));
	
	MapEditorTool::init();
}

const QCursor& TemplateAdjustMoveTool::getCursor() const
{
	return *cursor;
}

bool TemplateAdjustMoveTool::mousePressEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget)
{
	if (event->button() != Qt::LeftButton)
		return false;
	
	bool adjusted = this->widget->getTemplate()->isAdjustmentApplied();
	
	active_point = TemplateAdjustActivity::findHoverPoint(this->widget->getTemplate(), event->pos(), widget, active_point_is_src);
	if (active_point >= 0)
	{
		auto point = this->widget->getTemplate()->getPassPoint(active_point);
		MapCoordF* point_coords;
		if (active_point_is_src)
		{
			if (adjusted)
				point_coords = &point->calculated_coords;
			else
				point_coords = &point->src_coords;
		}
		else
			point_coords = &point->dest_coords;
		
		dragging = true;
		dragging_offset = MapCoordF(point_coords->x() - map_coord.x(), point_coords->y() - map_coord.y());
		
		widget->setCursor(*cursor_invisible);
	}
	
	return false;
}

bool TemplateAdjustMoveTool::mouseMoveEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget)
{
	if (!dragging)
		findHoverPoint(event->pos(), widget);
	else
		setActivePointPosition(map_coord);
	
	return true;
}

bool TemplateAdjustMoveTool::mouseReleaseEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget)
{
	Q_UNUSED(event);
	
	auto temp = this->widget->getTemplate();
	
	if (dragging)
	{
		setActivePointPosition(map_coord);
		
		if (temp->isAdjustmentApplied())
		{
			TemplateTransform transformation;
			if (TemplateAdjustActivity::calculateTemplateAdjust(temp, transformation, this->widget))
			{
				this->widget->updatePointErrors();
				this->widget->updateDirtyRect();
				temp->setTransform(transformation);
				temp->setAdjustmentDirty(false);
			}
		}
		
		dragging = false;
		widget->setCursor(*cursor);
	}
	return false;
}

void TemplateAdjustMoveTool::setActivePointPosition(const MapCoordF& map_coord)
{
	bool adjusted = this->widget->getTemplate()->isAdjustmentApplied();
	
	auto point = this->widget->getTemplate()->getPassPoint(active_point);
	MapCoordF* changed_coords;
	if (active_point_is_src)
	{
		if (adjusted)
			changed_coords = &point->calculated_coords;
		else
			changed_coords = &point->src_coords;
	}
	else
		changed_coords = &point->dest_coords;
	QRectF changed_rect = QRectF(adjusted ? point->calculated_coords : point->src_coords, QSizeF(0, 0));
	rectInclude(changed_rect, point->dest_coords);
	rectInclude(changed_rect, map_coord);
	
	*changed_coords = MapCoordF(map_coord.x() + dragging_offset.x(), map_coord.y() + dragging_offset.y());
	if (active_point_is_src)
	{
		if (adjusted)
		{
			MapCoordF src_coords_template = this->widget->getTemplate()->mapToTemplate(point->calculated_coords);
			point->src_coords = this->widget->getTemplate()->templateToMapOther(src_coords_template);
		}
	}
	point->error = -1;
	
	this->widget->updateRow(active_point);
	
	map()->setDrawingBoundingBox(QRectF(changed_coords->x(), changed_coords->y(), 0, 0), TemplateAdjustActivity::cross_radius + 1, false);
	map()->updateDrawing(changed_rect, TemplateAdjustActivity::cross_radius + 1);
	widget->updateDirtyRect();
	
	this->widget->getTemplate()->setAdjustmentDirty(true);
}



// ### TemplateAdjustDeleteTool ###

TemplateAdjustDeleteTool::TemplateAdjustDeleteTool(MapEditorController* editor, QAction* tool_button, TemplateAdjustWidget* widget): TemplateAdjustEditTool(editor, tool_button, widget)
{
	// nothing
}

void TemplateAdjustDeleteTool::init()
{
	setStatusBarText(tr("<b>Click</b>: Delete pass points. "));
	
	MapEditorTool::init();
}

const QCursor& TemplateAdjustDeleteTool::getCursor() const
{
	static auto const cursor = scaledToScreen(QCursor{ QPixmap(QString::fromLatin1(":/images/cursor-delete.png")), 1, 1});
	return cursor;
}

bool TemplateAdjustDeleteTool::mousePressEvent(QMouseEvent* event, const MapCoordF& /*map_coord*/, MapWidget* widget)
{
	if (event->button() != Qt::LeftButton)
		return false;
	
	bool adjusted = this->widget->getTemplate()->isAdjustmentApplied();
	
	active_point = TemplateAdjustActivity::findHoverPoint(this->widget->getTemplate(), event->pos(), widget, active_point_is_src);
	if (active_point >= 0)
	{
		auto point = this->widget->getTemplate()->getPassPoint(active_point);
		QRectF changed_rect = QRectF(adjusted ? point->calculated_coords : point->src_coords, QSizeF(0, 0));
		rectInclude(changed_rect, point->dest_coords);
		
		this->widget->deletePassPoint(active_point);
		findHoverPoint(event->pos(), widget);
		map()->updateDrawing(changed_rect, TemplateAdjustActivity::cross_radius + 1);
	}
	return true;
}

bool TemplateAdjustDeleteTool::mouseMoveEvent(QMouseEvent* event, const MapCoordF& /*map_coord*/, MapWidget* widget)
{
	findHoverPoint(event->pos(), widget);
	return true;
}


}  // namespace OpenOrienteering
