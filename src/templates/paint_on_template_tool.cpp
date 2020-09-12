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

#include <type_traits>

#include <Qt>
#include <QtMath>
#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QBrush>
#include <QCursor>
#include <QFlags>
#include <QIcon>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QPen>
#include <QPixmap>
#include <QPoint>
#include <QPointF>
#include <QPolygonF>
#include <QRgb>
#include <QSettings>
#include <QVariant>

#include "settings.h"
#include "core/map.h"
#include "core/map_view.h"
#include "gui/util_gui.h"
#include "gui/map/map_editor.h"
#include "gui/map/map_widget.h"
#include "gui/widgets/action_grid_bar.h"
#include "templates/template.h"
#include "util/util.h"


namespace OpenOrienteering {

namespace {

/// Indicates whether a color is considered dark, for the purpose of finding contrasting colors.
constexpr bool isDark(const QRgb rgb) noexcept
{
	constexpr auto medium_gray = 110;
	return qGray(rgb) < medium_gray;
}

bool isDark(const QColor& color) noexcept
{
	return isDark(color.rgb());
}

static_assert(isDark(qRgb(0,0,0)), "black is dark");
static_assert(isDark(qRgb(0,0,255)), "blue is dark");
static_assert(!isDark(qRgb(0,255,0)), "green isn't dark");
static_assert(!isDark(qRgb(255,255,0)), "yellow isn't dark");
static_assert(!isDark(qRgb(255,255,255)), "white isn't dark");

/// Draws a black or white checkmark, with good contrast to the background color. 
void drawCheckmark(QPixmap& pixmap, const QColor& background)
{
	auto const icon_size = pixmap.width();
	auto pen = QPen(QColor(isDark(background) ? Qt::white : Qt::black));
	pen.setWidth(icon_size / 9);
	QPainter p(&pixmap);
	p.setPen(pen);
	p.drawLine(6*icon_size/20, 11*icon_size/20, 8*icon_size/20, 13*icon_size/20);
	p.drawLine(8*icon_size/20, 13*icon_size/20, 13*icon_size/20, 6*icon_size/20);
}

/// Draws a simple eraser icon.
void drawEraser(QPixmap& pixmap, const QColor& background)
{
	auto const icon_size = pixmap.width();
	auto pen = QPen(QColor(isDark(background) ? Qt::white : Qt::black));
	pen.setWidth(icon_size / 9);
	QPainter p(&pixmap);
	p.setPen(pen);
	p.drawLine(8*icon_size/20, 18*icon_size/20, 19*icon_size/20, 18*icon_size/20);
	p.setBrush(QBrush(Qt::gray, Qt::SolidPattern));
	QPoint corners[] = {
	    { 10*icon_size/20,  0*icon_size/20 },
	    {  0*icon_size/20, 14*icon_size/20 },
	    {  8*icon_size/20, 19*icon_size/20 },
	    { 18*icon_size/20,  5*icon_size/20 },
	};
	p.drawPolygon(corners, std::extent<decltype(corners)>::value);
}

/// Create a simple eraser icon, with a checkmark for `QIcon::On` state.
QIcon makeEraserIcon(int const icon_size)
{
	QPixmap pixmap(icon_size, icon_size);
	pixmap.fill(Qt::transparent);
	auto const background = QApplication::palette().color(QPalette::Window);
	drawEraser(pixmap, background);
	QIcon icon(pixmap);
	drawCheckmark(pixmap, background);
	icon.addPixmap(pixmap, QIcon::Normal, QIcon::On);
	return icon;
}

QIcon makeBackgroundDrawingIcon(int const icon_size)
{
	auto const background = QApplication::palette().color(QPalette::Window);
	QRectF circle = {
	    0.1*icon_size,  0.1*icon_size,
	    0.8*icon_size,  0.8*icon_size
	};
	QPointF triangle[] = {
	    { 0,  0 },
	    { 0.8*icon_size, 0 },
	    { 0, 1.4*icon_size },
	};
	
	QPixmap pixmap(icon_size, icon_size);
	pixmap.fill(Qt::transparent);
	
	QPainter p(&pixmap);
	p.setPen(Qt::darkGreen);
	p.setBrush(Qt::green);
	p.drawEllipse(circle);
	p.setPen(Qt::NoPen);
	p.setBrush(QColor(isDark(background) ? Qt::lightGray : Qt::darkGray));
	p.drawPolygon(triangle, std::extent<decltype(triangle)>::value);
	p.end();
	
	return QIcon(pixmap);
}

}


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
	auto const last_selected = settings.value(QStringLiteral("PaintOnTemplateTool/selectedColor")).toString();
	paint_options = Template::ScribbleOption(settings.value(QStringLiteral("PaintOnTemplateTool/options")).toInt()
	                                         & Template::ScribbleOptionsMask);
	
	auto const icon_size = Util::mmToPixelPhysical(Settings::getInstance().getSetting(Settings::ActionGridBar_ButtonSizeMM).toReal());
	
	auto* toolbar = new ActionGridBar(ActionGridBar::Horizontal, 2);
	auto* color_options = new QActionGroup(this);
	
	int count = 0;
	static QColor const default_colors[] = {
	    qRgb(255,   0,   0),
	    qRgb(255, 255,   0),
	    qRgb(  0, 255,   0),
	    qRgb(219,   0, 216),
	    qRgb(  0,   0, 255),
	    qRgb(209,  92,   0),
	    qRgb(  0,   0,   0),
	};
	for (auto const& color: default_colors)
	{
		QPixmap pixmap(icon_size, icon_size);
		pixmap.fill(color);
		QIcon icon(pixmap);
		drawCheckmark(pixmap, color);
		icon.addPixmap(pixmap, QIcon::Normal, QIcon::On);
		
		auto* action = new QAction(icon, color.name(QColor::HexArgb), toolbar);
		action->setCheckable(true);
		action->setActionGroup(color_options);
		toolbar->addAction(action, count % 2, count / 2);
		if (count == 0 || action->text() == last_selected)
		{
			paint_color = color;
			action->setChecked(true);
		}
		connect(action, &QAction::triggered, this, [this, color]() {
			erasing.setFlag(ExplicitErasing, false);
			background_drawing_action->setEnabled(true);
			fill_options->setEnabled(true);
			setColor(color);
		});
		++count;
	}
	
	auto* erase_action = new QAction(makeEraserIcon(icon_size), tr("Erase"), toolbar);
	erase_action->setCheckable(true);
	erase_action->setActionGroup(color_options);
	connect(erase_action, &QAction::triggered, this, [this]() {
		erasing.setFlag(ExplicitErasing, true);
		background_drawing_action->setEnabled(false);
		fill_options->setEnabled(false);
	});
	toolbar->addAction(erase_action, count % 2, count / 2);
	
	background_drawing_action = new QAction(makeBackgroundDrawingIcon(icon_size),
	                                        ::OpenOrienteering::MapEditorController::tr("Background drawing"),
	                                        toolbar);
	background_drawing_action->setCheckable(true);
	background_drawing_action->setChecked(options().testFlag(Template::ComposeBackground));
	connect(background_drawing_action, &QAction::triggered, this, [this](bool checked) {
		setOptions(options().setFlag(Template::ComposeBackground, checked));
	});
	toolbar->addActionAtEnd(background_drawing_action, 0, 1);
	
	auto* fill_action = new QAction(QIcon(QString::fromLatin1(":/images/scribble-fill-shapes.png")),
	                                tr("Filled area"),
	                                toolbar);
	fill_action->setCheckable(true);
	fill_action->setChecked(options().testFlag(Template::FilledAreas));
	connect(fill_action, &QAction::triggered, this, [this](bool checked) {
		setOptions(Template::ScribbleOptions(options()).setFlag(Template::FilledAreas, checked));
	});
	toolbar->addActionAtEnd(fill_action, 1, 1);
	
	auto* fill_options_menu = new QMenu(toolbar);
	fill_action->setMenu(fill_options_menu);
	{
		fill_options = new QActionGroup(fill_action->menu());
		auto const add_option = [this, fill_options_menu](const QString& label, Template::ScribbleOption mode) {
			auto* option = fill_options_menu->addAction(label);
			option->setCheckable(true);
			option->setActionGroup(fill_options);
			option->setChecked((options() & Template::PatternFill) == mode);
			connect(option, &QAction::triggered, this, [this, mode]() {
				setOptions(mode | (options() & ~Template::PatternFill));
			});
		};
		add_option(tr("Solid"), {});
		add_option(tr("Pattern"), Template::PatternFill);
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


void PaintOnTemplateTool::setOptions(Template::ScribbleOptions options)
{
	paint_options = options;
	QSettings().setValue(QStringLiteral("PaintOnTemplateTool/options"), int(options));
}

void PaintOnTemplateTool::setColor(const QColor& color)
{
	paint_color = color;
	QSettings().setValue(QStringLiteral("PaintOnTemplateTool/selectedColor"), color.name(QColor::HexArgb));
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
	if (dragging)
		return true;
	
	if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton)
	{
		coords.push_back(map_coord);
		map_bbox = QRectF(map_coord.x(), map_coord.y(), 0, 0);
		dragging = true;
		erasing.setFlag(RightMouseButtonErasing, event->button() == Qt::RightButton);
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
		
		auto color = paint_color;
		auto width = 0;
		auto mode = options();
		if (erasing)
		{
			color = Qt::transparent;
			mode = mode & Template::FilledAreas;
			if (!mode.testFlag(Template::FilledAreas))
				width = erase_width;
		}
		temp->drawOntoTemplate(&coords[0], int(coords.size()), color, width, map_bbox, mode);
		
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
		QPen pen(erasing ? QColor(Qt::white) : paint_color);
		pen.setCapStyle(Qt::RoundCap);
		pen.setJoinStyle(Qt::RoundJoin);
		
		auto const scale = qMin(temp->getTemplateScaleX(), temp->getTemplateScaleY());
		auto const one_mm = widget->getMapView()->lengthToPixel(1000.0 * scale);
		if (options().testFlag(Template::FilledAreas))
			pen.setWidthF(0.5 * one_mm);
		else if (erasing)
			pen.setWidthF(erase_width * one_mm);
		else
			pen.setWidthF(one_mm);
		
		QPolygonF polygon;
		polygon.reserve(coords.size() + 1);
		for (auto const& coord : coords)
			polygon.append(widget->mapToViewport(coord));

		painter->setPen(pen);
		if (options().testFlag(Template::FilledAreas))
		{
			polygon.append(polygon.front());  // close the polygon
			painter->setBrush(QBrush(pen.color(), Qt::Dense5Pattern));
			painter->drawPolygon(polygon);
		}
		else
		{
			painter->drawPolyline(polygon);
		}
	}
}


}  // namespace OpenOrienteering
