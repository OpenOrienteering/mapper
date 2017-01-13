/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2014 Kai Pastor
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
 * Tool to pan the map.
 */
class PanTool : public MapEditorToolBase
{
Q_OBJECT
public:
	PanTool(MapEditorController* editor, QAction* tool_action);
	virtual ~PanTool();
	
protected:
	virtual void updateStatusText() override;
	virtual void objectSelectionChangedImpl() override;
	virtual void clickPress() override;
	virtual void dragStart() override;
	virtual void dragMove() override;
	virtual void dragFinish() override;
	virtual void dragCanceled() override;
};

#endif
