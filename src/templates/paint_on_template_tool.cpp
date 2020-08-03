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
#include <QApplication>
#include <QButtonGroup>
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
#include <QPolygonF>
#include <QRgb>
#include <QSettings>
#include <QToolButton>
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
	static QPoint corners[] = {
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
	settings.beginGroup(QStringLiteral("PaintOnTemplateTool"));
	auto last_selected = settings.value(QStringLiteral("selectedColor")).toString();
	
	auto const icon_size = Util::mmToPixelPhysical(Settings::getInstance().getSetting(Settings::ActionGridBar_ButtonSizeMM).toReal());
	
	auto* toolbar = new ActionGridBar(ActionGridBar::Horizontal, 2);
	auto* color_button_group = new QButtonGroup(this);
	
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
		toolbar->addAction(action, count % 2, count / 2);
		color_button_group->addButton(toolbar->getButtonForAction(action));
		if (count == 0 || action->text() == last_selected)
		{
			paint_color = color;
			action->setChecked(true);
		}
		connect(action, &QAction::triggered, this, [this, color](bool checked) { if (checked) setColor(color); });
		++count;
	}
	
	auto const mode_descriptor = settings.value(QStringLiteral("drawingMode")).toString().split(u',');
	
	auto* erase_action = new QAction(makeEraserIcon(icon_size), tr("Erase"), toolbar);
	erase_action->setCheckable(true);
	connect(erase_action, &QAction::triggered, this, [this](bool enabled) { erasing.setFlag(ExplicitErasing, enabled); storePaintToolStatus(); });
	toolbar->addAction(erase_action, count % 2, count / 2);
	// de-select color when activating eraser
	color_button_group->addButton(toolbar->getButtonForAction(erase_action));
	++count;
	if (mode_descriptor.contains(QLatin1String("erasing")))
	{
		erasing.setFlag(ExplicitErasing, true);
		erase_action->setChecked(true);
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
	
	auto* preserve_drawing_action = new QAction(QIcon(QString::fromLatin1(":/images/plus.png")),
	                                ::OpenOrienteering::MapEditorController::tr("Preserve drawing"),
	                                toolbar);
	preserve_drawing_action->setCheckable(true);
	connect(preserve_drawing_action, &QAction::triggered, this, &PaintOnTemplateTool::setpreserveDrawing);
	toolbar->addActionAtEnd(preserve_drawing_action, 0, 1);
	auto* preserve_drawing_button = toolbar->getButtonForAction(preserve_drawing_action);
	auto* drawing_options_menu = new QMenu(preserve_drawing_button);
	auto* preserve_drawing_option = drawing_options_menu->addAction(tr("Overlay drawing"));
	preserve_drawing_option->setCheckable(true);
	connect(preserve_drawing_option, &QAction::triggered, this, [this](bool enabled) {
		overlay_drawing_mode = enabled;
		storePaintToolStatus();
	});
	preserve_drawing_button->setMenu(drawing_options_menu);
	auto overlay = mode_descriptor.contains(QLatin1String("overlay_drawing"));
	if (mode_descriptor.contains(QLatin1String("preserve_drawing")) || overlay)
	{
		preserve_drawing = true;
		preserve_drawing_action->setChecked(true);
		if (overlay)
		{
			overlay_drawing_mode = true;
			preserve_drawing_option->setChecked(true);
		}
	}

	auto* fill_action = new QAction(QIcon(QString::fromLatin1(":/images/scribble-fill-shapes.png")),
	                                tr("Filled area"),
	                                toolbar);
	fill_action->setCheckable(true);
	connect(fill_action, &QAction::triggered, this, &PaintOnTemplateTool::setFillAreas);
	toolbar->addActionAtEnd(fill_action, 1, 1);
	if (mode_descriptor.contains(QLatin1String("fill_areas")))
	{
		fill_areas = true;
		fill_action->setChecked(true);
	}	
	
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
	storePaintToolStatus();
}

/// Store paint tool status - i.e. color and drawing mode.
void PaintOnTemplateTool::storePaintToolStatus()
{
	QSettings settings;
	settings.beginGroup(QStringLiteral("PaintOnTemplateTool"));

	auto mode_descriptor = QStringList {};
	if (preserve_drawing && overlay_drawing_mode)
		mode_descriptor.append(QLatin1String("overlay_drawing"));
	else if (preserve_drawing)
		mode_descriptor.append(QLatin1String("preserve_drawing"));
	if (fill_areas)
		mode_descriptor.append(QLatin1String("fill_areas"));
	if (erasing.testFlag(ExplicitErasing))
		mode_descriptor.append(QLatin1String("erasing"));
	settings.setValue(QStringLiteral("drawingMode"), mode_descriptor.join(u','));
	
	settings.setValue(QStringLiteral("selectedColor"), color().name(QColor::HexArgb));	
}

// slot
void PaintOnTemplateTool::setpreserveDrawing(bool enabled)
{
	preserve_drawing = enabled;
	storePaintToolStatus();
}

// slot
void PaintOnTemplateTool::setColor(const QColor& color)
{
	paint_color = color;
	storePaintToolStatus();
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
		
		auto mode = QFlags<Template::ScribbleOption>()
		            .setFlag(Template::FilledAreas, fillAreas())
		            .setFlag(Template::PreserveDrawing, preserveDrawing() && !overlay_drawing_mode)
		            .setFlag(Template::OverlayDrawing, preserveDrawing() && overlay_drawing_mode);
		
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
