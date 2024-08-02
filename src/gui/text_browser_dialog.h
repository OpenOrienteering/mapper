/*
 *    Copyright 2012, 2013, 2014 Thomas Schöps
 *    Copyright 2012-2015, 2017 Kai Pastor
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

#ifndef OPENORIENTEERING_TEXT_BROWSER_DIALOG_H
#define OPENORIENTEERING_TEXT_BROWSER_DIALOG_H

#include <QDialog>
#include <QObject>
#include <QSize>

class QWidget;

class QString;
class QTextBrowser;
class QUrl;


namespace OpenOrienteering {

/**
 * @brief A dialog for basic browsing of HTML pages.
 */
class TextBrowserDialog : public QDialog
{
Q_OBJECT
protected:
	explicit TextBrowserDialog(QWidget* parent = nullptr);
	
public:
	/**
	 * @brief Construct a new dialog and loads the text from the initial_url.
	 */
	TextBrowserDialog(const QUrl& initial_url, QWidget* parent = nullptr);
	
	/**
	 * @brief Construct a new dialog and sets the given text.
	 */
	TextBrowserDialog(const QString& text, QWidget* parent = nullptr);
	
	~TextBrowserDialog() override;
	
protected slots:
	/**
	 * @brief Sets custom HTML content when the URL identifies the first page.
	 */
	virtual void sourceChanged(const QUrl& url);
	
	/**
	 * @brief Updates the window title from the current document title.
	 */
	virtual void updateWindowTitle();
	
	/**
	 * @brief Displays a tooltip showing the link if it's an external document.
	 */
	void highlighted(const QString& link);
	
protected:
	/**
	 * @brief Returns a size hint based on the text browser's content.
	 */
	QSize sizeHint() const override;
	
	/**
	 * @brief The TextBrowser is the main widget of this dialog.
	 */
	QTextBrowser* const text_browser;
};


}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_TEXT_BROWSER_DIALOG_H
