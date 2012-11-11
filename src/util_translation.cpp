/*
 *    Copyright 2012 Kai Pastor
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


#include "util_translation.h"

#include <mapper_config.h>

#include <QCoreApplication>
#include <QDir>
#include <QLibraryInfo>
#include <QTranslator>

#include "mapper_resource.h"


QStringList TranslationUtil::search_path;


TranslationUtil::TranslationUtil(QLocale::Language lang)
: locale(lang)
{
	if (search_path.size() == 0)
		init_search_path();
	
	QString locale_name = locale.name();
	
	QString translation_name = "qt_" + locale_name;
	if (!qt_translator.load(translation_name, QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
		load(qt_translator, translation_name);
	
	translation_name = QCoreApplication::applicationName() + "_" + locale_name;
	load(app_translator, translation_name);
}


bool TranslationUtil::load(QTranslator& translator, QString translation_name)
{
	Q_FOREACH(QString translation_dir, search_path)
	{
		if (translator.load(translation_name, translation_dir))
			return true;
	}
	return false;
}


LanguageCollection TranslationUtil::getAvailableLanguages()
{
	if (search_path.size() == 0)
		init_search_path();
	
	LanguageCollection language_map;
	language_map.insert(QLocale::languageToString(QLocale::English), QLocale::English);
	
	QString app_name = QCoreApplication::applicationName();
	QStringList name_filter;
	name_filter << (app_name + "_*.qm");
	
	Q_FOREACH(QString translation_dir, search_path)
	{
		QDir dir(translation_dir);
		Q_FOREACH (QString name, dir.entryList(name_filter, QDir::Files))
		{
			name.remove(0, app_name.length()+1);
			name.remove(name.length()-3, 3);
#if (QT_VERSION >= QT_VERSION_CHECK(4, 8, 0))
			QString language_name = QLocale(name).nativeLanguageName();
#else
			QString language_name = QLocale::languageToString(QLocale(name).language());
#endif
			language_map.insert(language_name, QLocale(name).language());
		}
	}
	
	return language_map;
}

void TranslationUtil::init_search_path()
{
	search_path = MapperResource::getLocations(MapperResource::TRANSLATION);
}
