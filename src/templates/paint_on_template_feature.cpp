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


#include "paint_on_template_feature.h"

#include <algorithm>
#include <memory>
#include <utility>

#include <Qt>
#include <QtMath>
#include <QAction>
#include <QActionGroup>
#include <QColor>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFontMetrics>
#include <QIcon>
#include <QImage>
#include <QLatin1Char>
#include <QLatin1String>
#include <QList>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QStyle>
#include <QToolButton>
#include <QTransform>
#include <QWidget>

#include "core/georeferencing.h"
#include "core/map.h"
#include "core/map_coord.h"
#include "core/map_view.h"
#include "gui/util_gui.h"
#include "gui/main_window.h"
#include "gui/map/map_editor.h"
#include "gui/map/map_widget.h"
#include "templates/paint_on_template_tool.h"
#include "templates/template.h"
#include "templates/template_image.h"
#include "tools/tool.h"


namespace OpenOrienteering {

namespace {

// 10 pixel per mm, 100 mm per image -> 1000 pixel
// When these parameters are changed, alignmentBase() needs to be reviewed.
constexpr auto pixel_per_mm = 10;
constexpr auto size_mm      = 100;  // multiple of 2


void showMessage (MainWindow* window, const QString &message) {
#ifdef Q_OS_ANDROID
	window->showStatusBarMessage(message, 2000);
#else
	QMessageBox::warning(window,
	                     OpenOrienteering::MapEditorController::tr("Warning"),
	                     message,
	                     QMessageBox::Ok,
	                     QMessageBox::Ok);
#endif
}


/**
 * Constructs a functor which creates icons representing template position.
 * 
 * The functor captures the provided viewport, and creates icons which
 * represent the visible template area as a gray rectangle relative to
 * a white rectangle with a black outline representing the viewport.
 */
auto makeIconBuilder(int width, const QRectF& view_rect)
{
	auto const scale = qreal(width) / std::max(view_rect.width(), view_rect.height());
	QTransform transform;
	transform.translate(width / 2, width / 2);
	transform.scale(scale, scale);
	transform.translate(-view_rect.center().x(), -view_rect.center().y());
	
	// Draw viewport (background) aligned to integer pixels.
	auto const view_rect_px = transform.mapRect(view_rect)
	                          .toAlignedRect()
	                          .adjusted(0, 0, -1, -1)
	                          .intersected({0, 0, width, width});
	
	QPixmap pixmap(width, width);
	pixmap.fill(Qt::transparent);
	
	QPainter painter(&pixmap);
	painter.setPen(Qt::black);
	painter.setBrush(Qt::white);
	painter.drawRect(view_rect_px);
	painter.end();
	
	return [pixmap, transform](const QRectF& template_rect) -> QIcon {
		auto clone = pixmap;
		QPainter p(&clone);
		p.setCompositionMode(QPainter::CompositionMode_SourceAtop);
		p.setWorldTransform(transform);
		p.setPen(Qt::NoPen);
		p.setBrush(Qt::darkGray);
		p.drawRect(template_rect);
		p.end();
		return QIcon{clone};
	};
}


}  // namespace


// static
int PaintOnTemplateFeature::alignmentBase(qreal scale)
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

// static
qint64 PaintOnTemplateFeature::roundToMultiple(qreal x, int base)
{
	return qRound(x / base) * base;
}

// static
QPointF PaintOnTemplateFeature::roundToMultiple(const QPointF& point, int base)
{
	return { qreal(qRound(point.x() / base) * base), qreal(qRound(point.y() / base) * base) };
}


PaintOnTemplateFeature::PaintOnTemplateFeature(MapEditorController& controller)
: controller(controller)
{
	connect(controller.getMap(), &Map::templateAboutToBeDeleted, this, &PaintOnTemplateFeature::templateAboutToBeDeleted);
	
	paint_action = new QAction(QIcon(QString::fromLatin1(":/images/pencil.png")),
	                           QCoreApplication::translate("OpenOrienteering::MapEditorController", "Paint on template"),
	                           this);
	paint_action->setMenuRole(QAction::NoRole);
	paint_action->setCheckable(true);
	paint_action->setMenu(makeTemplateMenu(paint_action, controller.getMainWidget()));
	paint_action->setWhatsThis(Util::makeWhatThis("toolbars.html#draw_on_template"));
	connect(paint_action, &QAction::triggered, this, &PaintOnTemplateFeature::paintClicked);
}

PaintOnTemplateFeature::~PaintOnTemplateFeature() = default;


void PaintOnTemplateFeature::setEnabled(bool enabled)
{
	paintAction()->setEnabled(enabled);
}


// slot
void PaintOnTemplateFeature::templateAboutToBeDeleted(int /*pos*/, Template* temp)
{
	if (temp == last_template)
		last_template = nullptr;
}

void PaintOnTemplateFeature::paintClicked(bool checked)
{
	if (!checked)
	{
		finishPainting();
	}
	else if (last_template && last_template->boundingRect().intersects(viewedRect()))
	{
		startPainting(last_template);
	}
	else
	{
		paint_action->setChecked(false);
		if (auto* button = buttonForPaintAction())
				button->showMenu();
	}
}


QMenu* PaintOnTemplateFeature::makeTemplateMenu(QAction* action, QWidget* parent)
{
	auto* menu = new QMenu(parent);
	auto* action_group = new QActionGroup(menu);
	// Cannot use QMenu::aboutToShow because it causes menu mispositioning near
	// lower screen border when triggered from QToolButton.
	if (action)
		connect(action, &QAction::hovered, this, [this, menu, action_group]() {
			refreshTemplateMenu(menu, action_group);
		});
	else
		refreshTemplateMenu(menu, action_group);
	return menu;
}

void PaintOnTemplateFeature::refreshTemplateMenu(QMenu* menu, QActionGroup* action_group)
{
	menu->clear();
	Q_ASSERT(action_group->actions().isEmpty());
	
	/// \todo Review source string (no ellipsis when no dialog)
	auto* action_new = menu->addAction(QIcon(QStringLiteral(":/images/plus.png")),
	                                   QCoreApplication::translate("OpenOrienteering::TemplateListWidget", "Add template..."));
	connect(action_new, &QAction::triggered, this, [this]() {
		if (auto* selected_template = setupTemplate())
			startPainting(selected_template);
	});
	menu->addSeparator();
	
	auto& map = *controller.getMap();
	auto const viewed_rect = viewedRect();
	auto const icon_width = menu->style()->pixelMetric(QStyle::PM_SmallIconSize);
	auto const icon_builder = makeIconBuilder(icon_width, viewed_rect);
	for (int i = map.getNumTemplates() - 1; i >= 0; --i)
	{
		auto& temp = *map.getTemplate(i);
		if (temp.getTemplateState() == Template::Invalid
		    || !temp.canBeDrawnOnto())
			continue;
		
		auto const bounding_rect = temp.boundingRect();
		if (!bounding_rect.intersects(viewed_rect))
			continue;
		
		auto* action = menu->addAction(icon_builder(bounding_rect), temp.getTemplateFilename());
		action->setCheckable(true);
		action->setActionGroup(action_group);
		if (&temp == last_template)
			action->setChecked(true);
		connect(action, &QAction::triggered, this, [this, &temp]() {
			startPainting(&temp);
		});
	}
}


Template* PaintOnTemplateFeature::setupTemplate() const
{
	auto* window = controller.getWindow();
	if (!window || window->currentPath().isEmpty())
		return nullptr;
	
	// Determine aligned top-left position
	auto const center = controller.getMainWidget()->getMapView()->center();
	auto& map = *controller.getMap();
	auto const& georef = map.getGeoreferencing();
	auto const base = alignmentBase(georef.getScaleDenominator());
	auto const offset = MapCoord{size_mm/2, size_mm/2}; // from top-left to center
	auto const projected_top_left = roundToMultiple(georef.toProjectedCoords(center - offset), base);
	
	// Find or create a template for the track with a specific name
	const QString filename = QLatin1String("Draft @ ")
	                          + QString::number(qRound64(projected_top_left.x()))
	                          + QLatin1Char(',')
			                  + QString::number(qRound64(projected_top_left.y()))
	                          + QLatin1String(".png");
	QString image_file_path = QFileInfo(window->currentPath()).absoluteDir().canonicalPath()
	                          + QLatin1Char('/')
	                          + filename;
	
	// When needing a new file, we try write it and read it once (for confidence
	// in permissions), but remove it before exiting this function.
	bool remove_file = false;
	
	Template* temp = nullptr;
	if (QFileInfo::exists(image_file_path))
	{
		showMessage(window, tr("Template file exists: '%1'").arg(filename));
		
		// Our names are unique. So we can safely assume that existing files and
		// templates are meant to be reused.
		temp = [&map, image_file_path]() -> Template* {
			for (int i = map.getNumTemplates() - 1; i >= 0; --i)
			{
				auto* temp = map.getTemplate(i);
				if (temp->getTemplatePath() == image_file_path && temp->canBeDrawnOnto())
					return temp;
			}
			return nullptr;
		}();
		
		if (temp && temp->getTemplateState() != Template::Loaded)
		{
			temp->loadTemplateFile();
		}
		
		if (temp && temp->getTemplateState() != Template::Loaded)
		{
			showMessage(window,
			            ::OpenOrienteering::MainWindow::tr("Cannot open file:\n%1\n\n%2")
			            .arg(image_file_path, temp->errorString()));
			return nullptr;
		}
	}
	else
	{
		auto image = makeImage(filename);
		if (!image.save(image_file_path))
		{
			showMessage(window,
			            OpenOrienteering::MapEditorController::tr("Cannot save file\n%1:\n%2")
			            .arg(filename, QString{}));
			return nullptr;
		}
		remove_file = true;
	}
	
	if (!temp)
	{
		auto template_image = std::make_unique<TemplateImage>(image_file_path, &map);
		template_image->setTemplatePosition(georef.toMapCoords(projected_top_left) + offset);
		template_image->setTemplateScaleX(1.0/pixel_per_mm);
		template_image->setTemplateScaleY(1.0/pixel_per_mm);
		template_image->setTemplateShear(0);
		template_image->setTemplateRotation(0);
		template_image->loadTemplateFile();
		
		if (template_image->getTemplateState() != Template::Loaded)
		{
			showMessage(window,
			            ::OpenOrienteering::MainWindow::tr("Cannot open file:\n%1\n\n%2")
			            .arg(image_file_path, template_image->errorString()));
			return nullptr;
		}
		
		temp = template_image.get();
		map.addTemplate(-1, std::move(template_image));
	}
	
	if (remove_file)
	{
		QFile::remove(image_file_path);   // Created for check/initialization.
		temp->setHasUnsavedChanges(true); // Save again when the map is saved.
	}
	
	return temp;
}

// static
QImage PaintOnTemplateFeature::makeImage(const QString& label)
{
	constexpr auto size_pixel = size_mm * pixel_per_mm;
	auto image = QImage(size_pixel, size_pixel, QImage::Format_ARGB32);
	image.fill(QColor(Qt::transparent));
	QPainter p(&image);
	p.setPen(QColor(Qt::red));
	p.drawRect(0, 0, size_pixel-1, size_pixel-1);
	p.drawText(pixel_per_mm/2, pixel_per_mm/2 + QFontMetrics(p.font()).ascent(), label);
	return image;
}


void PaintOnTemplateFeature::startPainting(Template* temp)
{
	last_template = temp;
	
	auto* tool = qobject_cast<PaintOnTemplateTool*>(controller.getTool());
	if (!tool)
	{
		tool = new PaintOnTemplateTool(&controller, paintAction());
		controller.setTool(tool);
	}
	
	controller.hideAllTemplates(false);
	auto* view = controller.getMainWidget()->getMapView();
	auto vis = view->getTemplateVisibility(temp);
	vis.visible = true;
	view->setTemplateVisibility(temp, vis);
	
	tool->setTemplate(temp);
	
	paintAction()->setChecked(true);
}

void PaintOnTemplateFeature::finishPainting()
{
	if (auto* tool = qobject_cast<PaintOnTemplateTool*>(controller.getTool()))
		tool->deactivate();
}


QRectF PaintOnTemplateFeature::viewedRect() const
{
	auto const& map_widget = *controller.getMainWidget();
	auto const& view = *map_widget.getMapView();
	return view.calculateViewedRect(map_widget.viewportToView(map_widget.rect()));
}


QToolButton* PaintOnTemplateFeature::buttonForPaintAction()
{
	auto const widgets = paintAction()->associatedWidgets();
	for (auto* w : widgets)
	{
		if (auto* button = qobject_cast<QToolButton*>(w))
			return button;
	}
	return nullptr;
}


}  // namespace OpenOrienteering
