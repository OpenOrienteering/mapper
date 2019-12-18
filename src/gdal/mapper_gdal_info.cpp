/*
 *    Copyright 2019 Kai Pastor
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
#include <iostream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include <cpl_string.h>
#include <gdal.h>


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
	std::vector<std::string> raster_import_drivers;
	std::vector<std::string> vector_import_drivers;
	std::vector<std::string> vector_export_drivers;
	
	auto const count = GDALGetDriverCount();
	for (auto i = 0; i < count; ++i)
	{
		auto driver_data = GDALGetDriver(i);
		auto cap_raster = GDALGetMetadataItem(driver_data, GDAL_DCAP_RASTER, nullptr);
		auto cap_vector = GDALGetMetadataItem(driver_data, GDAL_DCAP_VECTOR, nullptr);
		auto cap_open = GDALGetMetadataItem(driver_data, GDAL_DCAP_OPEN, nullptr);
		auto cap_create = GDALGetMetadataItem(driver_data, GDAL_DCAP_CREATE, nullptr);
		auto extensions = GDALGetMetadataItem(driver_data, GDAL_DMD_EXTENSIONS, nullptr);
		auto help_topic = GDALGetMetadataItem(driver_data, GDAL_DMD_HELPTOPIC, nullptr);
		
		if (!extensions)
			continue;  // Not supported at the moment
		
		auto driver_line = std::string("| ");
		driver_line += GDALGetDriverShortName(driver_data);
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
