/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2013-2017 Kai Pastor
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


#include "scale_tool.h"

#include <Qt>
#include <QtGlobal>
#include <QCursor>
#include <QKeyEvent>
#include <QLocale>
#include <QPixmap>
#include <QRectF>
#include <QString>

#include "core/map.h"
#include "core/map_view.h"
#include "gui/map/map_widget.h"
#include "gui/util_gui.h"
#include "core/objects/object.h"
#include "gui/modifier_key.h"
#include "tools/tool.h"
#include "util/util.h"


namespace OpenOrienteering {

ScaleTool::ScaleTool(MapEditorController* editor, QAction* tool_action)
: MapEditorToolBase { scaledToScreen(QCursor{ QPixmap(QString::fromLatin1(":/images/cursor-scale.png")), 1, 1 }), Other, editor, tool_action }
{
	// nothing else
}


ScaleTool::~ScaleTool() = default;



void ScaleTool::initImpl()
{
	objectSelectionChangedImpl();
}



// This function contains translations. Keep it close to the top of the file so
// that line numbers remain stable here when changing other parts of the file.
void ScaleTool::updateStatusText()
{
	if (editingInProgress())
	{
		if (uniform_scaling)
			setStatusBarText(tr("<b>Scaling:</b> %1%").arg(QLocale().toString(qSqrt(scaling_factor.x()*scaling_factor.x() + scaling_factor.y()*scaling_factor.y()) * 100 / qSqrt(2), 'f', 1)));
		else
			setStatusBarText(tr("<b>Scaling:</b> x: %1% y: %2%").arg(QLocale().toString(scaling_factor.x() * 100, 'f', 1), QLocale().toString(scaling_factor.y() * 100, 'f', 1)));
	}
	else
	{
		auto status_text = tr("<b>Drag</b>: Scale the selected objects. ");
		if (using_scaling_center)
			status_text = tr("<b>Click</b>: Set the scaling center. ") +
			              status_text +
			              tr("<b>%1</b>: Switch to individual object scaling. ").arg(ModifierKey::control());
		if (uniform_scaling)
			status_text = status_text + tr("<b>%1</b>: Enable non-uniform scaling. ").arg(ModifierKey::shift());
		setStatusBarText(status_text);
	}
}



void ScaleTool::clickRelease()
{
	scaling_center = cur_pos_map;
	updateDirtyRect();
	updateStatusText();
}



void ScaleTool::dragStart()
{
	scaling_start_point = click_pos_map;
	startEditing(map()->selectedObjects());
}

double absmax(double v, double max)
{
	// a bipolar qMax() equivalent that returns v when v is larger than +max,
	// or smaller than -max, otherwise returns +max or -max depending on sign of v
	if (v >= 0)
		return qMax(v, max);
	else
		return qMin(v, -max);
}

void ScaleTool::dragMove()
{
	resetEditedObjects();
	
	// minimum_length will replace any shorter length, 
	// in order to avoid extreme values and division by zero.
	auto minimum_length = 1.0 / cur_map_widget->getMapView()->getZoom();
	
	auto scaling_point = cur_pos_map - scaling_center;
	auto reference_point = scaling_start_point - scaling_center;

	if (uniform_scaling)
	{
		// scaling_point and reference_point are converted to contain the respective vector length in both x and y components,
		// instead of having independent x and y components.
		scaling_point.setX(qSqrt(scaling_point.x()*scaling_point.x() + scaling_point.y()*scaling_point.y()));
		scaling_point.setY(scaling_point.x());
		reference_point.setX(qSqrt(reference_point.x()*reference_point.x() + reference_point.y()*reference_point.y()));
		reference_point.setY(reference_point.x());
	}

	scaling_factor.setX(absmax(scaling_point.x(), minimum_length) / absmax(reference_point.x(), minimum_length));
	scaling_factor.setY(absmax(scaling_point.y(), minimum_length) / absmax(reference_point.y(), minimum_length));

	if (using_scaling_center)
	{
		for (auto* object : editedObjects())
			object->scale(scaling_center, scaling_factor.x(), scaling_factor.y());
	}
	else
	{
		for (auto* object : editedObjects())
			object->scale(MapCoordF(object->getExtent().center()), scaling_factor.x(), scaling_factor.y());
	}
	
	updatePreviewObjects();
	updateStatusText();
}


void ScaleTool::dragFinish()
{
	finishEditing();
	updateStatusText();
}


bool ScaleTool::keyPressEvent(QKeyEvent* event)
{
	switch (event->key())
	{
	case Qt::Key_Control:
		using_scaling_center = false;
		updateStatusText();
		updateDirtyRect();
		return false; // not consuming Ctrl
	case Qt::Key_Shift:
		uniform_scaling = false;
		updateStatusText();
		updateDirtyRect();
		return false;
	}

	return false;
}


bool ScaleTool::keyReleaseEvent(QKeyEvent* event)
{
	switch (event->key())
	{
	case Qt::Key_Control:
		using_scaling_center = true;
		updateStatusText();
		updateDirtyRect();
		return false; // not consuming Ctrl
	case Qt::Key_Shift:
		uniform_scaling = true;
		updateStatusText();
		updateDirtyRect();
		return false;
	}

	return false;
}


void ScaleTool::drawImpl(QPainter* painter, MapWidget* widget)
{
	drawSelectionOrPreviewObjects(painter, widget);
	
	if (using_scaling_center)
	{
		Util::Marker::drawCenterMarker(painter, widget->mapToViewport(scaling_center));
	}
	else
	{
		for (const auto* object : map()->selectedObjects())
			Util::Marker::drawCenterMarker(painter, widget->mapToViewport(object->getExtent().center()));
	}
}


int ScaleTool::updateDirtyRectImpl(QRectF& rect)
{
	rectIncludeSafe(rect, scaling_center);
	return 5;
}



void ScaleTool::objectSelectionChangedImpl()
{
	if (editingInProgress())
	{
		abortEditing();
		cancelDragging();
	}
	
	if (map()->getNumSelectedObjects() == 0)
	{
		deactivate();
	}
	else
	{
		// Set initial scaling center to the center of the bounding box of the selected objects
		QRectF rect;
		map()->includeSelectionRect(rect);
		scaling_center = rect.center();
		
		updateDirtyRect();
	}
}


}  // namespace OpenOrienteering
