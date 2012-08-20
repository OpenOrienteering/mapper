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


#ifndef _OPENORIENTEERING_TOOL_ROTATE_PATTERN_H_
#define _OPENORIENTEERING_TOOL_ROTATE_PATTERN_H_

#include "tool.h"

#include <vector>

#include <QScopedPointer>

class Renderable;
class MapRenderables;
typedef std::vector<Renderable*> RenderableVector;

/// Tool to rotate patterns of area objects or rotatable point objects
class RotatePatternTool : public MapEditorToolBase
{
Q_OBJECT
public:
	RotatePatternTool(MapEditorController* editor, QAction* tool_button);
	
    virtual void draw(QPainter* painter, MapWidget* widget);
	
protected:
    virtual int updateDirtyRectImpl(QRectF& rect);
	virtual void updateStatusText();
    virtual void objectSelectionChangedImpl();
	
    virtual void dragStart();
    virtual void dragMove();
    virtual void dragFinish();
};

#endif
