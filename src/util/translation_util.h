/*
 *    Copyright 2012-2017 Kai Pastor
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


#ifndef OPENORIENTEERING_TRANSLATION_UTIL_H
#define OPENORIENTEERING_TRANSLATION_UTIL_H

#include <vector>
#include <memory>

#include <QtGlobal>
#include <QString>
#include <QTranslator>

QT_BEGIN_NAMESPACE
class QSettings;
// IWYU pragma: no_forward_declare QTranslator
QT_END_NAMESPACE


/**
 * A simplified interface to searching and loading translations.
 */
class TranslationUtil
{
public:
	/**
	 * A struct for representing the language of a translation.
	 * 
	 * This struct is independent of QLocale limitations for languages
	 * like Esperanto.
	 */
	struct Language
	{
		/** 
		 * The code defining the language. \see QLocale::name()
		 */
		QString code;
		
		/**
		 * The display name of the language.
		 */
		QString displayName;
		
		/**
		 * Returns true when the object holds valid data.
		 */
		bool isValid() const { return !code.isEmpty(); }
	};
	
	
	/**
	 * A collection of languages.
	 */
	using LanguageList = std::vector<Language>;
	
	
	/**
	 * Creates a new translation utility for the file or language from the settings.
	 * 
	 * The base name of the translation files must be set in advance.
	 */
	TranslationUtil();
	
	/**
	 * Creates a new translation utility for the file or language from the given settings.
	 * 
	 * The base name of the translation files must be set in advance.
	 */
	TranslationUtil(const QSettings& settings);
	
	/**
	 * Creates a new translation utility for the given language.
	 * 
	 * The base name of the translation files must be set in advance.
	 */
	TranslationUtil(const QString& code, QString translation_file = {});
	
	/**
	 * Returns the code of the language.
	 */
	QString code() const { return language.code; }
	
	/**
	 * Returns the display name of the language.
	 */
	QString displayName() const { return language.displayName; }
	
	/**
	 * Returns a translator for Qt strings.
	 */
	QTranslator& getQtTranslator() { return qt_translator; }
	
	/**
	 * Returns a translator for application strings.
	 */
	QTranslator& getAppTranslator() { return app_translator; }
	
	
	/**
	 * Sets the common base name of the application's translation files.
	 */
	static void setBaseName(const QLatin1String& name);
	
	/**
	 * Returns a collection of available languages for this application.
	 */
	static LanguageList availableLanguages();
	
	/**
	 * Returns the language for a given translation file.
	 */
	static Language languageFromFilename(const QString& path);
	
	/**
	 * Returns the language for a language name.
	 */
	static Language languageFromCode(const QString& code);
	
	/**
	 * Returns the language for the given settings object.
	 */
	static Language languageFromSettings(const QSettings& settings);
	
	
	/**
	 * Tries to create a QTranslator for the named translation.
	 * 
	 * If the named translation cannot be loaded from the search path,
	 * returns an unset unique_prt.
	 */
	std::unique_ptr<QTranslator> load(const QString& translation_name) const;
	
protected:
	/**
	 * Finds a named translation from the search path and loads it
	 * to the translator.
	 */
	bool load(QTranslator& translator, const QString& translation_name) const;
	
	
	static QString base_name;
	
	Language language;
	
	QTranslator qt_translator;
	
	QTranslator app_translator;
};


inline
bool operator<(const TranslationUtil::Language& first, const TranslationUtil::Language& second) {
	return first.code < second.code;
}


#endif
