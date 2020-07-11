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
#include <QBrush>
#include <QCursor>
#include <QFlags>
#include <QIcon>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPen>
#include <QPixmap>
#include <QPolygonF>
#include <QRect>
#include <QRgb>
#include <QSettings>
#include <QVariant>

#include "core/map.h"
#include "core/map_view.h"
#include "gui/util_gui.h"
#include "gui/map/map_editor.h"
#include "gui/map/map_widget.h"
#include "templates/template.h"
#include "util/util.h"


namespace OpenOrienteering {

// ### PaintOnTemplateTool ###

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
	
	widget = new PaintOnTemplatePaletteWidget(false);
	editor->showPopupWidget(widget, tr("Color selection"));
	connect(widget, &PaintOnTemplatePaletteWidget::colorSelected, this, &PaintOnTemplateTool::colorSelected);
	connect(widget, &PaintOnTemplatePaletteWidget::undoSelected, this, &PaintOnTemplateTool::undoSelected);
	connect(widget, &PaintOnTemplatePaletteWidget::redoSelected, this, &PaintOnTemplateTool::redoSelected);
	colorSelected(widget->getSelectedColor());
	
	MapEditorTool::init();
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
void PaintOnTemplateTool::colorSelected(const QColor& color)
{
	paint_color = color;
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
		            .setFlag(Template::FilledAreas, widget->getFillShapes());
		
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

		if (this->widget->getFillShapes())
		{
			painter->setPen(Qt::NoPen);
			painter->setBrush(QBrush(pen.color(), Qt::Dense5Pattern));
			painter->drawPolygon(polygon);
		}

		painter->setPen(pen);
		painter->drawPolyline(polygon);
	}
}



// ### PaintOnTemplatePaletteWidget ###

PaintOnTemplatePaletteWidget::PaintOnTemplatePaletteWidget(bool close_on_selection)
: QWidget()
, close_on_selection(close_on_selection)
{
	setAttribute(Qt::WA_DeleteOnClose);
	setAttribute(Qt::WA_OpaquePaintEvent);
	setAutoFillBackground(false);

	QSettings settings;
	settings.beginGroup(QString::fromLatin1("PaintOnTemplatePaletteWidget"));
	selected_color = qMax(0, qMin(getNumFieldsX()*getNumFieldsY() - 1, settings.value(QString::fromLatin1("selectedColor")).toInt()));
	settings.endGroup();
	
	pressed_buttons = 0;
}

PaintOnTemplatePaletteWidget::~PaintOnTemplatePaletteWidget()
{
	QSettings settings;
	settings.beginGroup(QString::fromLatin1("PaintOnTemplatePaletteWidget"));
	settings.setValue(QString::fromLatin1("selectedColor"), selected_color);
	settings.endGroup();
}

QColor PaintOnTemplatePaletteWidget::getSelectedColor()
{
	return getFieldColor(selected_color % getNumFieldsX(), selected_color / getNumFieldsX());
}


QSize PaintOnTemplatePaletteWidget::sizeHint() const
{
	constexpr qreal button_size_mm = 13;
	return QSize(qCeil(getNumFieldsX() * Util::mmToPixelLogical(button_size_mm)),
	             qCeil(getNumFieldsY() * Util::mmToPixelLogical(button_size_mm)));
}


void PaintOnTemplatePaletteWidget::paintEvent(QPaintEvent* event)
{
	QPainter painter;
	painter.begin(this);
	painter.setClipRect(event->rect());
	
	auto draw_field_selection_marker = [&painter](QRect field_rect) {
		int line_width = qMax(1, qRound(Util::mmToPixelLogical(0.5)));
		painter.fillRect(field_rect, Qt::black);
		QPen pen(Qt::white);
		pen.setStyle(Qt::DotLine);
		pen.setWidth(line_width);
		painter.setPen(pen);
		field_rect.adjust(line_width, line_width, -line_width, -line_width);
		painter.drawRect(field_rect);
		field_rect.adjust(line_width, line_width, -line_width, -line_width);
		return field_rect;
	};

	int max_x = getNumFieldsX();
	int max_y = getNumFieldsY();
	for (int x = 0; x < max_x; ++x)
	{
		for (int y = 0; y < max_y; ++y)
		{
			int field_x = width() * x / max_x;
			int field_y = height() * y / max_y;
			int field_width = width() * (x+1) / max_x - (width() * x / max_x);
			int field_height = height() * (y+1) / max_y - (height() * y / max_y);
			QRect field_rect = QRect(field_x, field_y, field_width, field_height);

			if (isUndoField(x, y))
				drawIcon(&painter, QString::fromLatin1(":/images/undo.png"), field_rect);
			else if (isRedoField(x, y))
				drawIcon(&painter, QString::fromLatin1(":/images/redo.png"), field_rect);
			else if (isFillShapesField(x, y))
			{
				if (fill_shapes)
					field_rect = draw_field_selection_marker(field_rect);
				drawIcon(&painter, QString::fromLatin1(":/images/scribble-fill-shapes.png"), field_rect);
			}
			else if (!isEmptyField(x, y))
			{
				if (selected_color == x + getNumFieldsX()*y)
					field_rect = draw_field_selection_marker(field_rect);
				painter.fillRect(field_rect, getFieldColor(x, y));
			}
		}
	}
	
	painter.end();
}


void PaintOnTemplatePaletteWidget::mousePressEvent(QMouseEvent* event)
{
	if (event->button() != Qt::LeftButton)
		return;
	
	// Workaround for multiple press bug on Android
	if ((event->button() & ~pressed_buttons) == 0)
		return;
	pressed_buttons |= event->button();
	
	int x = int(event->x() / (width() / qreal(getNumFieldsX())));
	int y = int(event->y() / (height() / qreal(getNumFieldsY())));
	
	if (isUndoField(x, y))
	{
		emit undoSelected();
	}
	else if (isRedoField(x, y))
	{
		emit redoSelected();
	}
	else if (isFillShapesField(x, y))
	{
		fill_shapes = !fill_shapes;
		update();
	}
	else if (!isEmptyField(x, y) && selected_color != x + getNumFieldsX()*y)
	{
		selected_color = x + getNumFieldsX()*y;
		update();
		emit colorSelected(getFieldColor(x, y));
	}
	
	if (close_on_selection)
		close();
}

void PaintOnTemplatePaletteWidget::mouseReleaseEvent(QMouseEvent* event)
{
	// Workaround for multiple press bug on Android part 2, see above in mousePressEvent()
	if ((event->button() & pressed_buttons) == 0)
		return;
	pressed_buttons &= ~event->button();
}


int PaintOnTemplatePaletteWidget::getNumFieldsX() const
{
	return 6;
}

int PaintOnTemplatePaletteWidget::getNumFieldsY() const
{
	return 2;
}

QColor PaintOnTemplatePaletteWidget::getFieldColor(int x, int y) const
{
	static QColor rows[2][4] = { {qRgb(255,   0,   0), qRgb(  0, 255,   0), qRgb(  0,   0, 255), qRgb(  0,   0,   0)},
	                             {qRgb(255, 255,   0), qRgb(219,   0, 216), qRgb(209,  92,   0), qRgb(255, 255, 255)} };
	return rows[y][x];
}

bool PaintOnTemplatePaletteWidget::isEmptyField(int x, int y) const
{
	return x == 5 && y == 0;
}

bool PaintOnTemplatePaletteWidget::isFillShapesField(int x, int y) const
{
	return x == 5 && y == 1;
}

bool PaintOnTemplatePaletteWidget::isUndoField(int x, int y) const
{
	return x == 4 && y == 0;
}

bool PaintOnTemplatePaletteWidget::isRedoField(int x, int y) const
{
	return x == 4 && y == 1;
}

void PaintOnTemplatePaletteWidget::drawIcon(QPainter* painter, const QString& resource_path, const QRect& field_rect)
{
	painter->fillRect(field_rect, Qt::white);
	QIcon icon(resource_path);
	painter->setRenderHint(QPainter::Antialiasing);
	painter->drawPixmap(field_rect, icon.pixmap(field_rect.size()));
	painter->setRenderHint(QPainter::Antialiasing, false);
}


}  // namespace OpenOrienteering
