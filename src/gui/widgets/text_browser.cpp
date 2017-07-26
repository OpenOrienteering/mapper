/*
 *    Copyright 2017 Kai Pastor
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


#include "text_browser.h"


TextBrowser::TextBrowser(QWidget* parent)
: QTextBrowser{ parent }
{
	// nothing else
}


TextBrowser::~TextBrowser() = default;



QVariant TextBrowser::loadResource(int type, const QUrl& name)
{
	auto result = QTextBrowser::loadResource(type, name);
	if (result.type() == QVariant::ByteArray
	    && type == QTextDocument::HtmlResource
	    && !name.fileName().contains(QLatin1String(".htm"), Qt::CaseInsensitive))
	{
		auto html = QString {
		  QLatin1String("<html><head><title>") +
		    name.fileName().toHtmlEscaped() +
		  QLatin1String("</title></head><body><pre>") +
		    QString::fromUtf8(result.toByteArray()).toHtmlEscaped() +
		  QLatin1String("</pre></body></html>")
		};
		result = QVariant{ html };
	}
	return result;
}
