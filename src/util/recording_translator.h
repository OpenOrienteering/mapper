/*
 *    Copyright 2013 Kai Pastor
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


#ifndef _UTIL_RECORDING_TRANSLATOR_H_
#define _UTIL_RECORDING_TRANSLATOR_H_

#include <QTranslator>


/** A QTranslator variation that doesn't actually translate,
 *  but instead dumps (TODO: records) the strings to be translated.
 * 
 *  Note: This is for Qt5 only.
 */
class RecordingTranslator : public QTranslator
{
Q_OBJECT
public:
	/** Constructs a new translator. */
	explicit RecordingTranslator(QObject* parent = 0);
	
	/** Destructs the translator. */
	virtual ~RecordingTranslator();
	
	/** Returns false. */
	virtual bool isEmpty() const;
	
	/** Dumps the context, sourceText, disambiguation and n using qDebug().
	 *  Return a null string. 
	 *
	 *  Note: This is the Qt5 signature of translate(). */
	virtual QString	translate(const char* context, const char* sourceText, const char* disambiguation = 0, int n = -1) const;
};

#endif
