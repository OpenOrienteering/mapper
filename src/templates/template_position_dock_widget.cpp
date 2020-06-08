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


#include "template_position_dock_widget.h"

#include <QtMath>
#include <QEvent>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>

#include "core/map.h"
#include "gui/main_window.h"
#include "gui/util_gui.h"
#include "gui/map/map_editor.h"
#include "templates/template.h"


namespace OpenOrienteering {

TemplatePositionDockWidget::TemplatePositionDockWidget(Template* temp, MapEditorController* controller, QWidget* parent)
 : QDockWidget(tr("Positioning"), parent), temp(temp), controller(controller)
{
	react_to_changes = true;
	
	QLabel* x_label = new QLabel(tr("X:"));
	x_edit = new QLineEdit();
	x_edit->setValidator(new DoubleValidator(-999999, 999999, x_edit));
	QLabel* y_label = new QLabel(tr("Y:"));
	y_edit = new QLineEdit();
	y_edit->setValidator(new DoubleValidator(-999999, 999999, y_edit));
	QLabel* x_scale_label = new QLabel(tr("X-Scale:"));
	scale_x_edit = new QLineEdit();
	scale_x_edit->setValidator(new DoubleValidator(0, 999999, scale_x_edit));
	QLabel* y_scale_label = new QLabel(tr("Y-Scale:"));
	scale_y_edit = new QLineEdit();
	scale_y_edit->setValidator(new DoubleValidator(0, 999999, scale_y_edit));
	QLabel* rotation_label = new QLabel(tr("Rotation:"));
	rotation_edit = new QLineEdit();
	rotation_edit->setValidator(new DoubleValidator(-999999, 999999, rotation_edit));
	QLabel* shear_label = new QLabel(tr("Shear:"));
	shear_edit = new QLineEdit();
	shear_edit->setValidator(new DoubleValidator(-999999, 999999, shear_edit));
	
	updateValues();
	
	QWidget* child_widget = new QWidget();
	QGridLayout* layout = new QGridLayout();
	layout->addWidget(x_label, 0, 0);
	layout->addWidget(x_edit, 0, 1);
	layout->addWidget(y_label, 1, 0);
	layout->addWidget(y_edit, 1, 1);
	layout->addWidget(x_scale_label, 2, 0);
	layout->addWidget(scale_x_edit, 2, 1);
	layout->addWidget(y_scale_label, 3, 0);
	layout->addWidget(scale_y_edit, 3, 1);
	layout->addWidget(rotation_label, 4, 0);
	layout->addWidget(rotation_edit, 4, 1);
	layout->addWidget(shear_label, 5, 0);
	layout->addWidget(shear_edit, 5, 1);
	child_widget->setLayout(layout);
	setWidget(child_widget);
	
	connect(x_edit, &QLineEdit::textEdited, this, &TemplatePositionDockWidget::valueChanged);
	connect(y_edit, &QLineEdit::textEdited, this, &TemplatePositionDockWidget::valueChanged);
	connect(scale_x_edit, &QLineEdit::textEdited, this, &TemplatePositionDockWidget::valueChanged);
	connect(scale_y_edit, &QLineEdit::textEdited, this, &TemplatePositionDockWidget::valueChanged);
	connect(rotation_edit, &QLineEdit::textEdited, this, &TemplatePositionDockWidget::valueChanged);
	connect(shear_edit, &QLineEdit::textEdited, this, &TemplatePositionDockWidget::valueChanged);
	
	connect(controller->getMap(), &Map::templateChanged, this, &TemplatePositionDockWidget::templateChanged);
	connect(controller->getMap(), &Map::templateDeleted, this, &TemplatePositionDockWidget::templateDeleted);
}

void TemplatePositionDockWidget::updateValues()
{
	if (!react_to_changes) return;
	x_edit->setText(QString::number(0.001 * temp->getTemplateX()));
	y_edit->setText(QString::number(-0.001 * temp->getTemplateY()));
	scale_x_edit->setText(QString::number(temp->getTemplateScaleX()));
	scale_y_edit->setText(QString::number(temp->getTemplateScaleY()));
	rotation_edit->setText(QString::number(temp->getTemplateRotation() * 180 / M_PI));
	shear_edit->setText(QString::number(temp->getTemplateShear()));
}

bool TemplatePositionDockWidget::event(QEvent* event)
{
	if (event->type() == QEvent::ShortcutOverride && controller->getWindow()->shortcutsBlocked())
		event->accept();
	return QDockWidget::event(event);
}

void TemplatePositionDockWidget::closeEvent(QCloseEvent* event)
{
	Q_UNUSED(event);
	controller->removeTemplatePositionDockWidget(temp);
}

void TemplatePositionDockWidget::templateChanged(int index, const Template* temp)
{
	Q_UNUSED(index);
	
	if (this->temp != temp)
		return;
	
	updateValues();
}

void TemplatePositionDockWidget::templateDeleted(int index, const Template* temp)
{
	Q_UNUSED(index);
	
	if (this->temp != temp)
		return;
	
	close();
}

void TemplatePositionDockWidget::valueChanged()
{
	// Mark original template area as dirty
	temp->setTemplateAreaDirty();
	
	// Transform relevant parts of pass points into template coords
	for (int i = 0; i < temp->getNumPassPoints(); ++i)
	{
		PassPoint* point = temp->getPassPoint(i);
		
		if (temp->isAdjustmentApplied())
		{
			point->dest_coords = temp->mapToTemplate(point->dest_coords);
			point->calculated_coords = temp->mapToTemplate(point->calculated_coords);
		}
		else
			point->src_coords = temp->mapToTemplate(point->src_coords);
	}
	
	// Change transformation
	QWidget* widget = (QWidget*)sender();
	if (widget == x_edit)
		temp->setTemplateX(qRound64(1000 * x_edit->text().toDouble()));
	else if (widget == y_edit)
		temp->setTemplateY(qRound64(-1000 * y_edit->text().toDouble()));
	else if (widget == scale_x_edit)
		temp->setTemplateScaleX(scale_x_edit->text().toDouble());
	else if (widget == scale_y_edit)
		temp->setTemplateScaleY(scale_y_edit->text().toDouble());
	else if (widget == rotation_edit)
		temp->setTemplateRotation(rotation_edit->text().toDouble() * M_PI / 180.0);
	else if (widget == shear_edit)
		temp->setTemplateShear(shear_edit->text().toDouble());
	
	// Transform relevant parts of pass points back to map coords
	for (int i = 0; i < temp->getNumPassPoints(); ++i)
	{
		PassPoint* point = temp->getPassPoint(i);
		
		if (temp->isAdjustmentApplied())
		{
			point->dest_coords = temp->templateToMap(point->dest_coords);
			point->calculated_coords = temp->templateToMap(point->calculated_coords);
		}
		else
			point->src_coords = temp->templateToMap(point->src_coords);
	}
	
	// Mark new template area as dirty
	temp->setTemplateAreaDirty();
	
	// Tell others about changes
	react_to_changes = false;
	controller->getMap()->emitTemplateChanged(temp);
	react_to_changes = true;
}


}  // namespace OpenOrienteering
