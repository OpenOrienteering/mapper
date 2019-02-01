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
#include <QInputDialog>
#include <QKeyEvent>
#include <QLocale>
#include <QPainter>
#include <QPixmap>
#include <QPoint>
#include <QPointF>
#include <QRectF>
#include <QString>

#include "core/map.h"
#include "core/map_view.h"
#include "gui/map/map_widget.h"
#include "core/objects/object.h"
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
		setStatusBarText(tr("<b>Scaling:</b> %1%").arg(QLocale().toString(scaling_factor * 100, 'f', 1)));
	else
		setStatusBarText(tr("<b>Click</b>: Set the scaling center. ") +
		                 tr("<b>Drag</b>: Scale the selected objects. "));
}



void ScaleTool::clickRelease()
{
	prev_scaling_center = scaling_center;
	scaling_center = cur_pos_map;
	updateDirtyRect();
	updateStatusText();
}



void ScaleTool::dragStart()
{
	// WARNING: reference_length may become 0.
	reference_length = (click_pos_map - scaling_center).length();
	startEditing(map()->selectedObjects());
}


namespace {
void doScaling(ScaleTool::ObjectsRange objects, double scaling_factor, const MapCoordF *group_center)
{
	for (auto object : objects)
	{
		const auto object_scaling_center = group_center ? *group_center : MapCoordF(object->getExtent().center());
		object->scale(object_scaling_center, scaling_factor);
	}
}
}  // anonymous namespace


void ScaleTool::dragMove()
{
	resetEditedObjects();
	
	// minimum_length will replace any shorter length, 
	// in order to avoid extreme values and division by zero.
	auto minimum_length = 1.0 / cur_map_widget->getMapView()->getZoom();
	
	auto scaling_length = (cur_pos_map - scaling_center).length();
	scaling_factor = qMax(minimum_length, scaling_length) / qMax(minimum_length, reference_length);
	doScaling(editedObjects(), scaling_factor, group_scaling ? &scaling_center : nullptr);
	
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
		group_scaling = false;
		updateStatusText();
		updateDirtyRect();
		return false; // not consuming Ctrl
	}

	return false;
}


bool ScaleTool::keyReleaseEvent(QKeyEvent* event)
{
	switch (event->key())
	{
	case Qt::Key_Control:
		group_scaling = true;
		updateStatusText();
		updateDirtyRect();
		return false; // not consuming Ctrl
	}

	return false;
}

bool ScaleTool::mouseDoubleClickEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget)
{
	Q_UNUSED(event)
	Q_UNUSED(map_coord)

	// restore scaling center originally moved by the doubleclick's first click
	scaling_center = prev_scaling_center;

	bool ok;
	auto scale_to_pct = QInputDialog::getDouble(widget, tr("Scale object(s)"),
	                                        tr("Percent:"), 100, 0, 1e4, 1, &ok,
	                                        Qt::WindowFlags(), 5);
	if (ok)
	{
		startEditing(map()->selectedObjects());
		doScaling(editedObjects(), scale_to_pct/100, group_scaling ? &scaling_center : nullptr);
		finishEditing();
	}

	return true;
}


void ScaleTool::drawImpl(QPainter* painter, MapWidget* widget)
{
	drawSelectionOrPreviewObjects(painter, widget);
	
	// /todo - this is the same circle as in rotate tool
	// move the function to Util and use the shared code
	auto draw_center_sign = [&](MapCoordF circle_center){
		painter->setPen(Qt::white);
		painter->setBrush(Qt::NoBrush);

		QPoint center = widget->mapToViewport(circle_center).toPoint();
		painter->drawEllipse(center, 3, 3);
		painter->setPen(Qt::black);
		painter->drawEllipse(center, 4, 4);
	};

	if (group_scaling)
	{
		draw_center_sign(scaling_center);
	}
	else
	{
		for (const auto object : map()->selectedObjects())
			draw_center_sign(MapCoordF(object->getExtent().center()));
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
