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


#include "translation_util.h"

#include <Qt>
#include <QtGlobal>
#include <QDir>
#include <QFileInfo>
#include <QLatin1Char>
#include <QLibraryInfo>
#include <QLocale>
#include <QSettings>
#include <QStringList>
#include <QTranslator>
#include <QVariant>

// IWYU pragma: no_forward_declare QTranslator


namespace
{

QStringList searchPath()
{
	static QStringList search_path;
	if (search_path.isEmpty())
	{
		const auto data_paths = QDir::searchPaths(QLatin1String("data"));
		search_path.reserve(data_paths.size() + 1);
		// Always load embedded translations first if enabled
		search_path.append(QLatin1String(":/translations"));
		for (const auto& path : data_paths)
			search_path.append(path + QLatin1String("/translations"));
	}
	return search_path;
}

QString default_language()
{
	return QLocale::system().name().left(2);
}

} // namespace



QString TranslationUtil::base_name(QString::fromLatin1("qt_"));


TranslationUtil::TranslationUtil()
: TranslationUtil{ QSettings{} }
{
	// nothing else
}


TranslationUtil::TranslationUtil(const QSettings& settings)
: TranslationUtil{ settings.value(QLatin1String("language"), default_language()).toString(),
                   settings.value(QLatin1String("translationFile")).toString() }
{
	// nothing else
}


TranslationUtil::TranslationUtil(const QString& code, QString translation_file)
{
	auto translation_from_file = languageFromFilename(translation_file);
	if (translation_from_file.isValid()
	    && translation_from_file.code == code)
	{
		language = translation_from_file;
	}
	else
	{
		language = languageFromCode(code);
		translation_file = base_name + language.code;
	}
	
	QString translation_name = QLatin1String("qt_") + language.code;
	if (!qt_translator.load(translation_name, QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
		load(qt_translator, translation_name);
	
	load(app_translator, translation_file);
}


std::unique_ptr<QTranslator> TranslationUtil::load(const QString& base_name) const
{
	auto translator = std::make_unique<QTranslator>();
	auto translation_name = QString{base_name + QLatin1Char('_') + language.code};
	auto success = load(*translator, translation_name);
	if (!success)
		translator.reset();
	return translator;
}


bool TranslationUtil::load(QTranslator& translator, const QString& translation_name) const
{
	for (const auto& translation_dir : searchPath())
	{
		if (translator.load(translation_name, translation_dir))
		{
			qDebug("TranslationUtil: Using %s from %s", qPrintable(translation_name), qPrintable(translation_dir));
			return true;
		}
	}
	if (translator.load(translation_name))
	{
		qDebug("TranslationUtil: Using %s from %s", qPrintable(translation_name), "default path");
		return true;
	}
	qDebug("TranslationUtil: Failed to load %s", qPrintable(translation_name));
	return false;
}


// static
void TranslationUtil::setBaseName(const QLatin1String& name)
{
	base_name = name + QLatin1Char('_');
}


// static
TranslationUtil::LanguageList TranslationUtil::availableLanguages()
{
	LanguageList language_map;
	
	auto en = Language { QLatin1String("en"), QLocale::languageToString(QLocale::English) };
	language_map.push_back(en);
	
	const QStringList name_filter = { base_name + QLatin1String("*.qm") };
	for (const auto& translation_dir : searchPath())
	{
		const auto translation_files = QDir(translation_dir).entryList(name_filter, QDir::Files);
		for (const auto& filename : translation_files)
		{
			auto language = languageFromFilename(filename);
			if (language.code != en.code)
				language_map.push_back(language);
		}
	}
	
	return language_map;
}


// static
TranslationUtil::Language TranslationUtil::languageFromFilename(const QString& path)
{
	Language language;
	if (path.endsWith(QLatin1String(".qm"), Qt::CaseInsensitive))
	{
		QString filename(QFileInfo(path).fileName());
		if (filename.startsWith(base_name, Qt::CaseInsensitive))
		{
			auto code = filename.mid(base_name.length(), filename.length() - base_name.length() - 3);
			language = languageFromCode(code);
		}
	}
	return language;
}

// static
TranslationUtil::Language TranslationUtil::languageFromCode(const QString& code)
{
	Language language { code, {} };
	if (code.startsWith(QLatin1String("eo")))
		language.displayName = QLocale::languageToString(QLocale::Esperanto);
	else
		language.displayName = QLocale(code).nativeLanguageName();
	return language;
}

// static
TranslationUtil::Language TranslationUtil::languageFromSettings(const QSettings& settings)
{
	// Only the stored code matters. The stored filename must match the code or is inactive.
	auto language_code = settings.value(QLatin1String("language"), default_language()).toString();
	return languageFromCode(language_code);
}
