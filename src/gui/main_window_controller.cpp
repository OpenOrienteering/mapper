/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2014 Thomas Schöps, Kai Pastor
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


#include "main_window_controller.h"

#include <QtGlobal>

#include "fileformats/file_format.h"
#include "fileformats/file_format_registry.h"
#include "gui/map/map_editor.h"


namespace OpenOrienteering {

MainWindowController::~MainWindowController() = default;


bool MainWindowController::save(const QString& /*path*/, const FileFormat* /*format*/)
{
	return false;
}

bool MainWindowController::exportTo(const QString& path, const FileFormat* format)
{
	Q_UNUSED(path);
	Q_UNUSED(format);
	return false;
}

bool MainWindowController::load(const QString& path, QWidget* dialog_parent)
{
	Q_UNUSED(path);
	Q_UNUSED(dialog_parent);
	return false;
}

void MainWindowController::detach()
{
	// nothing
}

bool MainWindowController::isEditingInProgress() const
{
	return false;
}

bool MainWindowController::keyPressEventFilter(QKeyEvent* event)
{
	Q_UNUSED(event);
	return false;
}

bool MainWindowController::keyReleaseEventFilter(QKeyEvent* event)
{
	Q_UNUSED(event);
	return false;
}

MainWindowController* MainWindowController::controllerForFile(const QString& filename)
{
	const FileFormat* format = FileFormats.findFormatForFilename(filename);
	if (format && format->supportsImport()) 
		return new MapEditorController(MapEditorController::MapEditor);
	
	return nullptr;
}


}  // namespace OpenOrienteering
