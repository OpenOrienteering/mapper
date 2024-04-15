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

#ifndef OPENORIENTEERING_PRINTER_PROPERTIES_H
#define OPENORIENTEERING_PRINTER_PROPERTIES_H

#include <memory>

class QPrinter;
class QWidget;


/**
 * Provides platform-dependent printer properties handling.
 * 
 * At moment, a specific implementation exists for Win32, dealing with DEVMODE
 * and DocumentProperties.
 */
namespace PlatformPrinterProperties
{
	/**
	 * Saves the printer's platform-dependent properties.
	 * 
	 * The buffer is reset to point to the saved data.
	 * 
	 * The default implementation does nothing.
	 */
	void save(const QPrinter* printer, std::shared_ptr<void>& buffer);
	
	/**
	 * Applies platform-dependent properties to the printer, if possible.
	 * 
	 * The default implementation does nothing.
	 */
	void restore(QPrinter* printer, const std::shared_ptr<void>& buffer);
	
	/**
	 * Returns true if the platform supports execDialog().
	 * 
	 * The default implementation returns false.
	 */
	bool dialogSupported();
	
	/**
	 * Shows a modal properties dialog for the given printer.
	 * 
	 * The printer parameter must not be nullptr. The buffer may return data
	 * which has to live as long as the printer object.
	 * 
	 * Returns QDialog::Accepted or QDialog::Rejected, depending on user action.
	 * 
	 * The default implementation returns QDialog::Rejected.
	 */
	int execDialog(QPrinter* printer, std::shared_ptr<void>& buffer, QWidget* parent = nullptr);
}

#endif // OPENORIENTEERING_PRINTER_PROPERTIES_H
