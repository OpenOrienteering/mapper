/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2015, 2017, 2018 Kai Pastor
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

#include <cstddef>
#include <memory>

#include <Qt>
#include <QtMath>
#include <QAbstractButton>
#include <QCoreApplication>
#include <QCursor>
#include <QDir>
#include <QFileInfo>
#include <QFlags>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QIcon>
#include <QImage>
#include <QLatin1Char>
#include <QLatin1String>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPen>
#include <QPixmap>
#include <QPointF>
#include <QPushButton>
#include <QRect>
#include <QRgb>
#include <QSettings>
#include <QVariant>
#include <QVBoxLayout>

#include "core/georeferencing.h"
#include "core/map.h"
#include "core/map_view.h"
#include "gui/main_window.h"
#include "gui/util_gui.h"
#include "gui/map/map_editor.h"
#include "gui/map/map_widget.h"
#include "templates/template.h"
#include "templates/template_image.h"
#include "util/util.h"


namespace OpenOrienteering {

namespace {

/**
 * Determines the base for rounding projected coordinates.
 * 
 * When adding templates for painting, the top left corner of these images
 * is aligned to projected coordinates which are multiples of this base.
 * (However, since map and images usually are not aligned to grid north,
 * images created at different locations will not really align very well.
 * 
 * This function is designed for images of 100 mm at 10 pixel per mm.
 */
int alignmentBase(qreal scale)
{
	auto l = (qLn(scale) / qLn(10)) - 2;
	auto base = 1;
	for (int i = qFloor(l); i > 0; --i)
		base *= 10;
	auto fraction = l - qFloor(l);
	if (fraction > 0.8)
		base *= 10;
	else if (fraction > 0.5)
		base *= 5;
	else if (fraction > 0.2)
		base *= 2;
	return base;
}


/**
 * Rounds x to a multiple of base.
 */
qint64 roundToMultiple(qreal x, int base)
{
	return qRound(x / base) * base;
}


#ifdef MAPPER_DEVELOPMENT_BUILD

bool selfTest()
{
	Q_ASSERT(alignmentBase(20000) == 200);
	Q_ASSERT(alignmentBase(15000) == 100);
	Q_ASSERT(alignmentBase(10000) == 100);
	Q_ASSERT(alignmentBase(7500)  == 100);
	Q_ASSERT(alignmentBase(5000)  == 50);
	Q_ASSERT(alignmentBase(4000)  == 50);
	Q_ASSERT(alignmentBase(1000)  == 10);
	
	Q_ASSERT(roundToMultiple(-347,  10) == -350);
	Q_ASSERT(roundToMultiple(-347,  20) == -340);
	Q_ASSERT(roundToMultiple(-347,  50) == -350);
	Q_ASSERT(roundToMultiple(-347, 100) == -300);
	Q_ASSERT(roundToMultiple(347,  10) == 350);
	Q_ASSERT(roundToMultiple(347,  20) == 340);
	Q_ASSERT(roundToMultiple(347,  50) == 350);
	Q_ASSERT(roundToMultiple(347, 100) == 300);
	
	return true;
}

#endif


} 

// ### PaintOnTemplateTool ###

int PaintOnTemplateTool::erase_width = 4;


PaintOnTemplateTool::PaintOnTemplateTool(MapEditorController* editor, QAction* tool_action)
: MapEditorTool(editor, Other, tool_action)
{
	connect(map(), &Map::templateDeleted, this, &PaintOnTemplateTool::templateDeleted);
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
	connect(editor->getMainWidget(), &QObject::destroyed, widget, [this]() { editor->deletePopupWidget(widget); });
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


void PaintOnTemplateTool::templateDeleted(int /*pos*/, const Template* temp)
{
	if (temp == this->temp)
	{
		temp = nullptr;
		deactivate();
	}
}

void PaintOnTemplateTool::colorSelected(const QColor& color)
{
	paint_color = color;
}

void PaintOnTemplateTool::undoSelected()
{
	if (temp)
		temp->drawOntoTemplateUndo(false);
}

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
		
		temp->drawOntoTemplate(&coords[0], int(coords.size()), erasing ? QColor(255, 255, 255, 0) : paint_color, erasing ? erase_width : 0, map_bbox);
		
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
		painter->setPen(pen);
		
		auto size = coords.size();
		for (std::size_t i = 1; i < size; ++i)
			painter->drawLine(widget->mapToViewport(coords[i - 1]), widget->mapToViewport(coords[i]));
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
			else
			{
				if (selected_color == x + getNumFieldsX()*y)
				{
					int line_width = qMax(1, qRound(Util::mmToPixelLogical(0.5)));
					painter.fillRect(field_rect, Qt::black);
					QPen pen(Qt::white);
					pen.setStyle(Qt::DotLine);
					pen.setWidth(line_width);
					painter.setPen(pen);
					field_rect.adjust(line_width, line_width, -line_width, -line_width);
					painter.drawRect(field_rect);
					field_rect.adjust(line_width, line_width, -line_width, -line_width);
				}
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
	static QColor rows[2][4] = { {qRgb(255,   0,   0), qRgb(  0, 255,   0), qRgb(  0,   0, 255), qRgb(  0,   0,   0)},
	                             {qRgb(255, 255,   0), qRgb(219,   0, 216), qRgb(209,  92,   0), qRgb(255, 255, 255)} };
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

PaintOnTemplateSelectDialog::PaintOnTemplateSelectDialog(Map* map, MapView* view, Template* selected, MainWindow* parent)
: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
, map(map)
, view(view)
{
#ifdef MAPPER_DEVELOPMENT_BUILD
	// With a global static, there is no reasonable output when selfTest fails.
	static bool self_tested = selfTest();
	Q_UNUSED(self_tested);
#endif
#if defined(ANDROID)
	setWindowState((windowState() & ~(Qt::WindowMinimized | Qt::WindowFullScreen))
	               | Qt::WindowMaximized);
#endif
	setWindowTitle(tr("Select template to draw onto"));
	
	template_list = new QListWidget();
	auto current_row = 0;
	/// \todo Review source string (no ellipsis when no dialog)
	auto item = new QListWidgetItem(QCoreApplication::translate("OpenOrienteering::TemplateListWidget", "Add template..."));
	item->setData(Qt::UserRole, qVariantFromValue<void*>(nullptr));
	template_list->addItem(item);
	for (int i = map->getNumTemplates() - 1; i >= 0; --i)
	{
		Template* temp = map->getTemplate(i);
		if (!temp->canBeDrawnOnto() 
		    || temp->getTemplateState() != Template::Loaded)
			continue;
		
		if (temp == selected)
		{
			selection = selected;
			current_row = template_list->count();
		}
		
		auto item = new QListWidgetItem(temp->getTemplateFilename());
		item->setData(Qt::UserRole, qVariantFromValue<void*>(temp));
		template_list->addItem(item);
	}
	template_list->setCurrentRow(current_row);
	
	QPushButton* cancel_button = new QPushButton(tr("Cancel"));
	draw_button = new QPushButton(QIcon(QString::fromLatin1(":/images/pencil.png")), tr("Draw"));
	draw_button->setDefault(true);
	
	auto buttons_layout = new QHBoxLayout();
	buttons_layout->addWidget(cancel_button);
	buttons_layout->addStretch(1);
	buttons_layout->addWidget(draw_button);
	
	auto layout = new QVBoxLayout();
	layout->addWidget(template_list);
	layout->addSpacing(16);
	layout->addLayout(buttons_layout);
	setLayout(layout);
	
	connect(cancel_button, &QAbstractButton::clicked, this, &QDialog::reject);
	connect(draw_button, &QAbstractButton::clicked, this, &PaintOnTemplateSelectDialog::drawClicked);
}

void PaintOnTemplateSelectDialog::drawClicked()
{
	if (auto current = template_list->currentItem())
	{
		selection = reinterpret_cast<Template*>(current->data(Qt::UserRole).value<void*>());
		if (!selection)
			selection = addNewTemplate();
		if (selection)
			accept();
	}
}


Template* PaintOnTemplateSelectDialog::addNewTemplate() const
{
	auto window = qobject_cast<MainWindow*>(parent());
	if (!window || window->currentPath().isEmpty())
		return nullptr;
	
	auto show_message = [window](const QString &message) {
#ifdef Q_OS_ANDROID
		window->showStatusBarMessage(message, 2000);
#else
		QMessageBox::warning(window, OpenOrienteering::Map::tr("Error"), message, QMessageBox::Ok, QMessageBox::Ok);
#endif
	};
	
	// 1000 pixel, 10 pixel per mm, 100 mm per image
	// When these parameters are changed, alignmentBase() needs to be reviewed.
	constexpr auto pixel_per_mm = 10;
	constexpr auto size_mm      = 100;  // multiple of 2
	constexpr auto size_pixel   = size_mm * pixel_per_mm;
	
	// Determine aligned top-left position
	auto top_left = view->center() - MapCoord{size_mm/2, size_mm/2};
	auto projected_top_left = map->getGeoreferencing().toProjectedCoords(top_left);
	const auto base = alignmentBase(map->getScaleDenominator());
	projected_top_left.setX(roundToMultiple(projected_top_left.x(), base));
	projected_top_left.setY(roundToMultiple(projected_top_left.y(), base));
	top_left = map->getGeoreferencing().toMapCoords(projected_top_left);
	
	// Find or create a template for the track with a specific name
	const QString filename = QLatin1String("Draft @ ")
	                          + QString::number(qRound64(projected_top_left.x()))
	                          + QLatin1Char(',')
			                  + QString::number(qRound64(projected_top_left.y()))
	                          + QLatin1String(".png");
	QString image_file_path = QFileInfo(window->currentPath()).absoluteDir().canonicalPath()
	                          + QLatin1Char('/')
	                          + filename;
	if (QFileInfo::exists(image_file_path))
	{
		show_message(tr("Template file exists: '%1'").arg(filename));
		return nullptr;
	}
	
	auto image = QImage(size_pixel, size_pixel, QImage::Format_ARGB32);
	image.fill(QColor(Qt::transparent));
	QPainter p(&image);
	p.setPen(QColor(Qt::red));
	p.drawRect(0, 0, size_pixel-1, size_pixel-1);
	p.drawText(pixel_per_mm/2, pixel_per_mm/2 + QFontMetrics(p.font()).ascent(),
	           QFileInfo(image_file_path).fileName());
	p.end();
	if (!image.save(image_file_path))
	{
		show_message(OpenOrienteering::Map::tr("Cannot save file\n%1:\n%2").arg(filename, QString{}));
		return nullptr;
	}
	
	auto temp = new TemplateImage{image_file_path, map};
	temp->setTemplatePosition(MapCoord{top_left + MapCoordF{size_mm/2, size_mm/2}});
	temp->setTemplateScaleX(1.0/pixel_per_mm);
	temp->setTemplateScaleY(1.0/pixel_per_mm);
	temp->setTemplateRotation(0);
	temp->loadTemplateFile(false);
	
	auto pos = map->getFirstFrontTemplate();
	map->addTemplate(temp, pos);
	map->setTemplateAreaDirty(pos);
	map->setFirstFrontTemplate(pos + 1);
	map->setTemplatesDirty();
	
	return temp;
}



}  // namespace OpenOrienteering
