/*
 *    Copyright 2012, 2013, 2014 Thomas Schöps
 *    Copyright 2012-2015 Kai Pastor
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

#ifndef _OPENORIENTEERING_ABOUT_DIALOG_H_
#define _OPENORIENTEERING_ABOUT_DIALOG_H_

#include "text_browser_dialog.h"

/**
 * @brief A dialog which shows information about Mapper and its components.
 */
class AboutDialog : public TextBrowserDialog
{
Q_OBJECT
public:
	/**
	 * @brief Construct a new AboutDialog.
	 */
	AboutDialog(QWidget* parent = nullptr);
	
	/**
	 * @brief Returns the basic information about this software.
	 * The return string may contain HTML formatting.
	 */
	static QString about();
	
protected:
	/**
	 * @brief Sets custom HTML content when the URL identifies the first page.
	 */
	virtual void sourceChanged(const QUrl& url) override;
	
	/**
	 * @brief Updates the window title from the current document title.
	 */
	void updateWindowTitle() override;
};

#endif
