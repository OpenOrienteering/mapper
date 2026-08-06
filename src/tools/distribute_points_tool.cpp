/*
 *    Copyright 2013 Thomas Schöps
 *    Copyright 2014-2015, 2017-2019 Kai Pastor
 *    Copyright 2024 Matthias Kühlewein
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

#include <vector>

#include <Qt>
#include <QtMath>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QSpacerItem>
#include <QSpinBox>
#include <QWidget>

#include "core/map.h"
#include "core/map_coord.h"
#include "core/path_coord.h"
#include "core/symbols/point_symbol.h"
#include "core/objects/object.h"
#include "gui/util_gui.h"
#include "undo/object_undo.h"


namespace OpenOrienteering {

DistributePointsDialog::DistributePointsDialog(QWidget* parent, Map* map, const PointSymbol* point)
: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
, map { map }
, point { point }
{
	setWindowTitle(tr("Distribute points evenly along path"));
	
	auto layout = new QFormLayout();
	
	num_points_edit = Util::SpinBox::create(1, 9999);
	num_points_edit->setValue(3);
	layout->addRow(tr("Number of points per path:"), num_points_edit);
	
	points_at_ends_check = new QCheckBox(tr("Also place objects at line end points"));
	points_at_ends_check->setChecked(true);
	layout->addRow(points_at_ends_check);
	
	layout->addItem(Util::SpacerItem::create(this));
	
	auto rotation_headline = Util::Headline::create(tr("Rotation settings"));
	layout->addRow(rotation_headline);
	
	rotate_symbols_check = new QCheckBox(tr("Align points with direction of line"));
	rotate_symbols_check->setChecked(true);
	layout->addRow(rotate_symbols_check);
	
	additional_rotation_edit = Util::SpinBox::create<Util::RotationalDegrees>();
	additional_rotation_edit->setDecimals(1);
	additional_rotation_edit->setSingleStep(5.0);
	additional_rotation_edit->setValue(qRadiansToDegrees(0.0));
	layout->addRow(tr("Additional rotation angle (counter-clockwise):"), additional_rotation_edit);
	
	if (!point->isRotatable())
	{
		rotation_headline->setEnabled(false);
		rotate_symbols_check->setEnabled(false);
		additional_rotation_edit->setEnabled(false);
		layout->labelForField(additional_rotation_edit)->setEnabled(false);
	}
	
	layout->addItem(Util::SpacerItem::create(this));
	auto button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	layout->addRow(button_box);
	
	setLayout(layout);
	
	connect(button_box, &QDialogButtonBox::accepted, this, &DistributePointsDialog::okClicked);
	connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

DistributePointsDialog::~DistributePointsDialog() = default;

// slot
void DistributePointsDialog::okClicked()
{
	// Create points along paths
	created_objects.reserve(map->selectedObjects().size() * num_points_edit->value());
	for (const auto* object : map->selectedObjects())
	{
		if (object->getType() == Object::Path)
			distributePoints(object->asPath());
	}
	if (created_objects.empty())
		return;
	
	// Add points to map
	for (auto* o : created_objects)
		map->addObject(o);
	
	// Create undo step and select new objects
	map->clearObjectSelection(false);
	MapPart* part = map->getCurrentPart();
	auto* delete_step = new DeleteObjectsUndoStep(map);
	for (std::size_t i = 0; i < created_objects.size(); ++i)
	{
		Object* object = created_objects[i];
		delete_step->addObject(part->findObjectIndex(object));
		map->addObjectToSelection(object, i == created_objects.size() - 1);
	}
	map->push(delete_step);
	map->setObjectsDirty();
	
	accept();
}

void DistributePointsDialog::distributePoints(const PathObject* path)
{
	const auto num_points_per_line = num_points_edit->value();
	const auto points_at_ends = points_at_ends_check->isChecked();
	const auto rotate_symbols = rotate_symbols_check->isChecked();
	const auto additional_rotation = qDegreesToRadians(additional_rotation_edit->value());
	
	path->update();
	
	// This places the points only on the first part.
	const auto& part = path->parts().front();
	
	// Check how to distribute the points over the part length
	int total, start, end;
	if (part.isClosed())
	{
		total = num_points_per_line;
		start = 0;
		end = total - 1;
	}
	else if (!points_at_ends)
	{
		total = num_points_per_line + 1;
		start = 1;
		end = total - 1;
	}
	else if (num_points_per_line == 1)
	{
		total = 1;
		start = 1;
		end = 1;
	}
	else
	{
		total = num_points_per_line - 1;
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
			double rotation = additional_rotation;
			if (rotate_symbols)
			{
				auto right = split.tangentVector().perpRight();
				rotation -= right.angle();
			}
			object->setRotation(rotation);
		}
		created_objects.push_back(object);
	}
}


}  // namespace OpenOrienteering
