/*
 *    Copyright 2012, 2013 Thomas Schöps
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

#include <QtWidgets>

#include "gui/main_window.h"
#include "map_editor.h"
#include "map_widget.h"
#include "template.h"
#include "transformation.h"
#include "util.h"
#include "gui/modifier_key.h"

float TemplateAdjustActivity::cross_radius = 4;

TemplateAdjustActivity::TemplateAdjustActivity(Template* temp, MapEditorController* controller) : controller(controller)
{
	setActivityObject(temp);
	connect(controller->getMap(), SIGNAL(templateChanged(int,Template*)), this, SLOT(templateChanged(int,Template*)));
	connect(controller->getMap(), SIGNAL(templateDeleted(int,Template*)), this, SLOT(templateDeleted(int,Template*)));
}
TemplateAdjustActivity::~TemplateAdjustActivity()
{
	widget->stopTemplateAdjust();
	delete dock;
}

void TemplateAdjustActivity::init()
{
	Template* temp = reinterpret_cast<Template*>(activity_object);
	
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
	Template* temp = reinterpret_cast<Template*>(activity_object);
	bool adjusted = temp->isAdjustmentApplied();
    
	for (int i = 0; i < temp->getNumPassPoints(); ++i)
	{
		PassPoint* point = temp->getPassPoint(i);
		QPointF start = widget->mapToViewport(adjusted ? point->calculated_coords : point->src_coords);
		QPointF end = widget->mapToViewport(point->dest_coords);
		
		drawCross(painter, start.toPoint(), QColor(Qt::red));
		painter->drawLine(start, end);
		drawCross(painter, end.toPoint(), QColor(Qt::green));
	}
}
void TemplateAdjustActivity::drawCross(QPainter* painter, QPoint midpoint, QColor color)
{
	painter->setPen(color);
	painter->drawLine(midpoint + QPoint(0, -TemplateAdjustActivity::cross_radius), midpoint + QPoint(0, TemplateAdjustActivity::cross_radius));
	painter->drawLine(midpoint + QPoint(-TemplateAdjustActivity::cross_radius, 0), midpoint + QPoint(TemplateAdjustActivity::cross_radius, 0));
}

int TemplateAdjustActivity::findHoverPoint(Template* temp, QPoint mouse_pos, MapWidget* widget, bool& point_src)
{
	bool adjusted = temp->isAdjustmentApplied();
	const float hover_distance_sq = 10*10;
	
	float minimum_distance_sq = 999999;
	int point_number = -1;
	
	for (int i = 0; i < temp->getNumPassPoints(); ++i)
	{
		PassPoint* point = temp->getPassPoint(i);
		
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

void TemplateAdjustActivity::templateChanged(int index, Template* temp)
{
	Q_UNUSED(index);
	if ((Template*)activity_object == temp)
	{
		widget->updateDirtyRect(true);
		widget->updateAllRows();
	}
}
void TemplateAdjustActivity::templateDeleted(int index, Template* temp)
{
	Q_UNUSED(index);
	if ((Template*)activity_object == temp)
		controller->setEditorActivity(NULL);
}

// ### TemplateAdjustDockWidget ###

TemplateAdjustDockWidget::TemplateAdjustDockWidget(const QString title, MapEditorController* controller, QWidget* parent): QDockWidget(title, parent), controller(controller)
{
}
bool TemplateAdjustDockWidget::event(QEvent* event)
{
	if (event->type() == QEvent::ShortcutOverride && controller->getWindow()->areShortcutsDisabled())
		event->accept();
    return QDockWidget::event(event);
}
void TemplateAdjustDockWidget::closeEvent(QCloseEvent* event)
{
	Q_UNUSED(event);
	emit(closed());
	controller->setEditorActivity(NULL);
}

TemplateAdjustWidget::TemplateAdjustWidget(Template* temp, MapEditorController* controller, QWidget* parent): QWidget(parent), temp(temp), controller(controller)
{
	react_to_changes = true;
	
	QToolBar* toolbar = new QToolBar();
	toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	toolbar->setFloatable(false);
	
	QLabel* passpoint_label = new QLabel(tr("Pass points:"));
	
	new_act = new QAction(QIcon(":/images/cursor-georeferencing-add.png"), tr("New"), this);
	new_act->setCheckable(true);
	toolbar->addAction(new_act);

	move_act = new QAction(QIcon(":/images/move.png"), tr("Move"), this);
	move_act->setCheckable(true);
	toolbar->addAction(move_act);
	
	delete_act = new QAction(QIcon(":/images/delete.png"), tr("Delete"), this);
	delete_act->setCheckable(true);
	toolbar->addAction(delete_act);
	
	table = new QTableWidget(temp->getNumPassPoints(), 5);
	table->setEditTriggers(QAbstractItemView::AllEditTriggers);
	table->setSelectionBehavior(QAbstractItemView::SelectRows);
	table->setHorizontalHeaderLabels(QStringList() << tr("Template X") << tr("Template Y") << tr("Map X") << tr("Map Y") << tr("Error"));
	table->verticalHeader()->setVisible(false);
	
	QHeaderView* header_view = table->horizontalHeader();
	for (int i = 0; i < 5; ++i)
		header_view->setSectionResizeMode(i, QHeaderView::ResizeToContents);
	header_view->setSectionsClickable(false);
	
	for (int i = 0; i < temp->getNumPassPoints(); ++i)
		addRow(i);
	
	apply_check = new QCheckBox(tr("Apply pass points"));
	apply_check->setChecked(temp->isAdjustmentApplied());
	QPushButton* help_button = new QPushButton(QIcon(":/images/help.png"), tr("Help"));
	clear_and_apply_button = new QPushButton(tr("Apply && clear all"));
	clear_and_revert_button = new QPushButton(tr("Clear all"));
	
	QHBoxLayout* buttons_layout = new QHBoxLayout();
	buttons_layout->addWidget(help_button);
	buttons_layout->addStretch(1);
	buttons_layout->addWidget(clear_and_revert_button);
	buttons_layout->addWidget(clear_and_apply_button);

	
	QVBoxLayout* layout = new QVBoxLayout();
	layout->addWidget(passpoint_label);
	layout->addWidget(toolbar);
	layout->addWidget(table, 1);
	layout->addWidget(apply_check);
	layout->addSpacing(16);
	layout->addLayout(buttons_layout);
	setLayout(layout);
	
	updateActions();
	
	connect(new_act, SIGNAL(triggered(bool)), this, SLOT(newClicked(bool)));
	connect(move_act, SIGNAL(triggered(bool)), this, SLOT(moveClicked(bool)));
	connect(delete_act, SIGNAL(triggered(bool)), this, SLOT(deleteClicked(bool)));
	
	connect(apply_check, SIGNAL(clicked(bool)), this, SLOT(applyClicked(bool)));
	connect(help_button, SIGNAL(clicked(bool)), this, SLOT(showHelp()));
	connect(clear_and_apply_button, SIGNAL(clicked(bool)), this, SLOT(clearAndApplyClicked(bool)));
	connect(clear_and_revert_button, SIGNAL(clicked(bool)), this, SLOT(clearAndRevertClicked(bool)));
	
	updateDirtyRect();
}
TemplateAdjustWidget::~TemplateAdjustWidget()
{

}

void TemplateAdjustWidget::addPassPoint(MapCoordF src, MapCoordF dest)
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
	assert(number >= 0 && number < temp->getNumPassPoints());
	
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
		// Set the tool's button to NULL, so on later deletion the tool does not
		// try to unset the button, which has already been deleted at that time
		controller->getTool()->setAction(NULL);
		controller->setTool(NULL);
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
		QTableWidgetItem* item = new QTableWidgetItem();
		item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);	// TODO: make editable    Qt::ItemIsEditable | 
		table->setItem(row, i, item);
	}
	
	QTableWidgetItem* item = new QTableWidgetItem();
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
		table->item(row, 4)->setText((point.error > 0) ? QString::number(point.error) : "?");
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
	
	table->item(row, 0)->setText(QString::number(src_coords_template.getX()));
	table->item(row, 1)->setText(QString::number(src_coords_template.getY()));
	table->item(row, 2)->setText(QString::number(point.dest_coords.getX()));
	table->item(row, 3)->setText(QString::number(point.dest_coords.getY()));
	table->item(row, 4)->setText((point.error > 0) ? QString::number(point.error) : "?");
	
	react_to_changes = true;
}

void TemplateAdjustWidget::updateDirtyRect(bool redraw)
{
	bool adjusted = temp->isAdjustmentApplied();
	
	if (temp->getNumPassPoints() == 0)
		temp->getMap()->clearActivityBoundingBox();
	else
	{
		QRectF rect = QRectF(adjusted ? temp->getPassPoint(0)->calculated_coords.toQPointF() : temp->getPassPoint(0)->src_coords.toQPointF(), QSizeF(0, 0));
		rectInclude(rect, temp->getPassPoint(0)->dest_coords.toQPointF());
		for (int i = 1; i < temp->getNumPassPoints(); ++i)
		{
			rectInclude(rect, adjusted ? temp->getPassPoint(i)->calculated_coords.toQPointF() : temp->getPassPoint(i)->src_coords.toQPointF());
			rectInclude(rect, temp->getPassPoint(i)->dest_coords.toQPointF());
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
		PassPoint* point = this->widget->getTemplate()->getPassPoint(active_point);
		MapCoordF position = active_point_is_src ? (adjusted ? point->calculated_coords : point->src_coords) : point->dest_coords;
		QPoint viewport_pos = widget->mapToViewport(position).toPoint();
		
		painter->setPen(active_point_is_src ? Qt::red : Qt::green);
		painter->setBrush(Qt::NoBrush);
		painter->drawRect(viewport_pos.x() - TemplateAdjustActivity::cross_radius, viewport_pos.y() - TemplateAdjustActivity::cross_radius,
						  2*TemplateAdjustActivity::cross_radius, 2*TemplateAdjustActivity::cross_radius);
	}
}

void TemplateAdjustEditTool::findHoverPoint(QPoint mouse_pos, MapWidget* map_widget)
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
			PassPoint* point = this->widget->getTemplate()->getPassPoint(active_point);
			if (active_point_is_src)
			{
				if (adjusted)
					editor->getMap()->setDrawingBoundingBox(QRectF(point->calculated_coords.getX(), point->calculated_coords.getY(), 0, 0), TemplateAdjustActivity::cross_radius);
				else
					editor->getMap()->setDrawingBoundingBox(QRectF(point->src_coords.getX(), point->src_coords.getY(), 0, 0), TemplateAdjustActivity::cross_radius);
			}
			else
				editor->getMap()->setDrawingBoundingBox(QRectF(point->dest_coords.getX(), point->dest_coords.getY(), 0, 0), TemplateAdjustActivity::cross_radius);
		}
		else
			editor->getMap()->clearDrawingBoundingBox();
	}
}

// ### TemplateAdjustAddTool ###

QCursor* TemplateAdjustAddTool::cursor = NULL;

TemplateAdjustAddTool::TemplateAdjustAddTool(MapEditorController* editor, QAction* tool_button, TemplateAdjustWidget* widget): MapEditorTool(editor, Other, tool_button), widget(widget)
{
	first_point_set = false;
	
	if (!cursor)
		cursor = new QCursor(QPixmap(":/images/cursor-georeferencing-add.png"), 11, 11);
}
void TemplateAdjustAddTool::init()
{
	// NOTE: this is called by other methods to set this text again. Change that behavior if adding stuff here
	setStatusBarText(tr("<b>Click</b>: Set the template position of the pass point. "));
}

bool TemplateAdjustAddTool::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
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
		                 MapEditorTool::tr("<b>%1</b>: Abort. ").arg(ModifierKey::escape()) );
	}
	else
	{
		this->widget->addPassPoint(first_point, map_coord);
		
		first_point_set = false;
		editor->getMap()->clearDrawingBoundingBox();
		
		init();
	}
	
	return true;
}
bool TemplateAdjustAddTool::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
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
		editor->getMap()->clearDrawingBoundingBox();
		
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
			double length = to_end.length();
			to_end.setX(to_end.getX() * ((length - 3) / length));
			to_end.setY(to_end.getY() * ((length - 3) / length));
			painter->drawLine(start, start + QPointF(to_end.getX(), to_end.getY()));
		}
	}
}

void TemplateAdjustAddTool::setDirtyRect(MapCoordF mouse_pos)
{
	QRectF rect = QRectF(first_point.getX(), first_point.getY(), 0, 0);
	rectInclude(rect, mouse_pos.toQPointF());
	editor->getMap()->setDrawingBoundingBox(rect, TemplateAdjustActivity::cross_radius);
}

// ### TemplateAdjustMoveTool ###

QCursor* TemplateAdjustMoveTool::cursor = NULL;
QCursor* TemplateAdjustMoveTool::cursor_invisible = NULL;

TemplateAdjustMoveTool::TemplateAdjustMoveTool(MapEditorController* editor, QAction* tool_button, TemplateAdjustWidget* widget): TemplateAdjustEditTool(editor, tool_button, widget)
{
	dragging = false;
	
	if (!cursor)
	{
		cursor = new QCursor(QPixmap(":/images/cursor-georeferencing-move.png"), 1, 1);
		cursor_invisible = new QCursor(QPixmap(":/images/cursor-invisible.png"), 0, 0);
	}
}
void TemplateAdjustMoveTool::init()
{
	setStatusBarText(tr("<b>Drag</b>: Move pass points. "));
}

bool TemplateAdjustMoveTool::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (event->button() != Qt::LeftButton)
		return false;
	
	bool adjusted = this->widget->getTemplate()->isAdjustmentApplied();
	
	active_point = TemplateAdjustActivity::findHoverPoint(this->widget->getTemplate(), event->pos(), widget, active_point_is_src);
	if (active_point >= 0)
	{
		PassPoint* point = this->widget->getTemplate()->getPassPoint(active_point);
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
		dragging_offset = MapCoordF(point_coords->getX() - map_coord.getX(), point_coords->getY() - map_coord.getY());
		
		widget->setCursor(*cursor_invisible);
	}
	
	return false;
}
bool TemplateAdjustMoveTool::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (!dragging)
		findHoverPoint(event->pos(), widget);
	else
		setActivePointPosition(map_coord);
	
	return true;
}
bool TemplateAdjustMoveTool::mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	Q_UNUSED(event);
	
	Template* temp = this->widget->getTemplate();
	
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

void TemplateAdjustMoveTool::setActivePointPosition(MapCoordF map_coord)
{
	bool adjusted = this->widget->getTemplate()->isAdjustmentApplied();
	
	PassPoint* point = this->widget->getTemplate()->getPassPoint(active_point);
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
	QRectF changed_rect = QRectF(adjusted ? point->calculated_coords.toQPointF() : point->src_coords.toQPointF(), QSizeF(0, 0));
	rectInclude(changed_rect, point->dest_coords.toQPointF());
	rectInclude(changed_rect, map_coord.toQPointF());
	
	*changed_coords = MapCoordF(map_coord.getX() + dragging_offset.getX(), map_coord.getY() + dragging_offset.getY());
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
	
	editor->getMap()->setDrawingBoundingBox(QRectF(changed_coords->getX(), changed_coords->getY(), 0, 0), TemplateAdjustActivity::cross_radius + 1, false);
	editor->getMap()->updateDrawing(changed_rect, TemplateAdjustActivity::cross_radius + 1);
	widget->updateDirtyRect();
	
	this->widget->getTemplate()->setAdjustmentDirty(true);
}

// ### TemplateAdjustDeleteTool ###

QCursor* TemplateAdjustDeleteTool::cursor = NULL;

TemplateAdjustDeleteTool::TemplateAdjustDeleteTool(MapEditorController* editor, QAction* tool_button, TemplateAdjustWidget* widget): TemplateAdjustEditTool(editor, tool_button, widget)
{
	if (!cursor)
		cursor = new QCursor(QPixmap(":/images/cursor-delete.png"), 1, 1);
}

void TemplateAdjustDeleteTool::init()
{
	setStatusBarText(tr("<b>Click</b>: Delete pass points. "));
}

bool TemplateAdjustDeleteTool::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	Q_UNUSED(map_coord);
	
	if (event->button() != Qt::LeftButton)
		return false;
	
	bool adjusted = this->widget->getTemplate()->isAdjustmentApplied();
	
	active_point = TemplateAdjustActivity::findHoverPoint(this->widget->getTemplate(), event->pos(), widget, active_point_is_src);
	if (active_point >= 0)
	{
		PassPoint* point = this->widget->getTemplate()->getPassPoint(active_point);
		QRectF changed_rect = QRectF(adjusted ? point->calculated_coords.toQPointF() : point->src_coords.toQPointF(), QSizeF(0, 0));
		rectInclude(changed_rect, point->dest_coords.toQPointF());
		
		this->widget->deletePassPoint(active_point);
		findHoverPoint(event->pos(), widget);
		editor->getMap()->updateDrawing(changed_rect, TemplateAdjustActivity::cross_radius + 1);
	}
	return true;
}
bool TemplateAdjustDeleteTool::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	Q_UNUSED(map_coord);
	
	findHoverPoint(event->pos(), widget);
	return true;
}
