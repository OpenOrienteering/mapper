/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2017-2020 Kai Pastor
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


#include "symbol_replacement.h"

#include <type_traits>
#include <utility>
#include <vector>

#include <QDialog>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QLatin1Char>
#include <QLatin1String>
#include <QMessageBox>
#include <QString>
#include <QTextStream>

#include "core/map.h"
#include "core/objects/symbol_rule_set.h"
#include "core/symbols/symbol.h"
#include "fileformats/file_format.h"
#include "fileformats/file_import_export.h"
#include "gui/file_dialog.h"
#include "gui/main_window.h"
#include "gui/symbols/symbol_replacement_dialog.h"


namespace OpenOrienteering {


SymbolReplacement::SymbolReplacement(Map& object_map) noexcept
: SymbolReplacement {object_map, object_map}
{}


SymbolReplacement::SymbolReplacement(Map& object_map, const Map& symbol_set) noexcept
: object_map {object_map}
, symbol_set {symbol_set}
{}


QString SymbolReplacement::crtFileById() const
{
	auto const source_id = object_map.symbolSetId();
	auto const target_id = symbol_set.symbolSetId();
	QString filepath = QLatin1String("data:/symbol sets/")
	                   + source_id + QLatin1Char('-')
	                   + target_id + QLatin1String(".crt");
#ifdef MAPPER_DEVELOPMENT_BUILD
	if (!QFileInfo::exists(filepath))
		filepath = QLatin1String("data:/symbol sets/COPY_OF_")
		           + source_id + QLatin1Char('-')
		           + target_id + QLatin1String(".crt");
#endif
	if (!QFileInfo::exists(filepath))
		filepath.clear();
	return filepath;
}


QString SymbolReplacement::crtFileSideBySide(const QString& hint) const
{
	// CRT file side-by-side?
	auto const index_of_dot = hint.lastIndexOf(QLatin1Char('.'));
	QString filepath = hint.left(index_of_dot) + QLatin1String(".crt");
	if (!QFileInfo::exists(filepath))
		filepath.replace(index_of_dot, 4, QLatin1String(".CRT"));
	if (!QFileInfo::exists(filepath))
		filepath.clear();
	return filepath;
}



bool SymbolReplacement::withSymbolSetFileDialog(QWidget* parent)
{
	auto new_symbol_set = getOpenSymbolSet(parent);
	if (!new_symbol_set)
		return false;
	
	return SymbolReplacement(object_map, *new_symbol_set).withNewSymbolSet(parent);
}


bool SymbolReplacement::withNewSymbolSet(QWidget* parent)
{
	auto symbol_rules = SymbolRuleSet::forAllSymbols(object_map);
	symbol_rules.matchQuerySymbolName(symbol_set);
	
	auto const crt_file = crtFileById();
	if (!crt_file.isEmpty())
		symbol_rules.merge(loadCrtFile(parent, crt_file), SymbolRuleSet::UpdateOnly);
	
	SymbolReplacementDialog dialog(parent, object_map, symbol_set, symbol_rules, SymbolReplacementDialog::ReplaceSymbolSet);
	if (dialog.exec() != QDialog::Accepted)
		return false;
	
	std::move(symbol_rules).apply(object_map, symbol_set, dialog.replacementOptions());
	object_map.setSymbolSetId(dialog.replacementId());
	return true;
}


bool SymbolReplacement::withCrtFileDialog(QWidget* parent)
{
	auto const dir = QLatin1String{"data:/symbol sets"};
	auto const filter = QString{::OpenOrienteering::SymbolReplacementDialog::tr("CRT file") + QLatin1String{" (*.crt)"}};
	auto const filepath = FileDialog::getOpenFileName(parent,
	                                                  ::OpenOrienteering::SymbolReplacementDialog::tr("Open CRT file..."),
	                                                  dir,
	                                                  filter);
	if (filepath.isEmpty())
		return false;
	
	return withCrtFile(parent, filepath);
}


bool SymbolReplacement::withAutoCrtFile(QWidget* parent, const QString& hint)
{
	auto filepath = crtFileSideBySide(hint);
	if (filepath.isEmpty())
		filepath = crtFileById();
	if (!filepath.isEmpty())
		return withCrtFile(parent, filepath);
	
	auto symbol_rules = SymbolRuleSet::forUsedSymbols(object_map);
	symbol_rules.matchQuerySymbolName(symbol_set);
	
	SymbolReplacementDialog dialog(parent, object_map, symbol_set, symbol_rules, SymbolReplacementDialog::AssignByPattern);
	if (dialog.exec() != QDialog::Accepted)
		return false;
	
	std::move(symbol_rules).apply(object_map, symbol_set);
	return true;
}


bool SymbolReplacement::withCrtFile(QWidget* parent, const QString& filepath)
{
	auto symbol_rules = loadCrtFile(parent, filepath);
	if (symbol_rules.empty())
		return false;
	
	SymbolReplacementDialog dialog(parent, object_map, symbol_set, symbol_rules, SymbolReplacementDialog::AssignByPattern);
	if (dialog.exec() != QDialog::Accepted)
		return false;
	
	std::move(symbol_rules).apply(object_map, symbol_set);
	return true;
}



SymbolRuleSet SymbolReplacement::loadCrtFile(QWidget* parent, const QString& filepath) const
{
	QFile crt_file{filepath};
	if (!crt_file.open(QFile::ReadOnly))
	{
		QMessageBox::warning(parent,
		                     ::OpenOrienteering::SymbolReplacementDialog::tr("Error"),
		                     ::OpenOrienteering::SymbolReplacementDialog::tr("Cannot load symbol set, aborting."));
		return {};
	}
	
	QTextStream stream{&crt_file};
	auto symbol_rules = SymbolRuleSet::loadCrt(stream, symbol_set);
	if (stream.status() != QTextStream::Ok)
	{
		QMessageBox::warning(parent,
		                     ::OpenOrienteering::SymbolReplacementDialog::tr("Error"),
		                     ::OpenOrienteering::SymbolReplacementDialog::tr("Cannot load symbol set, aborting."));
		return {};
	}
	
	// Postprocess CRT
	symbol_rules.recognizeSymbolPatterns(object_map);
	if (auto const* ambiguous = symbol_rules.findDuplicateSymbolPattern())
	{
		auto error_msg = ::OpenOrienteering::SymbolReplacementDialog::tr("There are multiple replacements for symbol %1.")
		                 .arg(ambiguous->getNumberAsString());
		QMessageBox::warning(parent, ::OpenOrienteering::Map::tr("Error"),
		                     ::OpenOrienteering::SymbolReplacementDialog::tr("Cannot open file:\n%1\n\n%2")
		                     .arg(filepath, error_msg) );
		return {};
	}
	
	if (symbol_rules.empty())
	{
		// Empty return value would indicate an error. Push a no-op rule.
		symbol_rules.push_back({});
	}
	
	return symbol_rules;
}



std::unique_ptr<Map> SymbolReplacement::getOpenSymbolSet(QWidget* parent) const
{
	auto symbol_set = std::unique_ptr<Map>{};
	while (!symbol_set)
	{
		auto selected = MainWindow::getOpenFileName(
		                    parent,
		                    ::OpenOrienteering::SymbolReplacementDialog::tr("Choose map file to load symbols from"),
		                    FileFormat::MapFile);
		if (!selected)
		{
			// canceled
			break;
		}
		
		if (!selected.fileFormat())
		{
			/// \todo More precise message
			QMessageBox::warning(parent,
			                     ::OpenOrienteering::SymbolReplacementDialog::tr("Error"),
			                     ::OpenOrienteering::SymbolReplacementDialog::tr("Cannot load symbol set, aborting."));
			break;
		}
		
		symbol_set = std::make_unique<Map>();
		auto importer = selected.fileFormat()->makeImporter(selected.filePath(), symbol_set.get(), nullptr);
		if (!importer)
		{
			/// \todo More precise message
			QMessageBox::warning(parent,
			                     ::OpenOrienteering::SymbolReplacementDialog::tr("Error"),
			                     ::OpenOrienteering::SymbolReplacementDialog::tr("Cannot load symbol set, aborting."));
			break;
		}
		
		if (!importer->doImport())
		{
			/// \todo Show error from importer
			QMessageBox::warning(parent,
			                     ::OpenOrienteering::SymbolReplacementDialog::tr("Error"),
			                     ::OpenOrienteering::SymbolReplacementDialog::tr("Cannot load symbol set, aborting."));
			break;
		}
		
		if (!importer->warnings().empty())
		{
			MainWindow::showMessageBox(parent,
			                           ::OpenOrienteering::SymbolReplacementDialog::tr("Warning"),
			                           ::OpenOrienteering::SymbolReplacementDialog::tr("The symbol set import generated warnings."),
			                           importer->warnings());
		}
		
		if (object_map.getScaleDenominator() != symbol_set->getScaleDenominator())
		{
			if (QMessageBox::warning(parent,
			                         ::OpenOrienteering::SymbolReplacementDialog::tr("Warning"),
			                         ::OpenOrienteering::SymbolReplacementDialog::tr("The chosen symbol set has a scale of 1:%1,"
			                                                                         " while the map scale is 1:%2. Do you really"
			                                                                         " want to choose this set?")
			                         .arg(symbol_set->getScaleDenominator())
			                         .arg(object_map.getScaleDenominator()),
				QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
			{
				symbol_set.reset();
			}
		}
	}
	return symbol_set;
}


}  // namespace OpenOrienteering
