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

#include <memory>

#include <Qt>
#include <QtMath>
#include <QAction>
#include <QColor>
#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QFontMetrics>
#include <QIcon>
#include <QImage>
#include <QLatin1Char>
#include <QLatin1String>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPainter>
#include <QPointF>
#include <QSpacerItem>
#include <QVariant>
#include <QVBoxLayout>

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
};


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
	                           QCoreApplication::translate("MapEditorController", "Paint on template"),
	                           this);
	paint_action->setMenuRole(QAction::NoRole);
	paint_action->setCheckable(true);
	paint_action->setWhatsThis(Util::makeWhatThis("toolbars.html#draw_on_template"));
	connect(paint_action, &QAction::triggered, this, &PaintOnTemplateFeature::paintClicked);
	
	// This action used to be listed as "Paint on template settings" in the touch UI.
	select_action = new QAction(QIcon(QString::fromLatin1(":/images/paint-on-template-settings.png")),
	                            QCoreApplication::translate("MapEditorController", "Select template..."),
	                            this);
	select_action->setMenuRole(QAction::NoRole);
	select_action->setWhatsThis(Util::makeWhatThis("toolbars.html#draw_on_template"));
	connect(select_action, &QAction::triggered, this, &PaintOnTemplateFeature::selectTemplateClicked);
}

PaintOnTemplateFeature::~PaintOnTemplateFeature() = default;


void PaintOnTemplateFeature::setEnabled(bool enabled)
{
	paintAction()->setEnabled(enabled);
	selectAction()->setEnabled(enabled);
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
		finishPainting();
	else if (last_template)
		startPainting(last_template);
	else if (auto* temp = selectTemplate())
		startPainting(temp);
	else
		paint_action->setChecked(false);
}

void PaintOnTemplateFeature::selectTemplateClicked()
{
	if (auto* temp = selectTemplate())
		startPainting(temp);
}

Template* PaintOnTemplateFeature::selectTemplate() const
{
	Template* selected = nullptr;
	QDialog dialog(controller.getWindow(), Qt::WindowSystemMenuHint | Qt::WindowTitleHint);
	initTemplateDialog(dialog, selected);
	dialog.exec();
	return selected;
}

void PaintOnTemplateFeature::initTemplateDialog(QDialog& dialog, Template*& selected_template) const
{
#if defined(ANDROID)
	dialog.setWindowState((dialog.windowState() & ~(Qt::WindowMinimized | Qt::WindowFullScreen))
	                      | Qt::WindowMaximized);
#endif
	dialog.setWindowTitle(QCoreApplication::translate("OpenOrienteering::PaintOnTemplateSelectDialog", "Select template to draw onto"));
	dialog.setWindowModality(Qt::WindowModal);
	
	auto* template_list = new QListWidget();
	initTemplateListWidget(*template_list);
	
	auto* button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal);
	
	auto layout = new QVBoxLayout();
	layout->addWidget(template_list);
	layout->addItem(Util::SpacerItem::create(&dialog));
	layout->addWidget(button_box);
	dialog.setLayout(layout);
	
	connect(button_box, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
	connect(button_box, &QDialogButtonBox::accepted, this, [this, template_list, &selected_template, &dialog]() {
		if (auto* current = template_list->currentItem())
		{
			selected_template = reinterpret_cast<Template*>(current->data(Qt::UserRole).value<void*>());
			if (!selected_template)
				selected_template = addNewTemplate();
			if (selected_template)
				dialog.accept();
		}
	});
}

void PaintOnTemplateFeature::initTemplateListWidget(QListWidget& list_widget) const
{
	/// \todo Review source string (no ellipsis when no dialog)
	auto* item = new QListWidgetItem(QCoreApplication::translate("OpenOrienteering::TemplateListWidget", "Add template..."));
	item->setData(Qt::UserRole, qVariantFromValue<void*>(nullptr));
	list_widget.addItem(item);
	
	auto& map = *controller.getMap();
	auto current_row = 0;
	for (int i = map.getNumTemplates() - 1; i >= 0; --i)
	{
		auto& temp = *map.getTemplate(i);
		if (!temp.canBeDrawnOnto() || temp.getTemplateState() != Template::Loaded)
			continue;
		
		if (&temp == last_template)
			current_row = list_widget.count();
		
		auto* item = new QListWidgetItem(temp.getTemplateFilename());
		item->setData(Qt::UserRole, qVariantFromValue<void*>(&temp));
		list_widget.addItem(item);
	}
	list_widget.setCurrentRow(current_row);
}

Template* PaintOnTemplateFeature::addNewTemplate() const
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
	if (QFileInfo::exists(image_file_path))
	{
		showMessage(window, tr("Template file exists: '%1'").arg(filename));
		return nullptr;
	}
	
	auto image = makeImage(filename);
	if (!image.save(image_file_path))
	{
		showMessage(window,
		            OpenOrienteering::MapEditorController::tr("Cannot save file\n%1:\n%2")
		            .arg(filename, QString{}));
		return nullptr;
	}
	
	auto* temp = new TemplateImage{image_file_path, &map};
	temp->setTemplatePosition(georef.toMapCoords(projected_top_left) + offset);
	temp->setTemplateScaleX(1.0/pixel_per_mm);
	temp->setTemplateScaleY(1.0/pixel_per_mm);
	temp->setTemplateShear(0);
	temp->setTemplateRotation(0);
	temp->loadTemplateFile();
	
	map.addTemplate(-1, std::unique_ptr<Template>{temp});
	
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
}

void PaintOnTemplateFeature::finishPainting()
{
	if (auto* tool = qobject_cast<PaintOnTemplateTool*>(controller.getTool()))
		tool->deactivate();
}


}  // namespace OpenOrienteering
