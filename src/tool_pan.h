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


#ifndef _OPENORIENTEERING_TOOL_PAN_H_
#define _OPENORIENTEERING_TOOL_PAN_H_

#include "tool_base.h"

/** 
 * A tool to pan the map.
 */
class PanTool : public MapEditorToolBase
{
Q_OBJECT
public:
	PanTool(MapEditorController* editor, QAction* tool_button);
	virtual ~PanTool();
	
protected:
	virtual void updateStatusText();
	virtual void objectSelectionChangedImpl();
	
	virtual void dragStart();
	virtual void dragMove();
	virtual void dragFinish();
};

#endif
