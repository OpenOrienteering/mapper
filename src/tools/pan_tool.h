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


#ifndef OPENORIENTEERING_TOOL_PAN_H
#define OPENORIENTEERING_TOOL_PAN_H

#include <QObject>
#include <QString>

#include "tools/tool_base.h"

class QAction;

namespace OpenOrienteering {

class MapEditorController;


/** 
 * Tool to pan the map.
 */
class PanTool : public MapEditorToolBase
{
Q_OBJECT
public:
	PanTool(MapEditorController* editor, QAction* tool_action);
	~PanTool() override;
	
protected:
	void updateStatusText() override;
	void objectSelectionChangedImpl() override;
	void clickPress() override;
	void dragStart() override;
	void dragMove() override;
	void dragFinish() override;
	void dragCanceled() override;
};


}  // namespace OpenOrienteering

#endif
