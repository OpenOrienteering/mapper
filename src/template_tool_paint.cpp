/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2015 Kai Pastor
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


#include "template_tool_paint.h"

#include <QListWidget>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

#include "map_widget.h"
#include "template.h"
#include "util.h"
#include "map_editor.h"


// ### PaintOnTemplateTool ###

int PaintOnTemplateTool::erase_width = 4;

PaintOnTemplateTool::PaintOnTemplateTool(MapEditorController* editor, QAction* tool_button, Template* temp) : MapEditorTool(editor, Other, tool_button)
{
	paint_color = Qt::black;
	dragging = false;
	
	this->temp = temp;
	connect(map(), SIGNAL(templateDeleted(int, const Template*)), this, SLOT(templateDeleted(int, const Template*)));
}

PaintOnTemplateTool::~PaintOnTemplateTool()
{
	editor->deletePopupWidget(widget);
}

void PaintOnTemplateTool::init()
{
	setStatusBarText(tr("<b>Click and drag</b>: Paint. <b>Right click and drag</b>: Erase. "));
	
	widget = new PaintOnTemplatePaletteWidget(false);
	editor->showPopupWidget(widget, tr("Color selection"));
	connect(widget, SIGNAL(colorSelected(QColor)), this, SLOT(colorSelected(QColor)));
	connect(widget, SIGNAL(undoSelected()), this, SLOT(undoSelected()));
	connect(widget, SIGNAL(redoSelected()), this, SLOT(redoSelected()));
	colorSelected(widget->getSelectedColor());
	
	MapEditorTool::init();
}

const QCursor&PaintOnTemplateTool::getCursor() const
{
	static auto const cursor = scaledToScreen(QCursor{ QPixmap(QString::fromLatin1(":/images/cursor-paint-on-template.png")), 1, 1 });
	return cursor;
}

void PaintOnTemplateTool::templateDeleted(int pos, const Template* temp)
{
	Q_UNUSED(pos);
	if (temp == this->temp)
		deactivate();
}

void PaintOnTemplateTool::colorSelected(QColor color)
{
	paint_color = color;
}

void PaintOnTemplateTool::undoSelected()
{
	temp->drawOntoTemplateUndo(false);
}

void PaintOnTemplateTool::redoSelected()
{
	temp->drawOntoTemplateUndo(true);
}

bool PaintOnTemplateTool::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	Q_UNUSED(widget);
	
	if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton)
	{
		coords.push_back(map_coord);
		map_bbox = QRectF(map_coord.x(), map_coord.y(), 0, 0);
		dragging = true;
		erasing = (event->button() == Qt::RightButton) || (paint_color == qRgb(255, 255, 255));
		return true;
	}
	else
		return false;
}

bool PaintOnTemplateTool::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	Q_UNUSED(event);
	
	if (dragging)
	{
		float scale = qMin(temp->getTemplateScaleX(), temp->getTemplateScaleY());
		
		coords.push_back(map_coord);
		rectInclude(map_bbox, map_coord);
		
		auto pixel_border = widget->getMapView()->lengthToPixel(1000.0 * scale * (erasing ? erase_width/2 : 1));
		map()->setDrawingBoundingBox(map_bbox, pixel_border);
		
		return true;
	}
	else
		return false;
}

bool PaintOnTemplateTool::mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	Q_UNUSED(event);
	Q_UNUSED(widget);
	
	if (dragging)
	{
		coords.push_back(map_coord);
		rectInclude(map_bbox, map_coord);
		
		temp->drawOntoTemplate(&coords[0], coords.size(), erasing ? QColor(255, 255, 255, 0) : paint_color, erasing ? erase_width : 0, map_bbox);
		
		coords.clear();
		map()->clearDrawingBoundingBox();
		
		dragging = false;
		return true;
	}
	else
		return false;
}

void PaintOnTemplateTool::draw(QPainter* painter, MapWidget* widget)
{
	float scale = qMin(temp->getTemplateScaleX(), temp->getTemplateScaleY());
	
	QPen pen(erasing ? qRgb(255, 255, 255) : paint_color);
	pen.setWidthF(widget->getMapView()->lengthToPixel(1000.0 * scale * (erasing ? erase_width : 1)));
	pen.setCapStyle(Qt::RoundCap);
	pen.setJoinStyle(Qt::RoundJoin);
	painter->setPen(pen);
	
	int size = (int)coords.size();
	for (int i = 1; i < size; ++i)
		painter->drawLine(widget->mapToViewport(coords[i - 1]), widget->mapToViewport(coords[i]));
}

// ### PaintOnTemplatePaletteWidget ###

PaintOnTemplatePaletteWidget::PaintOnTemplatePaletteWidget(bool close_on_selection) : QWidget(), close_on_selection(close_on_selection)
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
	const float mmPerButton = 13;
	return QSize(getNumFieldsX() * Util::mmToPixelLogical(mmPerButton),
				 getNumFieldsY() * Util::mmToPixelLogical(mmPerButton));
}

void PaintOnTemplatePaletteWidget::paintEvent(QPaintEvent* event)
{
	QPainter painter;
	painter.begin(this);
	painter.setClipRect(event->rect());
	
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
			else if (selected_color == x + getNumFieldsX()*y)
			{
				int line_width = qMax(1, qRound(Util::mmToPixelLogical(0.2f)));
				painter.fillRect(field_rect, Qt::black);
				painter.fillRect(field_rect.adjusted(line_width, line_width, -1 * line_width, -field_rect.height() / 2), Qt::white);
				painter.fillRect(field_rect.adjusted(2 * line_width, 2 * line_width, -2 * line_width, -2 * line_width), getFieldColor(x, y));
			}
			else
				painter.fillRect(field_rect, getFieldColor(x, y));
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
	
    int x = (int)(event->x() / (width() / (float)getNumFieldsX()));
	int y = (int)(event->y() / (height() / (float)getNumFieldsY()));
	
	if (isUndoField(x, y))
	{
		emit undoSelected();
	}
	else if (isRedoField(x, y))
	{
		emit redoSelected();
	}
	else if (selected_color != x + getNumFieldsX()*y)
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
	return 5;
}

int PaintOnTemplatePaletteWidget::getNumFieldsY() const
{
	return 2;
}

QColor PaintOnTemplatePaletteWidget::getFieldColor(int x, int y) const
{
	static QColor rows[2][4] = {{qRgb(255, 0, 0), qRgb(0, 255, 0), qRgb(0, 0, 255), qRgb(0, 0, 0)}, {qRgb(255, 255, 0), qRgb(219, 0, 216), qRgb(219, 180, 126), qRgb(255, 255, 255)}};
	return rows[y][x];
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

// ### PaintOnTemplateSelectDialog ###

PaintOnTemplateSelectDialog::PaintOnTemplateSelectDialog(Map* map, QWidget* parent) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
{
#if defined(ANDROID)
	setWindowState((windowState() & ~(Qt::WindowMinimized | Qt::WindowFullScreen))
                   | Qt::WindowMaximized);
#endif
	setWindowTitle(tr("Select template to draw onto"));
	
	QListWidget* template_list = new QListWidget();
	for (int i = map->getNumTemplates() - 1; i >= 0; --i)
	{
		Template* temp = map->getTemplate(i);
		if (!temp->canBeDrawnOnto())
			continue;
		
		QListWidgetItem* item = new QListWidgetItem(temp->getTemplateFilename());
		item->setData(Qt::UserRole, qVariantFromValue<void*>(temp));
		template_list->addItem(item);
	}
	
	QPushButton* cancel_button = new QPushButton(tr("Cancel"));
	draw_button = new QPushButton(QIcon(QString::fromLatin1(":/images/pencil.png")), tr("Draw"));
	draw_button->setDefault(true);
	
	QHBoxLayout* buttons_layout = new QHBoxLayout();
	buttons_layout->addWidget(cancel_button);
	buttons_layout->addStretch(1);
	buttons_layout->addWidget(draw_button);
	
	QVBoxLayout* layout = new QVBoxLayout();
	layout->addWidget(template_list);
	layout->addSpacing(16);
	layout->addLayout(buttons_layout);
	setLayout(layout);
	
	connect(cancel_button, SIGNAL(clicked(bool)), this, SLOT(reject()));
	connect(draw_button, SIGNAL(clicked(bool)), this, SLOT(accept()));
	connect(template_list, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), this, SLOT(currentTemplateChanged(QListWidgetItem*,QListWidgetItem*)));
	
	selection = NULL;
	template_list->setCurrentRow(0);
	draw_button->setEnabled(selection != NULL);
}

void PaintOnTemplateSelectDialog::currentTemplateChanged(QListWidgetItem* current, QListWidgetItem* previous)
{
	Q_UNUSED(previous);
	
	draw_button->setEnabled(current != NULL);
	if (current)
		selection = reinterpret_cast<Template*>(current->data(Qt::UserRole).value<void*>());
}
