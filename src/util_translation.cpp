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


#include "util_translation.h"

#include <mapper_config.h>

#include <QCoreApplication>
#include <QDir>
#include <QLibraryInfo>
#include <QTranslator>

#include "mapper_resource.h"

QString TranslationUtil::base_name("qt_"); 

QStringList TranslationUtil::search_path;


TranslationUtil::TranslationUtil(QLocale::Language lang, QString translation_file)
: locale(lang)
{
	if (search_path.size() == 0)
		init_search_path();
	
	QString locale_name = locale.name();
	
	QString translation_name = "qt_" + locale_name;
	if (!qt_translator.load(translation_name, QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
		load(qt_translator, translation_name);
	
	QString file_locale = localeNameForFile(translation_file);
	if (!file_locale.isEmpty() && QLocale(file_locale).language() == lang)
	{
		load(app_translator, translation_file);
	}
	else
	{
		translation_name = base_name + locale_name;
		load(app_translator, translation_name);
	}
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
	
	QStringList name_filter;
	name_filter << (base_name + "*.qm");
	
	Q_FOREACH(QString translation_dir, search_path)
	{
		QDir dir(translation_dir);
		Q_FOREACH (QString name, dir.entryList(name_filter, QDir::Files))
		{
			name.remove(0, base_name.length());
			name.remove(name.length()-3, 3);
			QString language_name = QLocale(name).nativeLanguageName();
			language_map.insert(language_name, QLocale(name).language());
		}
	}
	
	return language_map;
}

QString TranslationUtil::localeNameForFile(const QString& filename)
{
	if (!filename.endsWith(".qm", Qt::CaseInsensitive))
		return QString();
	
	QFileInfo info(filename);
	if (!info.isFile())
		return QString();
	
	QString name(info.fileName());
	if (!name.startsWith(base_name, Qt::CaseInsensitive))
		return QString();
	
	name.remove(0, base_name.length());
	name.remove(name.length()-3, 3);
	return name;
}

void TranslationUtil::setBaseName(const QString& name)
{
	base_name = name + "_";
}

void TranslationUtil::init_search_path()
{
	search_path = MapperResource::getLocations(MapperResource::TRANSLATION);
}
