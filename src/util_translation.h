/*
 *    Copyright 2012, 2013 Kai Pastor
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


#ifndef OPENORIENTEERING_UTIL_TRANSLATION_H
#define OPENORIENTEERING_UTIL_TRANSLATION_H

#include <QLocale>
#include <QMap>
#include <QStringList>
#include <QTranslator>


/**
 * A collection of language names and the corresponding IDs.
 */
typedef QMap<QString, QLocale::Language> LanguageCollection;


/**
 * A simplified interface to searching and loading translations.
 */
class TranslationUtil
{
public:
	/**
	 * Creates a new translation utility for the given language.
	 * The base name of the translation files should be set in advance.
	 */
	TranslationUtil(QLocale::Language lang, QString translation_file = {});
	
	/**
	 * Returns the locale.
	 */
	const QLocale& getLocale() const { return locale; }
	
	/**
	 * Returns a translation for Qt.
	 */
	QTranslator& getQtTranslator() { return qt_translator; }
	
	/**
	 * Returns a translation for the application.
	 */
	QTranslator& getAppTranslator() { return app_translator; }
	
	/**
	 * Returns a collection of available languages for this application.
	 */
	static LanguageCollection getAvailableLanguages();
	
	/**
	 * Sets the common base name of the application's translation files.
	 */
	static void setBaseName(const QString& name);
	
	/**
	 * Returns the locale name for a given translation file,
	 * or returns an empty string if the file is not a valid translation.
	 * 
	 * In order to be valid, the file must exist, and the file name
	 * (without directory) must be composed of the base name, an underscore,
	 * the locale name and the suffix ".qm".
	 */
	static QString localeNameForFile(const QString& filename);
	
protected:
	/**
	 * Initializes the search path.
	 */
	static void init_search_path();
	
	/**
	 * Finds a named translation from the search path and loads it
	 * to the translator.
	 */
	bool load(QTranslator& translator, QString translation_name);
	
	static QString base_name;
	
	static QStringList search_path;
	
	QLocale locale;
	
	QTranslator qt_translator;
	
	QTranslator app_translator;
};

#endif
