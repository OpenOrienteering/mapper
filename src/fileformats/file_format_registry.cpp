/*
 *    Copyright 2012, 2013 Pete Curtis
 *    Copyright 2013, 2015-2018 Kai Pastor
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

#include "file_format_registry.h"

#include <QFile>
#include <QFileInfo>

#include "fileformats/file_import_export.h"


namespace OpenOrienteering {

FileFormatRegistry FileFormats;


// ### FormatRegistry ###

FileFormatRegistry::FileFormatRegistry() noexcept
: default_format_id{ nullptr }
{
	// nothing else
}


FileFormatRegistry::~FileFormatRegistry()
{
	for (auto format : fmts)
		delete format;
}



void FileFormatRegistry::registerFormat(FileFormat *format)
{
	fmts.push_back(format);
	if (fmts.size() == 1) default_format_id = format->id();
	if (format->supportsReading())
	{
		// There must be at least one one format for a filename with the registered extension.
		Q_ASSERT(findFormatForFilename(QLatin1String("filename.") + format->primaryExtension(), &FileFormat::supportsReading) != nullptr);
		// The filter shall be unique at least by description.
		Q_ASSERT(findFormatByFilter(format->filter(), &FileFormat::supportsReading) == format); 
	}
	if (format->supportsWriting())
	{
		// There must be at least one one format for a filename with the registered extension.
		Q_ASSERT(findFormatForFilename(QLatin1String("filename.") + format->primaryExtension(), &FileFormat::supportsWriting) != nullptr);
		// The filter shall be unique at least by description.
		Q_ASSERT(findFormatByFilter(format->filter(), &FileFormat::supportsWriting) == format); 
	}
}

std::unique_ptr<FileFormat> FileFormatRegistry::unregisterFormat(const FileFormat* format)
{
	std::unique_ptr<FileFormat> ret;
	auto it = std::find(begin(fmts), end(fmts), format);
	if (it != end(fmts))
	{
		ret.reset(*it);
		fmts.erase(it);
	}
	return ret;
}


std::unique_ptr<Importer> FileFormatRegistry::makeImporter(const QString& path, Map& map, MapView* view)
{
	auto extension = QFileInfo(path).suffix();
	auto format = findFormat([extension](auto format) {
		return format->supportsReading()
		       && format->fileExtensions().contains(extension, Qt::CaseInsensitive);
	});
	if (!format)
		format = findFormatForData(path, FileFormat::AllFiles);
	return format ? format->makeImporter(path, &map, view) : nullptr;
}

std::unique_ptr<Exporter> FileFormatRegistry::makeExporter(const QString& path, const Map* map, const MapView* view)
{
	auto extension = QFileInfo(path).suffix();
	auto format = findFormat([extension](auto format) {
		return format->supportsWriting()
		       && format->fileExtensions().contains(extension, Qt::CaseInsensitive);
	});
	if (!format && QFileInfo::exists(path))
		format = findFormatForData(path, FileFormat::AllFiles);
	return format ? format->makeExporter(path, map, view) : nullptr;
}


const FileFormat* FileFormatRegistry::findFormat(std::function<bool (const FileFormat*)> predicate) const
{
	auto found = std::find_if(begin(fmts), end(fmts), predicate);
	return (found != end(fmts)) ? *found : nullptr;
}


const FileFormat *FileFormatRegistry::findFormat(const char* id) const
{
	return findFormat([id](auto format) { return qstrcmp(format->id(), id) == 0; });
}

const FileFormat *FileFormatRegistry::findFormatByFilter(const QString& filter, bool (FileFormat::*predicate)() const) const
{
	// Compare only before closing ')'. Needed for QTBUG 51712 workaround in
	// file_dialog.cpp, and warranted by Q_ASSERT in registerFormat().
	return findFormat([predicate, filter](auto format) {
		return (format->*predicate)()
		       && filter.startsWith(format->filter().leftRef(format->filter().length()-1));
	});
}

const FileFormat *FileFormatRegistry::findFormatForFilename(const QString& filename, bool (FileFormat::*predicate)() const) const
{
	auto extension = QFileInfo(filename).suffix();
	return findFormat([predicate, extension](auto format) {
		return (format->*predicate)()
		       && format->fileExtensions().contains(extension, Qt::CaseInsensitive);
	});
}

const FileFormat* FileFormatRegistry::findFormatForData(const QString& path, FileFormat::FileTypes types) const
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly))
		return nullptr;
	
	char buffer[256];
	auto total_read = int(file.read(buffer, file.isSequential() ? 0 : std::extent<decltype(buffer)>::value));
	
	FileFormat* candidate = nullptr;
	for (auto format : fmts)
	{
		if (!format->supportsReading() || !(format->fileType() & types))
			continue;
		
		switch (format->understands(buffer, total_read))
		{
		case FileFormat::NotSupported:
			break;
		case FileFormat::Unknown:
			if (!candidate)
				candidate = format;
			break;
		case FileFormat::FullySupported:
			return format;  // shortcut
		}
	}
	return candidate;
}


}  // namespace OpenOrienteering
