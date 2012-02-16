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


#include "georeferencing.h"

#include <QtGui>

#include "template.h"
#include "util.h"
#include "map_widget.h"

float GeoreferencingActivity::cross_radius = 4;

GeoreferencingActivity::GeoreferencingActivity(Template* temp, MapEditorController* controller) : controller(controller)
{
	setActivityObject(temp);
}
GeoreferencingActivity::~GeoreferencingActivity()
{
	widget->stopGeoreferencing();
	delete dock;
}

void GeoreferencingActivity::init()
{
	Template* temp = reinterpret_cast<Template*>(activity_object);
	
	dock = new GeoreferencingDockWidget(tr("Georeferencing"), controller, controller->getWindow());
	widget = new GeoreferencingWidget(temp, controller, dock);
	dock->setWidget(widget);
	
	// Show dock in floating state
	dock->setFloating(true);
	dock->show();
	dock->setGeometry(controller->getWindow()->geometry().left() + 40, controller->getWindow()->geometry().top() + 100, dock->width(), dock->height());
}

void GeoreferencingActivity::draw(QPainter* painter, MapWidget* widget)
{
	Template* temp = reinterpret_cast<Template*>(activity_object);
	bool georef = temp->isGeoreferencingApplied();
    
	for (int i = 0; i < temp->getNumPassPoints(); ++i)
	{
		Template::PassPoint* point = temp->getPassPoint(i);
		QPointF start = widget->mapToViewport(georef ? point->calculated_coords_map : point->src_coords_map);
		QPointF end = widget->mapToViewport(point->dest_coords_map);
		
		drawCross(painter, start.toPoint(), QColor(Qt::red));
		painter->drawLine(start, end);
		drawCross(painter, end.toPoint(), QColor(Qt::green));
	}
}
void GeoreferencingActivity::drawCross(QPainter* painter, QPoint midpoint, QColor color)
{
	painter->setPen(color);
	painter->drawLine(midpoint + QPoint(0, -GeoreferencingActivity::cross_radius), midpoint + QPoint(0, GeoreferencingActivity::cross_radius));
	painter->drawLine(midpoint + QPoint(-GeoreferencingActivity::cross_radius, 0), midpoint + QPoint(GeoreferencingActivity::cross_radius, 0));
}

int GeoreferencingActivity::findHoverPoint(Template* temp, QPoint mouse_pos, MapWidget* widget, bool& point_src)
{
	bool georef = temp->isGeoreferencingApplied();
	const float hover_distance_sq = 10*10;
	
	float minimum_distance_sq = 999999;
	int point_number = -1;
	
	for (int i = 0; i < temp->getNumPassPoints(); ++i)
	{
		Template::PassPoint* point = temp->getPassPoint(i);
		
		QPointF display_pos_src = georef ? widget->mapToViewport(point->calculated_coords_map) : widget->mapToViewport(point->src_coords_map);
		float distance_sq = (display_pos_src.x() - mouse_pos.x())*(display_pos_src.x() - mouse_pos.x()) + (display_pos_src.y() - mouse_pos.y())*(display_pos_src.y() - mouse_pos.y());
		if (distance_sq < hover_distance_sq && distance_sq < minimum_distance_sq)
		{
			minimum_distance_sq = distance_sq;
			point_number = i;
			point_src = true;
		}
		
		QPointF display_pos_dest = widget->mapToViewport(point->dest_coords_map);
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

bool GeoreferencingActivity::calculateGeoreferencing(Template* temp, Template::TemplateTransform& out, QWidget* dialog_parent)
{
	int num_pass_points = temp->getNumPassPoints();
	if (temp->isGeoreferencingApplied())	// get original transformation
		temp->getOtherTransform(out);
	else
		temp->getTransform(out);
	
	if (num_pass_points == 1)
	{
		Template::PassPoint* point = temp->getPassPoint(0);
		MapCoordF offset = MapCoordF(point->dest_coords_map.getX() - point->src_coords_map.getX(), point->dest_coords_map.getY() - point->src_coords_map.getY());
		
		out.template_x += qRound64(1000 * offset.getX());
		out.template_y += qRound64(1000 * offset.getY());
		point->calculated_coords_map = point->dest_coords_map;
		point->error = 0;
	}
	else if (num_pass_points >= 2)
	{
		// Create linear equation system and solve using the pseuo inverse
		Matrix mat(2*num_pass_points, 4);
		Matrix values(2*num_pass_points, 1);
		for (int i = 0; i < num_pass_points; ++i)
		{
			Template::PassPoint* point = temp->getPassPoint(i);
			mat.set(2*i, 0, point->src_coords_map.getX());
			mat.set(2*i, 1, point->src_coords_map.getY());
			mat.set(2*i, 2, 1);
			mat.set(2*i, 3, 0);
			mat.set(2*i+1, 0, point->src_coords_map.getY());
			mat.set(2*i+1, 1, -point->src_coords_map.getX());
			mat.set(2*i+1, 2, 0);
			mat.set(2*i+1, 3, 1);
			
			values.set(2*i, 0, point->dest_coords_map.getX());
			values.set(2*i+1, 0, point->dest_coords_map.getY());
		}
		
		Matrix transposed;
		mat.transpose(transposed);
		
		Matrix mat_temp, mat_temp2, pseudo_inverse;
		transposed.multiply(mat, mat_temp);
		if (!mat_temp.invert(mat_temp2))
		{
			QMessageBox::warning(dialog_parent, tr("Error"), tr("Failed to calculate georeferencing!"));
			return false;
		}
		mat_temp2.multiply(transposed, pseudo_inverse);
		
		// Calculate transformation parameters
		Matrix output;
		pseudo_inverse.multiply(values, output);
		
		double move_x = output.get(2, 0);
		double move_y = output.get(3, 0);
		double rotation = qAtan2((-1) * output.get(1, 0), output.get(0, 0));
		double scale = output.get(0, 0) / qCos(rotation);
		
		// Calculate transformation matrix
		double cosr = cos(rotation);
		double sinr = sin(rotation);
		
		Matrix trans_change(3, 3);
		trans_change.set(0, 0, scale * cosr);
		trans_change.set(0, 1, scale * (-sinr));
		trans_change.set(1, 0, scale * sinr);
		trans_change.set(1, 1, scale * cosr);
		trans_change.set(0, 2, move_x);
		trans_change.set(1, 2, move_y);
		
		// Transform the original transformation parameters to get the new transformation
		out.template_scale_x *= scale;
		out.template_scale_y *= scale;
		out.template_rotation -= rotation;
		qint64 temp_x = qRound64(1000.0 * (trans_change.get(0, 0) * (out.template_x/1000.0) + trans_change.get(0, 1) * (out.template_y/1000.0) + trans_change.get(0, 2)));
		out.template_y = qRound64(1000.0 * (trans_change.get(1, 0) * (out.template_x/1000.0) + trans_change.get(1, 1) * (out.template_y/1000.0) + trans_change.get(1, 2)));
		out.template_x = temp_x;
		
		// Transform the pass points and calculate error
		for (int i = 0; i < num_pass_points; ++i)
		{
			Template::PassPoint* point = temp->getPassPoint(i);
			
			point->calculated_coords_map = MapCoordF(trans_change.get(0, 0) * point->src_coords_map.getX() + trans_change.get(0, 1) * point->src_coords_map.getY() + trans_change.get(0, 2),
													 trans_change.get(1, 0) * point->src_coords_map.getX() + trans_change.get(1, 1) * point->src_coords_map.getY() + trans_change.get(1, 2));
			point->error = point->calculated_coords_map.lengthTo(point->dest_coords_map);
		}
	}
	
	return true;
}

// ### GeoreferencingDockWidget ###

GeoreferencingDockWidget::GeoreferencingDockWidget(const QString title, MapEditorController* controller, QWidget* parent): QDockWidget(title, parent), controller(controller)
{
}
void GeoreferencingDockWidget::closeEvent(QCloseEvent* event)
{
	emit(closed());
	controller->setEditorActivity(NULL);
}

GeoreferencingWidget::GeoreferencingWidget(Template* temp, MapEditorController* controller, QWidget* parent): QWidget(parent), temp(temp), controller(controller)
{
	react_to_changes = true;
	
	QToolBar* toolbar = new QToolBar();
	toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	toolbar->setFloatable(false);
	
	new_act = new QAction(QIcon(":/images/cursor-georeferencing-add.png"), tr("New point"), this);
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
		header_view->setResizeMode(i, QHeaderView::ResizeToContents);
	header_view->setClickable(false);
	
	for (int i = 0; i < temp->getNumPassPoints(); ++i)
		addRow(i);
	
	apply_check = new QCheckBox(tr("Apply transformation"));
	apply_check->setChecked(temp->isGeoreferencingApplied());
	clear_and_apply_button = new QPushButton(tr("Clear points && apply"));
	clear_and_revert_button = new QPushButton(tr("Clear points && revert"));
	
	QVBoxLayout* layout = new QVBoxLayout();
	layout->addWidget(toolbar);
	layout->addWidget(table, 1);
	layout->addWidget(apply_check);
	layout->addWidget(clear_and_apply_button);
	layout->addWidget(clear_and_revert_button);
	setLayout(layout);
	
	updateActions();
	
	connect(new_act, SIGNAL(triggered(bool)), this, SLOT(newClicked(bool)));
	connect(move_act, SIGNAL(triggered(bool)), this, SLOT(moveClicked(bool)));
	connect(delete_act, SIGNAL(triggered(bool)), this, SLOT(deleteClicked(bool)));
	
	connect(apply_check, SIGNAL(clicked(bool)), this, SLOT(applyClicked(bool)));
	connect(clear_and_apply_button, SIGNAL(clicked(bool)), this, SLOT(clearAndApplyClicked(bool)));
	connect(clear_and_revert_button, SIGNAL(clicked(bool)), this, SLOT(clearAndRevertClicked(bool)));
	
	updateDirtyRect();
}
GeoreferencingWidget::~GeoreferencingWidget()
{

}

void GeoreferencingWidget::addPassPoint(MapCoordF src, MapCoordF dest)
{
	bool georef = temp->isGeoreferencingApplied();
	Template::PassPoint new_point;
	
	if (georef)
	{
		new_point.src_coords_template = temp->mapToTemplate(src);
		new_point.src_coords_map = temp->templateToMapOther(new_point.src_coords_template);
	}
	else
	{
		new_point.src_coords_map = src;
		new_point.src_coords_template = temp->mapToTemplate(new_point.src_coords_map);
	}
	new_point.dest_coords_map = dest;
	new_point.error = -1;
	
	int row = temp->getNumPassPoints();
	temp->addPassPoint(new_point, row);
	table->insertRow(row);
	addRow(row);
	
	temp->setGeoreferencingDirty(true);
	if (georef)
	{
		Template::TemplateTransform transformation;
		if (GeoreferencingActivity::calculateGeoreferencing(temp, transformation, this))
		{
			updatePointErrors();
			temp->setTransform(transformation);
			temp->setGeoreferencingDirty(false);
		}
	}
	updateDirtyRect();
	updateActions();
}
void GeoreferencingWidget::deletePassPoint(int number)
{
	assert(number >= 0 && number < temp->getNumPassPoints());
	
	temp->deletePassPoint(number);
	table->removeRow(number);
	
	temp->setGeoreferencingDirty(true);
	if (temp->isGeoreferencingApplied())
	{
		Template::TemplateTransform transformation;
		if (GeoreferencingActivity::calculateGeoreferencing(temp, transformation, this))
		{
			updatePointErrors();
			temp->setTransform(transformation);
			temp->setGeoreferencingDirty(false);
		}
	}
	updateDirtyRect(false);
	updateActions();
}
void GeoreferencingWidget::stopGeoreferencing()
{
	if (new_act->isChecked() || move_act->isChecked() || delete_act->isChecked())
		controller->setTool(NULL);
}

void GeoreferencingWidget::updateActions()
{
	bool has_georeferencing = temp->getNumPassPoints() > 0;
	
	clear_and_apply_button->setEnabled(has_georeferencing);
	clear_and_revert_button->setEnabled(has_georeferencing);
}
void GeoreferencingWidget::newClicked(bool checked)
{
	controller->setTool(checked ? (new GeoreferencingAddTool(controller, new_act, this)) : NULL);
}
void GeoreferencingWidget::moveClicked(bool checked)
{
	controller->setTool(checked ? (new GeoreferencingMoveTool(controller, move_act, this)) : NULL);
}
void GeoreferencingWidget::deleteClicked(bool checked)
{
	controller->setTool(checked ? (new GeoreferencingDeleteTool(controller, delete_act, this)) : NULL);
}

void GeoreferencingWidget::applyClicked(bool checked)
{
	if (checked)
	{
		if (temp->isGeoreferencingDirty())
		{
			Template::TemplateTransform transformation;
			if (!GeoreferencingActivity::calculateGeoreferencing(temp, transformation, this))
			{
				apply_check->setChecked(false);
				return;
			}
			updatePointErrors();
			temp->setOtherTransform(transformation);
			temp->setGeoreferencingDirty(false);
		}
	}
	
	temp->switchTransforms();
	updateDirtyRect();
}

void GeoreferencingWidget::clearAndApplyClicked(bool checked)
{
	if (!temp->isGeoreferencingApplied())
		applyClicked(true);
	
	clearPassPoints();
}
void GeoreferencingWidget::clearAndRevertClicked(bool checked)
{
	if (temp->isGeoreferencingApplied())
		applyClicked(false);
	
	clearPassPoints();
}
void GeoreferencingWidget::clearPassPoints()
{
	temp->clearPassPoints();
	apply_check->setChecked(false);
	table->clearContents();
	table->setRowCount(0);
	updateDirtyRect();
	updateActions();
}

void GeoreferencingWidget::addRow(int row)
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
void GeoreferencingWidget::updatePointErrors()
{
	react_to_changes = false;
	
	for (int row = 0; row < temp->getNumPassPoints(); ++row)
	{
		Template::PassPoint& point = *temp->getPassPoint(row);
		table->item(row, 4)->setText((point.error > 0) ? QString::number(point.error) : "?");
	}
	
	react_to_changes = true;
}
void GeoreferencingWidget::updateRow(int row)
{
	react_to_changes = false;
	
	Template::PassPoint& point = *temp->getPassPoint(row);
	table->item(row, 0)->setText(QString::number(point.src_coords_template.getX()));
	table->item(row, 1)->setText(QString::number(point.src_coords_template.getY()));
	table->item(row, 2)->setText(QString::number(point.dest_coords_map.getX()));
	table->item(row, 3)->setText(QString::number(point.dest_coords_map.getY()));
	table->item(row, 4)->setText((point.error > 0) ? QString::number(point.error) : "?");
	
	react_to_changes = true;
}

void GeoreferencingWidget::updateDirtyRect(bool redraw)
{
	bool georef = temp->isGeoreferencingApplied();
	
	if (temp->getNumPassPoints() == 0)
		temp->getMap()->clearActivityBoundingBox();
	else
	{
		QRectF rect = QRectF(georef ? temp->getPassPoint(0)->calculated_coords_map.toQPointF() : temp->getPassPoint(0)->src_coords_map.toQPointF(), QSizeF(0, 0));
		rectInclude(rect, temp->getPassPoint(0)->dest_coords_map.toQPointF());
		for (int i = 1; i < temp->getNumPassPoints(); ++i)
		{
			rectInclude(rect, georef ? temp->getPassPoint(i)->calculated_coords_map.toQPointF() : temp->getPassPoint(i)->src_coords_map.toQPointF());
			rectInclude(rect, temp->getPassPoint(i)->dest_coords_map.toQPointF());
		}
		temp->getMap()->setActivityBoundingBox(rect, GeoreferencingActivity::cross_radius, redraw);
	}
}

// ### GeoreferencingEditTool ###

GeoreferencingEditTool::GeoreferencingEditTool(MapEditorController* editor, QAction* tool_button, GeoreferencingWidget* widget): MapEditorTool(editor, Other, tool_button), widget(widget)
{
	active_point = -1;
}

void GeoreferencingEditTool::draw(QPainter* painter, MapWidget* widget)
{
	bool georef = this->widget->getTemplate()->isGeoreferencingApplied();
	
	if (active_point >= 0)
	{
		Template::PassPoint* point = this->widget->getTemplate()->getPassPoint(active_point);
		MapCoordF position = active_point_is_src ? (georef ? point->calculated_coords_map : point->src_coords_map) : point->dest_coords_map;
		QPoint viewport_pos = widget->mapToViewport(position).toPoint();
		
		painter->setPen(active_point_is_src ? Qt::red : Qt::green);
		painter->setBrush(Qt::NoBrush);
		painter->drawRect(viewport_pos.x() - GeoreferencingActivity::cross_radius, viewport_pos.y() - GeoreferencingActivity::cross_radius,
						  2*GeoreferencingActivity::cross_radius, 2*GeoreferencingActivity::cross_radius);
	}
}

void GeoreferencingEditTool::findHoverPoint(QPoint mouse_pos, MapWidget* map_widget)
{
	bool georef = this->widget->getTemplate()->isGeoreferencingApplied();
	bool new_active_point_is_src;
	int new_active_point = GeoreferencingActivity::findHoverPoint(this->widget->getTemplate(), mouse_pos, map_widget, new_active_point_is_src);
	
	if (new_active_point != active_point || (new_active_point >= 0 && new_active_point_is_src != active_point_is_src))
	{
		// Hovering over a different point than before (or none)
		active_point = new_active_point;
		active_point_is_src = new_active_point_is_src;
		
		if (active_point >= 0)
		{
			Template::PassPoint* point = this->widget->getTemplate()->getPassPoint(active_point);
			if (active_point_is_src)
			{
				if (georef)
					editor->getMap()->setDrawingBoundingBox(QRectF(point->calculated_coords_map.getX(), point->calculated_coords_map.getY(), 0, 0), GeoreferencingActivity::cross_radius);
				else
					editor->getMap()->setDrawingBoundingBox(QRectF(point->src_coords_map.getX(), point->src_coords_map.getY(), 0, 0), GeoreferencingActivity::cross_radius);
			}
			else
				editor->getMap()->setDrawingBoundingBox(QRectF(point->dest_coords_map.getX(), point->dest_coords_map.getY(), 0, 0), GeoreferencingActivity::cross_radius);
		}
		else
			editor->getMap()->clearDrawingBoundingBox();
	}
}

// ### GeoreferencingAddTool ###

QCursor* GeoreferencingAddTool::cursor = NULL;

GeoreferencingAddTool::GeoreferencingAddTool(MapEditorController* editor, QAction* tool_button, GeoreferencingWidget* widget): MapEditorTool(editor, Other, tool_button), widget(widget)
{
	first_point_set = false;
	
	if (!cursor)
		cursor = new QCursor(QPixmap(":/images/cursor-georeferencing-add.png"), 11, 11);
}
void GeoreferencingAddTool::init()
{
	// NOTE: this is called by other methods to set this text again. Change that behavior if adding stuff here
	setStatusBarText(tr("<b>Click</b> to set the source position of the pass point"));
}

bool GeoreferencingAddTool::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (event->button() != Qt::LeftButton)
		return false;
	
	if (!first_point_set)
	{
		first_point = map_coord;
		mouse_pos = map_coord;
		first_point_set = true;
		setDirtyRect(map_coord);
		
		setStatusBarText(tr("<b>Click</b> to set the destination position of the pass point, <b>Esc</b> to abort"));
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
bool GeoreferencingAddTool::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (first_point_set)
	{
		mouse_pos = map_coord;
		setDirtyRect(map_coord);
	}
	
    return true;
}
bool GeoreferencingAddTool::keyPressEvent(QKeyEvent* event)
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

void GeoreferencingAddTool::draw(QPainter* painter, MapWidget* widget)
{
	if (first_point_set)
	{
		QPointF start = widget->mapToViewport(first_point);
		QPointF end = widget->mapToViewport(mouse_pos);
		
		GeoreferencingActivity::drawCross(painter, start.toPoint(), Qt::red);
		
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

void GeoreferencingAddTool::setDirtyRect(MapCoordF mouse_pos)
{
	QRectF rect = QRectF(first_point.getX(), first_point.getY(), 0, 0);
	rectInclude(rect, mouse_pos.toQPointF());
	editor->getMap()->setDrawingBoundingBox(rect, GeoreferencingActivity::cross_radius);
}

// ### GeoreferencingMoveTool ###

QCursor* GeoreferencingMoveTool::cursor = NULL;
QCursor* GeoreferencingMoveTool::cursor_invisible = NULL;

GeoreferencingMoveTool::GeoreferencingMoveTool(MapEditorController* editor, QAction* tool_button, GeoreferencingWidget* widget): GeoreferencingEditTool(editor, tool_button, widget)
{
	dragging = false;
	
	if (!cursor)
	{
		cursor = new QCursor(QPixmap(":/images/cursor-georeferencing-move.png"), 1, 1);
		cursor_invisible = new QCursor(QPixmap(":/images/cursor-invisible.png"), 0, 0);
	}
}
void GeoreferencingMoveTool::init()
{
	setStatusBarText(tr("<b>Drag</b> to move pass points"));
}

bool GeoreferencingMoveTool::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (event->button() != Qt::LeftButton)
		return false;
	
	bool georef = this->widget->getTemplate()->isGeoreferencingApplied();
	
	active_point = GeoreferencingActivity::findHoverPoint(this->widget->getTemplate(), event->pos(), widget, active_point_is_src);
	if (active_point >= 0)
	{
		Template::PassPoint* point = this->widget->getTemplate()->getPassPoint(active_point);
		MapCoordF* point_coords;
		if (active_point_is_src)
		{
			if (georef)
				point_coords = &point->calculated_coords_map;
			else
				point_coords = &point->src_coords_map;
		}
		else
			point_coords = &point->dest_coords_map;
		
		dragging = true;
		dragging_offset = MapCoordF(point_coords->getX() - map_coord.getX(), point_coords->getY() - map_coord.getY());
		
		widget->setCursor(*cursor_invisible);
	}
	
	return false;
}
bool GeoreferencingMoveTool::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (!dragging)
		findHoverPoint(event->pos(), widget);
	else
		setActivePointPosition(map_coord);
	
	return true;
}
bool GeoreferencingMoveTool::mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	Template* temp = this->widget->getTemplate();
	
	if (dragging)
	{
		setActivePointPosition(map_coord);
		
		if (temp->isGeoreferencingApplied())
		{
			Template::TemplateTransform transformation;
			if (GeoreferencingActivity::calculateGeoreferencing(temp, transformation, this->widget))
			{
				this->widget->updatePointErrors();
				this->widget->updateDirtyRect();
				temp->setTransform(transformation);
				temp->setGeoreferencingDirty(false);
			}
		}
		
		dragging = false;
		widget->setCursor(*cursor);
	}
	return false;
}

void GeoreferencingMoveTool::setActivePointPosition(MapCoordF map_coord)
{
	bool georef = this->widget->getTemplate()->isGeoreferencingApplied();
	
	Template::PassPoint* point = this->widget->getTemplate()->getPassPoint(active_point);
	MapCoordF* changed_coords;
	if (active_point_is_src)
	{
		if (georef)
			changed_coords = &point->calculated_coords_map;
		else
			changed_coords = &point->src_coords_map;
	}
	else
		changed_coords = &point->dest_coords_map;
	QRectF changed_rect = QRectF(georef ? point->calculated_coords_map.toQPointF() : point->src_coords_map.toQPointF(), QSizeF(0, 0));
	rectInclude(changed_rect, point->dest_coords_map.toQPointF());
	rectInclude(changed_rect, map_coord.toQPointF());
	
	*changed_coords = MapCoordF(map_coord.getX() + dragging_offset.getX(), map_coord.getY() + dragging_offset.getY());
	if (active_point_is_src)
	{
		if (georef)
		{
			point->src_coords_template = this->widget->getTemplate()->mapToTemplate(point->calculated_coords_map);
			point->src_coords_map = this->widget->getTemplate()->templateToMapOther(point->src_coords_template);
		}
		else
			point->src_coords_template = this->widget->getTemplate()->mapToTemplate(point->src_coords_map);
	}
	point->error = -1;
	
	this->widget->updateRow(active_point);
	
	editor->getMap()->setDrawingBoundingBox(QRectF(changed_coords->getX(), changed_coords->getY(), 0, 0), GeoreferencingActivity::cross_radius + 1, false);
	editor->getMap()->updateDrawing(changed_rect, GeoreferencingActivity::cross_radius + 1);
	widget->updateDirtyRect();
	
	this->widget->getTemplate()->setGeoreferencingDirty(true);
}

// ### GeoreferencingDeleteTool ###

QCursor* GeoreferencingDeleteTool::cursor = NULL;

GeoreferencingDeleteTool::GeoreferencingDeleteTool(MapEditorController* editor, QAction* tool_button, GeoreferencingWidget* widget): GeoreferencingEditTool(editor, tool_button, widget)
{
	if (!cursor)
		cursor = new QCursor(QPixmap(":/images/cursor-delete.png"), 1, 1);
}
void GeoreferencingDeleteTool::init()
{
	setStatusBarText(tr("<b>Click</b> to delete pass points"));
}

bool GeoreferencingDeleteTool::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (event->button() != Qt::LeftButton)
		return false;
	
	bool georef = this->widget->getTemplate()->isGeoreferencingApplied();
	
	active_point = GeoreferencingActivity::findHoverPoint(this->widget->getTemplate(), event->pos(), widget, active_point_is_src);
	if (active_point >= 0)
	{
		Template::PassPoint* point = this->widget->getTemplate()->getPassPoint(active_point);
		QRectF changed_rect = QRectF(georef ? point->calculated_coords_map.toQPointF() : point->src_coords_map.toQPointF(), QSizeF(0, 0));
		rectInclude(changed_rect, point->dest_coords_map.toQPointF());
		
		this->widget->deletePassPoint(active_point);
		findHoverPoint(event->pos(), widget);
		editor->getMap()->updateDrawing(changed_rect, GeoreferencingActivity::cross_radius + 1);
	}
	return true;
}
bool GeoreferencingDeleteTool::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	findHoverPoint(event->pos(), widget);
	return true;
}

#include "georeferencing.moc"
