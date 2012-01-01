/*
 *    Copyright 2011 Thomas Schöps
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


#include "paint_on_template.h"

#include <QtGui>

#include "template.h"
#include "map_widget.h"
#include "util.h"

QCursor* PaintOnTemplateTool::cursor = NULL;
int PaintOnTemplateTool::erase_width = 4;

PaintOnTemplateTool::PaintOnTemplateTool(MapEditorController* editor, QAction* tool_button, Template* temp) : MapEditorTool(editor, tool_button)
{
	paint_color = Qt::black;
	dragging = false;
	
	this->temp = temp;
	connect(editor->getMap(), SIGNAL(templateDeleted(Template*)), this, SLOT(templateDeleted(Template*)));
	
	if (!cursor)
		cursor = new QCursor(QPixmap("images/cursor-paint-on-template.png"), 1, 1);
}
PaintOnTemplateTool::~PaintOnTemplateTool()
{
	delete dock_widget;
}
void PaintOnTemplateTool::init()
{
	setStatusBarText(tr("<b>Left mouse click and drag</b> to paint, <b>Right mouse click and drag</b> to erase"));
	
	dock_widget = new QDockWidget(tr("Color selection"), editor->getWindow());
	dock_widget->setFeatures(dock_widget->features() & ~QDockWidget::DockWidgetClosable);
	widget = new PaintOnTemplatePaletteWidget(false);
	dock_widget->setWidget(widget);
	
	// Show dock in floating state
	dock_widget->setFloating(true);
	dock_widget->show();
	dock_widget->setGeometry(editor->getWindow()->geometry().left() + 40, editor->getWindow()->geometry().top() + 100, dock_widget->width(), dock_widget->height());
	
	connect(widget, SIGNAL(colorSelected(QColor)), this, SLOT(colorSelected(QColor)));
}
void PaintOnTemplateTool::templateDeleted(Template* temp)
{
	if (temp == this->temp)
		editor->setTool(NULL);
}
void PaintOnTemplateTool::colorSelected(QColor color)
{
	paint_color = color;
}

bool PaintOnTemplateTool::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton)
	{
		coords.push_back(map_coord);
		map_bbox = QRectF(map_coord.getX(), map_coord.getY(), 0, 0);
		dragging = true;
		erasing = (event->button() == Qt::RightButton) || (paint_color == qRgb(255, 255, 255));
		return true;
	}
	else
		return false;
}
bool PaintOnTemplateTool::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (dragging)
	{
		float scale = qMin(temp->getTemplateFinalScaleX(), temp->getTemplateFinalScaleY());
		
		coords.push_back(map_coord);
		rectInclude(map_bbox, map_coord.toQPointF());
		
		editor->getMap()->setDrawingBoundingBox(map_bbox, widget->getMapView()->lengthToPixel((erasing ? (erase_width/2) : 1) * 1000 * scale * 1));
		
		return true;
	}
	else
		return false;
}
bool PaintOnTemplateTool::mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (dragging)
	{
		coords.push_back(map_coord);
		rectInclude(map_bbox, map_coord.toQPointF());
		
		temp->drawOntoTemplate(&coords[0], coords.size(), erasing ? qRgb(255, 255, 255) : paint_color, erasing ? erase_width : 0, map_bbox);
		
		coords.clear();
		editor->getMap()->clearDrawingBoundingBox();
		
		dragging = false;
		return true;
	}
	else
		return false;
}

void PaintOnTemplateTool::draw(QPainter* painter, MapWidget* widget)
{
	float scale = qMin(temp->getTemplateFinalScaleX(), temp->getTemplateFinalScaleY());
	
	QPen pen(erasing ? qRgb(255, 255, 255) : paint_color);
	pen.setWidthF(widget->getMapView()->lengthToPixel(1000 * scale * (erasing ? erase_width : 1)));
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
}
void PaintOnTemplatePaletteWidget::paintEvent(QPaintEvent* event)
{
	QPainter painter;
	painter.begin(this);
	painter.setClipRect(event->rect());
	
	int max_x = getNumFieldsX();
	int max_y = getNumFieldsY();
	for (int x = 0; x < max_x; ++x)
		for (int y = 0; y < getNumFieldsY(); ++y)
			painter.fillRect(QRect(width() * x / max_x, height() * y / max_y,
								   width() * (x+1) / max_x - (width() * x / max_x), height() * (y+1) / max_y - (height() * y / max_y)),
								   getFieldColor(x, y));
	
	painter.end();
}
void PaintOnTemplatePaletteWidget::mousePressEvent(QMouseEvent* event)
{
    int x = (int)(event->x() / (width() / (float)getNumFieldsX()));
	int y = (int)(event->y() / (height() / (float)getNumFieldsY()));
	
	emit colorSelected(getFieldColor(x, y));
	
	if (close_on_selection)
		close();
}

int PaintOnTemplatePaletteWidget::getNumFieldsX()
{
	return 4;
}
int PaintOnTemplatePaletteWidget::getNumFieldsY()
{
	return 2;
}
QColor PaintOnTemplatePaletteWidget::getFieldColor(int x, int y)
{
	static QColor rows[2][4] = {{qRgb(255, 0, 0), qRgb(0, 255, 0), qRgb(0, 0, 255), qRgb(0, 0, 0)}, {qRgb(255, 255, 0), qRgb(219, 0, 216), qRgb(219, 180, 126), qRgb(255, 255, 255)}};
	return rows[y][x];
}

// ### PaintOnTemplateSelectDialog ###

PaintOnTemplateSelectDialog::PaintOnTemplateSelectDialog(Map* map, QWidget* parent) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
{
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
	draw_button = new QPushButton(QIcon("images/pencil.png"), tr("Draw"));
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
	draw_button->setEnabled(current != NULL);
	if (current)
		selection = reinterpret_cast<Template*>(current->data(Qt::UserRole).value<void*>());
}

#include "paint_on_template.moc"
