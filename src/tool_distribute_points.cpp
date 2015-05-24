/*
 *    Copyright 2013 Thomas Schöps
 *    Copyright 2014, 2015 Kai Pastor
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


#include "tool_distribute_points.h"

#include <qmath.h>
#include <QDialogButtonBox>
#include <QFormLayout>

#include "map.h"
#include "symbol_point.h"
#include "object.h"
#include "object_undo.h"
#include "util.h"
#include "util_gui.h"

bool DistributePointsTool::showSettingsDialog(QWidget* parent, PointSymbol* point, DistributePointsTool::Settings& settings)
{
	DistributePointsSettingsDialog dialog(parent, point, settings);
	dialog.setWindowModality(Qt::WindowModal);
	if (dialog.exec() == QDialog::Rejected)
		return false;
	
	dialog.getValues(settings);
	return true;
}

void DistributePointsTool::execute(PathObject* path, PointSymbol* point, const DistributePointsTool::Settings& settings, std::vector<PointObject*>* out_objects)
{
	path->update();
	
	// This places the points only on the first part.
	PathPart& part = path->parts().front();
	
	// Check how to distribute the points over the part length
	int total, start, end;
	if (part.isClosed())
	{
		total = settings.num_points_per_line;
		start = 0;
		end = total - 1;
	}
	else if (settings.points_at_ends)
	{
		total = settings.num_points_per_line - 1;
		start = 0;
		end = total;
	}
	else
	{
		total = settings.num_points_per_line + 1;
		start = 1;
		end = total - 1;
	}
	
	auto distance = part.length() / total;
	auto coords = path->getRawCoordinateVector();
	auto split = SplitPathCoord::begin(part.path_coords);
	
	// Create the objects
	for (int i = start; i <= end; ++i)
	{
		auto clen = distance * i;
		split = SplitPathCoord::at(part.path_coords, clen);
		
		PointObject* object = new PointObject(point);
		object->setPosition(split.pos);
		if (point->isRotatable())
		{
			double rotation = settings.additional_rotation;
			if (settings.rotate_symbols)
			{
				auto right = split.tangentVector().perpRight();
				rotation -= right.angle();
			}
			object->setRotation(rotation);
		}
		out_objects->push_back(object);
	}
}


DistributePointsSettingsDialog::DistributePointsSettingsDialog(QWidget* parent, PointSymbol* point, DistributePointsTool::Settings& settings)
 : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
{
	setWindowTitle(tr("Distribute points evenly along path"));
	
	QFormLayout* layout = new QFormLayout();
	
	num_points_edit = Util::SpinBox::create(settings.points_at_ends ? 2 : 1, 9999);
	num_points_edit->setValue(settings.num_points_per_line);
	layout->addRow(tr("Number of points per path:"), num_points_edit);
	
	points_at_ends_check = new QCheckBox(tr("Also place objects at line end points"));
	points_at_ends_check->setChecked(settings.points_at_ends);
	layout->addRow(points_at_ends_check);
	
	
	layout->addItem(Util::SpacerItem::create(this));
	layout->addRow(Util::Headline::create(tr("Rotation settings")));
	
	rotate_symbols_check = new QCheckBox(tr("Align points with direction of line"));
	rotate_symbols_check->setChecked(settings.rotate_symbols);
	layout->addRow(rotate_symbols_check);
	
	additional_rotation_edit = Util::SpinBox::create(1, 0, 360, trUtf8("°", "degrees"));
	additional_rotation_edit->setValue(settings.additional_rotation * 180 / M_PI);
	layout->addRow(tr("Additional rotation angle (counter-clockwise):"), additional_rotation_edit);
	
	if (!point->isRotatable())
	{
		rotate_symbols_check->setEnabled(false);
		additional_rotation_edit->setEnabled(false);
	}
	
	
	layout->addItem(Util::SpacerItem::create(this));
	QDialogButtonBox* button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal);
	layout->addRow(button_box);
	
	setLayout(layout);
	
	connect(points_at_ends_check, SIGNAL(clicked(bool)), this, SLOT(updateWidgets()));
	connect(button_box, SIGNAL(accepted()), this, SLOT(accept()));
	connect(button_box, SIGNAL(rejected()), this, SLOT(reject()));
}

void DistributePointsSettingsDialog::getValues(DistributePointsTool::Settings& settings)
{
	settings.num_points_per_line = num_points_edit->value();
	settings.points_at_ends = points_at_ends_check->isChecked();
	settings.rotate_symbols = rotate_symbols_check->isChecked();
	settings.additional_rotation = additional_rotation_edit->value() * M_PI / 180.0;
}

void DistributePointsSettingsDialog::updateWidgets()
{
	num_points_edit->setMinimum(points_at_ends_check->isChecked() ? 2 : 1);
}
