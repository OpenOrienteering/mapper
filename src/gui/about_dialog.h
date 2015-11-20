/*
 *    Copyright 2012, 2013, 2014 Kai Pastor, Thomas Schöps
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

#include <QDialog>
#include <QTextBrowser>
#include <QUrl>

/**
 * @brief A dialog which shows information about Mapper and its components.
 */
class AboutDialog : public QDialog
{
Q_OBJECT
public:
	/**
	 * @brief Construct a new AboutDialog.
	 */
	AboutDialog(QWidget* parent = NULL);
	
	/**
	 * @brief Returns the basic information about this software.
	 * The return string may contain HTML formatting.
	 */
	static QString about();
	
protected slots:
	/**
	 * @brief Sets custom HTML content when the URL identifies the first page.
	 */
	void sourceChanged(QUrl url);
	
	/**
	 * @brief Updates the window title from the current document title.
	 */
	void updateWindowTitle();
	
	/**
	 * @brief Shows a tooltip showing the link if is an external document.
	 */
	void highlighted(QString link);
	
protected:
	/**
	 * @brief Returns a size hint based on the text browser's content.
	 */
	virtual QSize sizeHint() const;
	
	/**
	 * @brief The TextBrowser is the main widget of the AboutDialog.
	 */
	QTextBrowser* text_browser;
};

#endif
