/*
 *    Copyright 2019-2020 Kai Pastor
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

#include <algorithm>
#include <cstring>
#include <iostream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include <cpl_string.h>
#include <gdal.h>

#include "gdal_extensions.h"

/*
 * Add prefix to ambiguous extensions in secondary string.
 * 
 * Both primary and secondary string are expected to be lines in the form
 * 
 * | short name | long name | one_extension another_extension |
 * 
 * so that a list of extensions can be found at the end between '|'  and ' ',
 * and each extension is a pattern like " one_extension ".
 */
void prefixDuplicates(std::string const& primary, std::string& secondary, const char* prefix)
{
	auto primary_end = primary.find_last_of(' ');
	auto primary_begin = primary.find_last_of('|', primary_end) + 1;
	
	auto secondary_end = secondary.find_last_of(' ');
	auto secondary_begin = secondary.find_last_of('|', secondary_end) + 1;
	
	while (primary_begin != primary_end)
	{
		auto pos = primary.find(' ', primary_begin + 1);
		auto pattern = primary.substr(primary_begin, pos - primary_begin + 1);
		auto found = secondary.find(pattern, secondary_begin);
		if (found != std::string::npos)
		{
			secondary.insert(found + 1, prefix);
			secondary_end += std::strlen(prefix);
		}
		primary_begin = pos;
	}
}

void prefixDuplicates(std::string const& primary, std::vector<std::string>& secondary_list, const char* prefix)
{
	for (auto& secondary : secondary_list)
	{
		prefixDuplicates(primary, secondary, prefix);
	}
}

void prefixDuplicates(std::vector<std::string> const& primary_list, std::vector<std::string>& secondary_list, const char* prefix)
{
	for (auto& primary : primary_list)
	{
		prefixDuplicates(primary, secondary_list, prefix);
	}
}


void dumpDriverList(std::string const& headline, std::vector<std::string> list)
{
	std::cout << headline << std::endl << std::endl;
	std::sort(begin(list), end(list));
	std::cout << "| Driver | Long name  | Extensions |" << std::endl;
	std::cout << "|--------|------------|------------|" << std::endl;
	std::for_each(begin(list), end(list), [](auto const& line) { std::cout << line << std::endl; });
	std::cout << std::endl;
}


void dumpGdalDrivers()
{
	auto qimagereader_extensions = std::string("| Qt | QImageReader | ");
	for (auto& extension : OpenOrienteering::gdal::qImageReaderExtensions<std::vector<const char*>>())
	{
		 qimagereader_extensions += extension;
		 qimagereader_extensions += " ";
	}
	qimagereader_extensions += "|";
	
	std::vector<std::string> raster_import_drivers;
	std::vector<std::string> vector_import_drivers;
	std::vector<std::string> secondary_vector_import_drivers;
	std::vector<std::string> vector_export_drivers;
	
	auto const count = GDALGetDriverCount();
	for (auto i = 0; i < count; ++i)
	{
		auto driver_data = GDALGetDriver(i);
		auto driver_name = GDALGetDriverShortName(driver_data);
		auto cap_raster = GDALGetMetadataItem(driver_data, GDAL_DCAP_RASTER, nullptr);
		auto cap_vector = GDALGetMetadataItem(driver_data, GDAL_DCAP_VECTOR, nullptr);
		auto cap_open = GDALGetMetadataItem(driver_data, GDAL_DCAP_OPEN, nullptr);
		auto cap_create = GDALGetMetadataItem(driver_data, GDAL_DCAP_CREATE, nullptr);
		auto extensions = GDALGetMetadataItem(driver_data, GDAL_DMD_EXTENSIONS, nullptr);
		auto help_topic = GDALGetMetadataItem(driver_data, GDAL_DMD_HELPTOPIC, nullptr);
		
		auto driver_line = std::string("| ");
		driver_line += driver_name;
		driver_line += " | ";
		if (help_topic)
		{
			driver_line += "[";
			driver_line += GDALGetDriverLongName(driver_data);
			driver_line += "](https://gdal.org/";
			driver_line += help_topic;
			driver_line += ")";
		}
		else
		{
			driver_line += GDALGetDriverLongName(driver_data);
		}
		driver_line += " | ";
		driver_line += extensions ? extensions : "";
		driver_line += " |";
		
		if (cap_raster && CPLTestBoolean(cap_raster))
		{
			if (cap_open && CPLTestBoolean(cap_open))
				raster_import_drivers.push_back(driver_line);
		}
		
		if (cap_vector && CPLTestBoolean(cap_vector))
		{
			if (cap_open && CPLTestBoolean(cap_open))
				vector_import_drivers.push_back(driver_line);
			if (cap_create && CPLTestBoolean(cap_create))
				vector_export_drivers.push_back(driver_line);
		}
	}
	
	// This block duplicates code in GdalManagerPrivate::updateExtensions().
	prefixDuplicates(qimagereader_extensions, raster_import_drivers, "raster.");
	
	std::cout << "## GDAL/OGR driver list for OpenOrienteering Mapper" << std::endl << std::endl;
	dumpDriverList("Raster import drivers", std::move(raster_import_drivers));
	dumpDriverList("Vector import drivers", std::move(vector_import_drivers));
	dumpDriverList("Vector export drivers", std::move(vector_export_drivers));
}


int main(int /*argc*/, char** /*argv*/)
{
	GDALAllRegister();
	dumpGdalDrivers();
	return 0;
}
