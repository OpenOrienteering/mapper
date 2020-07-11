/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2020 Kai Pastor
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


#include "paint_on_template_tool.h"

#include <Qt>
#include <QtMath>
#include <QAction>
#include <QButtonGroup>
#include <QBrush>
#include <QCursor>
#include <QFlags>
#include <QIcon>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QPolygonF>
#include <QRgb>
#include <QSettings>
#include <QToolButton>
#include <QVariant>

#include "core/map.h"
#include "core/map_view.h"
#include "gui/map/map_editor.h"
#include "gui/map/map_widget.h"
#include "gui/widgets/action_grid_bar.h"
#include "templates/template.h"
#include "util/util.h"


namespace OpenOrienteering {


int PaintOnTemplateTool::erase_width = 4;


PaintOnTemplateTool::PaintOnTemplateTool(MapEditorController* editor, QAction* tool_action)
: MapEditorTool(editor, Scribble, tool_action)
{
	connect(map(), &Map::templateAboutToBeDeleted, this, &PaintOnTemplateTool::templateAboutToBeDeleted);
}

PaintOnTemplateTool::~PaintOnTemplateTool()
{
	if (widget)
		editor->deletePopupWidget(widget);
}

void PaintOnTemplateTool::setTemplate(Template* temp)
{
	this->temp = temp;
}

void PaintOnTemplateTool::init()
{
	setStatusBarText(tr("<b>Click and drag</b>: Paint. <b>Right click and drag</b>: Erase. "));
	
	widget = makeToolBar();
	editor->showPopupWidget(widget, tr("Color selection"));
	
	MapEditorTool::init();
}

ActionGridBar* PaintOnTemplateTool::makeToolBar()
{
	QSettings settings;
	settings.beginGroup(QStringLiteral("PaintOnTemplateTool"));
	auto last_selected = settings.value(QStringLiteral("selectedColor")).toString();
	
	auto* toolbar = new ActionGridBar(ActionGridBar::Horizontal, 2);
	auto* color_button_group = new QButtonGroup(this);
	
	int count = 0;
	static QColor const default_colors[8] = {
	    qRgb(255,   0,   0),
	    qRgb(255, 255,   0),
	    qRgb(  0, 255,   0),
	    qRgb(219,   0, 216),
	    qRgb(  0,   0, 255),
	    qRgb(209,  92,   0),
	    qRgb(  0,   0,   0),
	    qRgb(255, 255, 255)
	};
	for (auto const& color: default_colors)
	{
		QPixmap pixmap(1,1);
		pixmap.fill(color);
		auto* action = new QAction(pixmap, color.name(QColor::HexArgb), toolbar);
		action->setCheckable(true);
		toolbar->addAction(action, count % 2, count / 2);
		color_button_group->addButton(toolbar->getButtonForAction(action));
		if (count == 0 || action->text() == last_selected)
		{
			paint_color = color;
			action->setChecked(true);
		}
		connect(action, &QAction::triggered, this, [this, color](bool checked) { if (checked) colorSelected(color); });
		++count;
	}
	
	auto* undo_action = new QAction(QIcon(QString::fromLatin1(":/images/undo.png")),
	                                ::OpenOrienteering::MapEditorController::tr("Undo"),
	                                toolbar);
	connect(undo_action, &QAction::triggered, this, &PaintOnTemplateTool::undoSelected);
	toolbar->addActionAtEnd(undo_action, 0, 0);
	
	auto* redo_action = new QAction(QIcon(QString::fromLatin1(":/images/redo.png")),
	                                ::OpenOrienteering::MapEditorController::tr("Redo"),
	                                toolbar);
	connect(redo_action, &QAction::triggered, this, &PaintOnTemplateTool::redoSelected);
	toolbar->addActionAtEnd(redo_action, 1, 0);
	
	auto* fill_action = new QAction(QIcon(QString::fromLatin1(":/images/scribble-fill-shapes.png")),
	                                tr("Filled area"),
	                                toolbar);
	fill_action->setCheckable(true);
	connect(fill_action, &QAction::triggered, this, &PaintOnTemplateTool::setFillAreas);
	toolbar->addActionAtEnd(fill_action, 1, 1);
	
	return toolbar;
}

const QCursor& PaintOnTemplateTool::getCursor() const
{
	static auto const cursor = scaledToScreen(QCursor{ QPixmap(QString::fromLatin1(":/images/cursor-paint-on-template.png")), 1, 1 });
	return cursor;
}


// slot
void PaintOnTemplateTool::templateAboutToBeDeleted(int /*pos*/, Template* temp)
{
	if (temp == this->temp)
	{
		this->temp = nullptr;
		deactivate();
	}
}

// slot
void PaintOnTemplateTool::setFillAreas(bool enabled)
{
	fill_areas = enabled;
}

// slot
void PaintOnTemplateTool::colorSelected(const QColor& color)
{
	paint_color = color;
	
	QSettings settings;
	settings.beginGroup(QStringLiteral("PaintOnTemplateTool"));
	settings.setValue(QStringLiteral("selectedColor"), color.name(QColor::HexArgb));
}

// slot
void PaintOnTemplateTool::undoSelected()
{
	if (temp)
		temp->drawOntoTemplateUndo(false);
}

// slot
void PaintOnTemplateTool::redoSelected()
{
	if (temp)
		temp->drawOntoTemplateUndo(true);
}


bool PaintOnTemplateTool::mousePressEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* /*widget*/)
{
	if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton)
	{
		coords.push_back(map_coord);
		map_bbox = QRectF(map_coord.x(), map_coord.y(), 0, 0);
		dragging = true;
		erasing = (event->button() == Qt::RightButton) || (paint_color == qRgb(255, 255, 255));
		return true;
	}
	
	return false;
}

bool PaintOnTemplateTool::mouseMoveEvent(QMouseEvent* /*event*/, const MapCoordF& map_coord, MapWidget* widget)
{
	if (dragging && temp)
	{
		auto scale = qMin(temp->getTemplateScaleX(), temp->getTemplateScaleY());
		
		coords.push_back(map_coord);
		rectInclude(map_bbox, map_coord);
		
		auto pixel_border = widget->getMapView()->lengthToPixel(1000.0 * scale * (erasing ? erase_width/2 : 1));
		map()->setDrawingBoundingBox(map_bbox, qCeil(pixel_border));
		
		return true;
	}
	
	return false;
}

bool PaintOnTemplateTool::mouseReleaseEvent(QMouseEvent* /*event*/, const MapCoordF& map_coord, MapWidget* /*widget*/)
{
	if (dragging && temp)
	{
		coords.push_back(map_coord);
		rectInclude(map_bbox, map_coord);
		
		auto mode = QFlags<Template::ScribbleOption>()
		            .setFlag(Template::FilledAreas, fillAreas());
		
		temp->drawOntoTemplate(&coords[0], int(coords.size()),
		        erasing ? QColor(255, 255, 255, 0) : paint_color,
		        erasing ? erase_width : 0, map_bbox, mode);
		
		coords.clear();
		map()->clearDrawingBoundingBox();
		
		dragging = false;
		return true;
	}
	
	return false;
}


void PaintOnTemplateTool::draw(QPainter* painter, MapWidget* widget)
{
	if (dragging && temp)
	{
		auto scale = qMin(temp->getTemplateScaleX(), temp->getTemplateScaleY());
		
		QPen pen(erasing ? qRgb(255, 255, 255) : paint_color);
		pen.setWidthF(widget->getMapView()->lengthToPixel(1000.0 * scale * (erasing ? erase_width : 1)));
		pen.setCapStyle(Qt::RoundCap);
		pen.setJoinStyle(Qt::RoundJoin);
		
		QPolygonF polygon;
		polygon.reserve(coords.size());
		for (auto const& coord : coords)
			polygon.append(widget->mapToViewport(coord));

		if (fillAreas())
		{
			painter->setPen(Qt::NoPen);
			painter->setBrush(QBrush(pen.color(), Qt::Dense5Pattern));
			painter->drawPolygon(polygon);
		}

		painter->setPen(pen);
		painter->drawPolyline(polygon);
	}
}


}  // namespace OpenOrienteering
