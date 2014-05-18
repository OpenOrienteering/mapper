/*
 *    Copyright 2013 Thomas Schöps
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


#ifndef _OPENORIENTEERING_TOOL_CUTOUT_H_
#define _OPENORIENTEERING_TOOL_CUTOUT_H_

#include "tool.h"
#include "tool_base.h"

class PathObject;
class Symbol;
class ObjectSelector;

/** Tool to make map cutouts */
class CutoutTool : public MapEditorToolBase
{
Q_OBJECT
public:
	/**
	 * Constructs a CutoutTool.
	 * 
	 * Setting cut_away to true inverts the tool's effect, cutting the
	 * part inside the cut shape away instead of the part outside.
	 */
	CutoutTool(MapEditorController* editor, QAction* tool_action, bool cut_away);
	virtual ~CutoutTool();

	virtual void drawImpl(QPainter* painter, MapWidget* widget);

	virtual bool keyPress(QKeyEvent* event);
	
	/**
	 * Applies the tool to cut all selected objects in map, or to all objects
	 * if there is no selection. cutout_object itself must not be selected!
	 * 
	 * Can also be used without constructing a tool object.
	 * 
	 * Setting cut_away to true inverts the effect, cutting the
	 * part inside the cut shape away instead of the part outside.
	 */
	static void apply(Map* map, PathObject* cutout_object, bool cut_away);
	
protected:
	virtual void initImpl();
	virtual int updateDirtyRectImpl(QRectF& rect);
	virtual void updateStatusText();
	virtual void objectSelectionChangedImpl();
	
    virtual void clickRelease();
    virtual void dragStart();
    virtual void dragMove();
    virtual void dragFinish();
	
	/**
	 * If false, the tool removes all objects outside the cutout area,
	 * otherwise it removes all objects inside the cutout area
	 */
	bool cut_away;
	
	/** The object which determines the cutout's shape */
	PathObject* cutout_object;
	
	/**
	 * The index of the cutout object in its map part, so it can be inserted
	 * there again after taking it out. Set to a negative value to indicate
	 * that the object has already been re-added.
	 */
	int cutout_object_index;
	
	/** Object selection helper */
	QScopedPointer<ObjectSelector> object_selector;
};

#endif
