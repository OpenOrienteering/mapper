/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2014, 2016 Kai Pastor
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


#ifndef OPENORIENTEERING_TOOL_ROTATE_PATTERN_H
#define OPENORIENTEERING_TOOL_ROTATE_PATTERN_H

#include <QObject>
#include <QString>

#include "tools/tool_base.h"

class QAction;
class QKeyEvent;
class QPainter;
class QRectF;

namespace OpenOrienteering {

class MapEditorController;
class MapWidget;


/**
 * Tool to rotate patterns of area objects, or to set the direction of rotatable point objects.
 */
class RotatePatternTool : public MapEditorToolBase
{
Q_OBJECT
public:
	RotatePatternTool(MapEditorController* editor, QAction* tool_action);
	~RotatePatternTool() override;
	
	bool keyPressEvent(QKeyEvent* event) override;
	bool keyReleaseEvent(QKeyEvent* event) override;
	
protected:
	void clickPress() override;
	void clickRelease() override;
	void dragStart() override;
	void dragMove() override;
	void dragFinish() override;
	
	void initImpl() override;
	void drawImpl(QPainter* painter, MapWidget* widget) override;
	int updateDirtyRectImpl(QRectF& rect) override;
	void updateStatusText() override;
	void objectSelectionChangedImpl() override;
};


}  // namespace OpenOrienteering

#endif
