/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2015-2017 Kai Pastor
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

#ifndef OPENORIENTEERING_EDIT_TOOL_H
#define OPENORIENTEERING_EDIT_TOOL_H

#include <utility>
#include <vector>

#include <Qt>
#include <QFlags>
#include <QObject>
#include <QPointF>
#include <QRectF>
#include <QRgb>
#include <QScopedPointer>

#include "core/map_coord.h"
#include "tools/tool.h"
#include "tools/tool_base.h"

class QAction;
class QPainter;
class QPointF;
class QRectF;

namespace OpenOrienteering {

class MapEditorController;
class MapWidget;
class Object;
class ObjectSelector;

using SelectionInfoVector = std::vector<std::pair<int, Object*>>;


/**
 * Base class for object editing tools.
 */
class EditTool : public MapEditorToolBase
{
public:
	/**
	 * @brief A type for general information on what is hovered over.
	 */
	enum HoverFlag
	{
		OverNothing      = 0,
		OverFrame        = 1,
		OverObjectNode   = 2,
		OverPathEdge     = 4,
	};
	Q_DECLARE_FLAGS(HoverState, HoverFlag)
	
Q_OBJECT
public:
	EditTool(MapEditorController* editor, MapEditorTool::Type type, QAction* tool_action);
	
	~EditTool() override;
	
	/**
	 * The platform's key for deleting selected objects.
	 * 
	 * OS X use the backspace key for deleting selected objects,
	 * while other platforms use the delete key.
	 * 
	 * This causes translation issues and inconsistent behaviour on OS X:
	 * - In Finder, moving an object to trash is Cmd+Backspace.
	 * - Other programs are reported to use [forward] delete.
	 * - Some programs are reported to support multiple keys,
	 *   e.g. Delete and Backspace.
	 * - A major source of irritation is the absence of a delete key on some
	 *   Macbooks. On these keyboards, delete is entered as Fn+Backspace.
	 * - Some programs use another key for delete, e.g. "x".
	 *   (Note that Cmd-x (aka Cut) will have a similar effect.)
	 * 
	 * \todo Either use a function for testing whether a key means
	 *       "delete object", or switch to a QAction based implementation
	 *       since QAction supports alternative QKeySequences.
	 */
#ifdef Q_OS_MACOS
	static constexpr Qt::Key DeleteObjectKey = Qt::Key_Backspace;
#else
	static constexpr Qt::Key DeleteObjectKey = Qt::Key_Delete;
#endif
	
protected:
	/**
	 * Deletes all selected objects and updates the status text.
	 */
	void deleteSelectedObjects();
	
	/**
	 * Creates a replace object undo step for the given object.
	 */
	void createReplaceUndoStep(Object* object);
	
	/**
	 * Returns if the point is inside the click_tolerance from the rect's border.
	 */
	bool pointOverRectangle(const QPointF& point, const QRectF& rect) const;
	
	/**
	 * Returns the point on the rect which is closest to the given point.
	 */
	static MapCoordF closestPointOnRect(MapCoordF point, const QRectF& rect);
	
	/**
	 * Configures the angle helper from the primary directions of the edited objects.
	 * 
	 * If no primary directions are found, the default directions are set.
	 */
	void setupAngleHelperFromEditedObjects();
	
	/**
	 * Draws a bounding box with a dashed line of the given color.
	 * 
	 * @param bounding_box the box extent in map coordinates
	 */
	void drawBoundingBox(QPainter* painter, MapWidget* widget, const QRectF& bounding_box, const QRgb& color);
	
	/**
	 * Draws a bounding path with a dashed line of the given color.
	 * 
	 * @param bounding_box the box extent in map coordinates
	 */
	void drawBoundingPath(QPainter* painter, MapWidget* widget, const std::vector<QPointF>& bounding_path, const QRgb& color);
	
	/**
	 * An utility implementing object selection logic.
	 */
	QScopedPointer<ObjectSelector> object_selector;
};


}  // namespace OpenOrienteering


Q_DECLARE_OPERATORS_FOR_FLAGS(OpenOrienteering::EditTool::HoverState)


#endif
