/*
 *    Copyright 2016 Kai Pastor
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

/*
 * This file is not compiled but parsed for strings to be translated.
 *
 * Strings in this file are to be used in future releases, as soon as major languages
 * are covered by translations.
 */

auto future_translations =
{
	/// \todo Fix uses of plural "object(s)" and "symbol(s)"
	//: To replace existing translation which uses "object(s)".
	MapEditorController::tr("Rotate objects"),
	
	//: To replace existing translation which uses "object(s)".
	MapEditorController::tr("Scale objects"),
	
	//: Past tense. Displayed when an Edit > Cut operator is completed.
	MapEditorController::tr("Cut %n object(s)", nullptr, 1),
	
	//: To replace existing translation which doesn't use plural correctly
	MapEditorController::tr("Copied %n object(s)", nullptr, 1),
	
	//: To replace existing translation which doesn't use plural correctly
	MapEditorController::tr("Pasted %n object(s)", nullptr, 1),
	
	//: To replace existing translation which uses "object(s)".
	MapEditorController::tr("Deletes the selected objects."),
	
	//: To replace existing translation which uses "object(s)".
	MapEditorController::tr("Duplicate the selected objects."),
	
	//: To replace existing translation which uses "object(s)".
	MapEditorController::tr("Rotate the selected objects."),
	
	//: To replace existing translation which uses "object(s)".
	MapEditorController::tr("Scale the selected objects."),
	
	//: To replace existing translation which uses "object(s)".
	MapEditorController::tr("Cut the selected objects into smaller parts."),
	
	//: To replace existing translation which uses "object(s)".
	MapEditorController::tr("Switches the symbol of the selected objects to the selected symbol."),
	
	//: To replace existing translation which uses "line(s)" and "area(s)".
	MapEditorController::tr("Fill the selected lines or create a border for the selected areas."),
	
	//: To replace existing translation which doesn't use plural correctly
	MapEditorController::tr("Duplicated %n object(s)", nullptr, 1),
	
	//: To replace existing translation which uses "symbol(s)".
	MapEditorController::tr("No objects were selected because there are no objects with the selected symbols."),
	
	//: To replace existing translation which uses "object(s)".
	SymbolRenderWidget::tr("Switch symbol of selected objects"),
	
	//: To replace existing translation which uses "object(s)".
	SymbolRenderWidget::tr("Fill / Create border for selected objects"),
	
	//: To replace existing translation which uses "symbol(s)".
	SymbolRenderWidget::tr("Scale symbols"),
};
