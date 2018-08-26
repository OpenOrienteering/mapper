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


#include "distribute_points_tool.h"

#include <Qt>
#include <QtMath>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFlags>
#include <QFormLayout>
#include <QLabel>
#include <QSpacerItem>
#include <QSpinBox>
#include <QWidget>

#include "core/map_coord.h"
#include "core/path_coord.h"
#include "core/symbols/point_symbol.h"
#include "core/objects/object.h"
#include "gui/util_gui.h"


namespace OpenOrienteering {

bool DistributePointsTool::showSettingsDialog(
        QWidget* parent,
        const PointSymbol* point,
        DistributePointsTool::Settings& settings )
{
	DistributePointsSettingsDialog dialog(parent, point, settings);
	dialog.setWindowModality(Qt::WindowModal);
	if (dialog.exec() == QDialog::Rejected)
		return false;
	
	dialog.getValues(settings);
	return true;
}

void DistributePointsTool::execute(
        const PathObject* path,
        PointSymbol* point,
        const DistributePointsTool::Settings& settings,
        std::vector<PointObject*>& out_objects )
{
	path->update();
	
	// This places the points only on the first part.
	const auto& part = path->parts().front();
	
	// Check how to distribute the points over the part length
	int total, start, end;
	if (part.isClosed())
	{
		total = settings.num_points_per_line;
		start = 0;
		end = total - 1;
	}
	else if (!settings.points_at_ends)
	{
		total = settings.num_points_per_line + 1;
		start = 1;
		end = total - 1;
	}
	else if (settings.num_points_per_line == 1)
	{
		total = 1;
		start = 1;
		end = 1;
	}
	else
	{
		total = settings.num_points_per_line - 1;
		start = 0;
		end = total;
	}
	
	auto distance = part.length() / total;
	auto split = SplitPathCoord::begin(part.path_coords);
	
	// Create the objects
	for (int i = start; i <= end; ++i)
	{
		auto clen = distance * i;
		split = SplitPathCoord::at(clen, split);
		
		auto object = new PointObject(point);
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
		out_objects.push_back(object);
	}
}


DistributePointsSettingsDialog::DistributePointsSettingsDialog(
        QWidget* parent,
        const PointSymbol* point,
        const DistributePointsTool::Settings& settings )
 : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
{
	setWindowTitle(tr("Distribute points evenly along path"));
	
	auto layout = new QFormLayout();
	
	num_points_edit = Util::SpinBox::create(1, 9999);
	num_points_edit->setValue(settings.num_points_per_line);
	layout->addRow(tr("Number of points per path:"), num_points_edit);
	
	points_at_ends_check = new QCheckBox(tr("Also place objects at line end points"));
	points_at_ends_check->setChecked(settings.points_at_ends);
	layout->addRow(points_at_ends_check);
	
	layout->addItem(Util::SpacerItem::create(this));
	
	auto rotation_headline = Util::Headline::create(tr("Rotation settings"));
	layout->addRow(rotation_headline);
	
	rotate_symbols_check = new QCheckBox(tr("Align points with direction of line"));
	rotate_symbols_check->setChecked(settings.rotate_symbols);
	layout->addRow(rotate_symbols_check);
	
	additional_rotation_edit = Util::SpinBox::create(1, 0, 360, trUtf8("°", "degrees"), 5);
	additional_rotation_edit->setValue(qRadiansToDegrees(settings.additional_rotation));
	layout->addRow(tr("Additional rotation angle (counter-clockwise):"), additional_rotation_edit);
	
	if (!point->isRotatable())
	{
		rotation_headline->setEnabled(false);
		rotate_symbols_check->setEnabled(false);
		additional_rotation_edit->setEnabled(false);
		layout->labelForField(additional_rotation_edit)->setEnabled(false);
	}
	
	layout->addItem(Util::SpacerItem::create(this));
	auto button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal);
	layout->addRow(button_box);
	
	setLayout(layout);
	
	connect(button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void DistributePointsSettingsDialog::getValues(DistributePointsTool::Settings& settings)
{
	settings.num_points_per_line = num_points_edit->value();
	settings.points_at_ends = points_at_ends_check->isChecked();
	settings.rotate_symbols = rotate_symbols_check->isChecked();
	settings.additional_rotation = qDegreesToRadians(additional_rotation_edit->value());
}


}  // namespace OpenOrienteering
