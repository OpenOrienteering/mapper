/*
 *    Copyright 2020 Kai Pastor
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

#include <Qt>
#include <QAction>
#include <QCoreApplication>
#include <QDialog>
#include <QIcon>

#include "core/map.h"
#include "core/map_view.h"
#include "gui/util_gui.h"
#include "gui/map/map_editor.h"
#include "gui/map/map_widget.h"
#include "templates/paint_on_template_tool.h"
#include "templates/template.h"
#include "tools/tool.h"


namespace OpenOrienteering {

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
	auto* main_view = controller.getMainWidget()->getMapView();
	PaintOnTemplateSelectDialog paint_dialog(controller.getMap(), main_view, last_template, controller.getWindow());
	paint_dialog.setWindowModality(Qt::WindowModal);
	if (paint_dialog.exec() == QDialog::Accepted)
		return paint_dialog.getSelectedTemplate();
	else
		return nullptr;
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
