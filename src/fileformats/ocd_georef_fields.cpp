/*
 *    Copyright 2013-2017 Kai Pastor
 *    Copyright 2018 Libor Pecháček
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

#include "ocd_georef_fields.h"

#include <algorithm>
#include <iterator>
#include <utility>
#include <vector>

#include <QtGlobal>
#include <QtNumeric>
#include <QCoreApplication>
#include <QLatin1Char>
#include <QLatin1String>
#include <QPointF>
#include <QString>
#include <QStringList>

#include "core/crs_template.h"
#include "core/georeferencing.h"
#include "core/map_coord.h"
// translation context
#include "fileformats/ocd_file_import.h" // IWYU pragma: keep
#include "fileformats/ocd_file_export.h" // IWYU pragma: keep

namespace OpenOrienteering {

namespace {

enum struct MapperCrs
{
	Invalid,
	Local,
	Utm,
	GaussKrueger,
	Epsg,
};

constexpr struct
{
	MapperCrs key;
	const char* value;
} crs_string_map[] {
    { MapperCrs::Local, "Local" },
    { MapperCrs::Utm, "UTM" },
    { MapperCrs::GaussKrueger, "Gauss-Krueger, datum: Potsdam" },
    { MapperCrs::Epsg, "EPSG" },
};

/**
 * OcdGrid enum lists OCD country codes which happen to be thousands in the
 * `i' field value.
 */
enum struct OcdGrid
{
	Invalid = -1,
	Local = 1,
	Utm = 2,
	Austria = 3,
	BelgiumLambert = 4,
	Britain = 5,
	Finland = 6,
	France = 7,
	Germany = 8,
	Ireland = 9,
	Japan = 11,
	Norway = 12,
	Sweden = 13,
	Switzerland = 14,
	Slovenia = 15,
	Italy = 16,
	Iraq = 17,
	SouthAfrica = 18,
	NewZealand = 19,
	Australia = 20,
	Unknown21 = 21,
	NewZealand49 = 24,
	Lithuania = 26,
	Estonia = 27,
	Latvia = 28,
	Greece = 29,
	Unknown30 = 30,
	Croatia = 31,
	Unknown32 = 32,
	Luxembourg = 33,
	IrelandTm65 = 34,
	Albania = 35,
	Unknown36 = 36,
	Unknown37 = 37,
	Iceland = 38,
	Unknown39 = 39,
	Unknown40 = 40,
	Netherlands = 41,
	Portugal = 42,
	Unknown44 = 44,
	Czechia = 46,
	Slovakia = 47,
	Poland = 48,
	Hungary = 49,
	Usa = 50,
	Brazil = 52,
	Israel = 53,
	Malaysia = 54,
	GoogleTms = 56,
	IvoryCoast = 57,
	Hongkong = 61,
	UtmNad83 = 62,
	UtmEtrs89 = 63,
	PopularVisualisation = 64,
	Wgs84PseudoMercator = 65,
	UsaFeet = 66,
	KenyaTanzania = 68,
	Pulkovo42 = 69,
	Canada = 70,
};

constexpr struct
{
	const OcdGrid ocd_grid_id;
	const int ocd_zone_id, epsg_code;
} grid_zone_epsg_codes[] {
    // sort with `LC_ALL=C sort -k 1db,1 -k 2n -s -t ,`
    { OcdGrid::Albania, 0, 2462 }, // Albanian 1987 / Gauss-Kruger zone 4
    { OcdGrid::Australia, 1, 28348 }, // GDA94 / MGA zone 48
    { OcdGrid::Australia, 2, 28349 }, // GDA94 / MGA zone 49
    { OcdGrid::Australia, 3, 28350 }, // GDA94 / MGA zone 50
    { OcdGrid::Australia, 4, 28351 }, // GDA94 / MGA zone 51
    { OcdGrid::Australia, 5, 28352 }, // GDA94 / MGA zone 52
    { OcdGrid::Australia, 6, 28353 }, // GDA94 / MGA zone 53
    { OcdGrid::Australia, 7, 28354 }, // GDA94 / MGA zone 54
    { OcdGrid::Australia, 8, 28355 }, // GDA94 / MGA zone 55
    { OcdGrid::Australia, 9, 28356 }, // GDA94 / MGA zone 56
    { OcdGrid::Australia, 10, 28357 }, // GDA94 / MGA zone 57
    { OcdGrid::Australia, 11, 28358 }, // GDA94 / MGA zone 58
    { OcdGrid::Australia, 12, 3112 }, // GDA94 / Geoscience Australia Lambert
    { OcdGrid::Austria, 28, 31257 }, // MGI / Austria GK M28
    { OcdGrid::Austria, 31, 31258 }, // MGI / Austria GK M31
    { OcdGrid::Austria, 34, 31259 }, // MGI / Austria GK M34
    { OcdGrid::Austria, 35, 31254 }, // MGI / Austria GK West
    { OcdGrid::Austria, 36, 31255 }, // MGI / Austria GK Central
    { OcdGrid::Austria, 38, 31256 }, // MGI / Austria GK East
    { OcdGrid::BelgiumLambert, 1, 31300 }, // Belge 1972 / Belge Lambert 72
    { OcdGrid::BelgiumLambert, 2, 3447 }, // ETRS89 / Belgian Lambert 2005
    { OcdGrid::BelgiumLambert, 3, 3812 }, // ETRS89 / Belgian Lambert 2008
    { OcdGrid::Brazil, 4, 22521 }, // Corrego Alegre 1970-72 / UTM zone 21S
    { OcdGrid::Brazil, 5, 22522 }, // Corrego Alegre 1970-72 / UTM zone 22S
    { OcdGrid::Brazil, 6, 22523 }, // Corrego Alegre 1970-72 / UTM zone 23S
    { OcdGrid::Brazil, 7, 22524 }, // Corrego Alegre 1970-72 / UTM zone 24S
    { OcdGrid::Brazil, 8, 22525 }, // Corrego Alegre 1970-72 / UTM zone 25S
    { OcdGrid::Brazil, 9, 5875 }, // SAD69(96) / UTM zone 18S
    { OcdGrid::Brazil, 10, 5876 }, // SAD69(96) / UTM zone 19S
    { OcdGrid::Brazil, 11, 5877 }, // SAD69(96) / UTM zone 20S
    { OcdGrid::Brazil, 12, 5531 }, // SAD69(96) / UTM zone 21S
    { OcdGrid::Brazil, 13, 5858 }, // SAD69(96) / UTM zone 22S
    { OcdGrid::Brazil, 14, 5533 }, // SAD69(96) / UTM zone 23S
    { OcdGrid::Brazil, 15, 5534 }, // SAD69(96) / UTM zone 24S
    { OcdGrid::Brazil, 16, 5535 }, // SAD69(96) / UTM zone 25S
    { OcdGrid::Brazil, 17, 31978 }, // SIRGAS 2000 / UTM zone 18S
    { OcdGrid::Brazil, 18, 31979 }, // SIRGAS 2000 / UTM zone 19S
    { OcdGrid::Brazil, 19, 31980 }, // SIRGAS 2000 / UTM zone 20S
    { OcdGrid::Brazil, 20, 31981 }, // SIRGAS 2000 / UTM zone 21S
    { OcdGrid::Brazil, 21, 31982 }, // SIRGAS 2000 / UTM zone 22S
    { OcdGrid::Brazil, 22, 31983 }, // SIRGAS 2000 / UTM zone 23S
    { OcdGrid::Brazil, 23, 31984 }, // SIRGAS 2000 / UTM zone 24S
    { OcdGrid::Brazil, 24, 31985 }, // SIRGAS 2000 / UTM zone 25S
    { OcdGrid::Britain, 0, 27700 }, // OSGB 1936
    { OcdGrid::Canada, 0, 2019 }, // NAD27(76) / MTM zone 10
    { OcdGrid::Croatia, 7, 3765 }, // HTRS96 / Croatia TM
    { OcdGrid::Czechia, 1, 5514 }, // S-JTSK Krovak East North
    { OcdGrid::Czechia, 2, 28403 }, // Pulkovo 1942 / Gauss-Kruger zone 3
    { OcdGrid::Estonia, 0, 3301 }, // Estonian Coordinate System of 1997
    { OcdGrid::Finland, 1, 2391 }, // KKJ / Finland zone 1
    { OcdGrid::Finland, 2, 2392 }, // KKJ / Finland zone 2
    { OcdGrid::Finland, 3, 2393 }, // KKJ / Finland Uniform Coordinate System
    { OcdGrid::Finland, 4, 2394 }, // KKJ / Finland zone 4
    { OcdGrid::Finland, 5, 3067 }, // TM35FIN, ETRS89 / ETRS-TM35FIN
    { OcdGrid::Finland, 6, 3873 }, // ETRS89 / GK19FIN
    { OcdGrid::Finland, 7, 3874 }, // ETRS89 / GK20FIN
    { OcdGrid::Finland, 8, 3875 }, // ETRS89 / GK21FIN
    { OcdGrid::Finland, 9, 3876 }, // ETRS89 / GK22FIN
    { OcdGrid::Finland, 10, 3877 }, // ETRS89 / GK23FIN
    { OcdGrid::Finland, 11, 3878 }, // ETRS89 / GK24FIN
    { OcdGrid::Finland, 12, 3879 }, // ETRS89 / GK25FIN
    { OcdGrid::Finland, 13, 3880 }, // ETRS89 / GK26FIN
    { OcdGrid::Finland, 14, 3881 }, // ETRS89 / GK27FIN
    { OcdGrid::Finland, 15, 3882 }, // ETRS89 / GK28FIN
    { OcdGrid::Finland, 16, 3883 }, // ETRS89 / GK29FIN
    { OcdGrid::Finland, 17, 3884 }, // ETRS89 / GK30FIN
    { OcdGrid::Finland, 18, 3885 }, // ETRS89 / GK31FIN
    { OcdGrid::Finland, 19, 3126 }, // ETRS89 / ETRS-GK19FIN
    { OcdGrid::Finland, 20, 3127 }, // ETRS89 / ETRS-GK20FIN
    { OcdGrid::Finland, 21, 3128 }, // ETRS89 / ETRS-GK21FIN
    { OcdGrid::Finland, 22, 3129 }, // ETRS89 / ETRS-GK22FIN
    { OcdGrid::Finland, 23, 3130 }, // ETRS89 / ETRS-GK23FIN
    { OcdGrid::Finland, 24, 3131 }, // ETRS89 / ETRS-GK24FIN
    { OcdGrid::Finland, 25, 3132 }, // ETRS89 / ETRS-GK25FIN
    { OcdGrid::Finland, 26, 3133 }, // ETRS89 / ETRS-GK26FIN
    { OcdGrid::Finland, 27, 3134 }, // ETRS89 / ETRS-GK27FIN
    { OcdGrid::Finland, 28, 3135 }, // ETRS89 / ETRS-GK28FIN
    { OcdGrid::Finland, 29, 3136 }, // ETRS89 / ETRS-GK29FIN
    { OcdGrid::Finland, 30, 3137 }, // ETRS89 / ETRS-GK30FIN
    { OcdGrid::Finland, 31, 3138 }, // ETRS89 / ETRS-GK31FIN
    { OcdGrid::Finland, 32, 3387 }, // KKJ / Finland zone 5
    { OcdGrid::France, 1, 27591 }, // NTF_Paris_Nord_France
    { OcdGrid::France, 2, 27581 }, // NTF_Paris_France_I
    { OcdGrid::France, 3, 27592 }, // NTF_Paris_Centre_France
    { OcdGrid::France, 4, 27572 }, // NTF (Paris) / Lambert zone II
    { OcdGrid::France, 5, 27593 }, // NTF_Paris_Sud_France
    { OcdGrid::France, 6, 27583 }, // NTF_Paris_France_III
    { OcdGrid::France, 7, 27594 }, // NTF_Paris_Corse
    { OcdGrid::France, 8, 27584 }, // NTF_Paris_France_IV
    { OcdGrid::France, 9, 2154 }, // RGF93 / Lambert-93
    { OcdGrid::France, 10, 3942 }, // RGF93 / CC42
    { OcdGrid::France, 11, 3943 }, // RGF93 / CC43
    { OcdGrid::France, 12, 3944 }, // RGF93 / CC44
    { OcdGrid::France, 13, 3945 }, // RGF93 / CC45
    { OcdGrid::France, 14, 3946 }, // RGF93 / CC46
    { OcdGrid::France, 15, 3947 }, // RGF93 / CC47
    { OcdGrid::France, 16, 3948 }, // RGF93 / CC48
    { OcdGrid::France, 17, 3949 }, // RGF93 / CC49
    { OcdGrid::France, 18, 3950 }, // RGF93 / CC50
    { OcdGrid::Germany, 2, 31466 }, // DHDN, Gauss-Krüger Zone 2
    { OcdGrid::Germany, 3, 31467 }, // DHDN, Gauss-Krüger Zone 3
    { OcdGrid::Germany, 4, 31468 }, // DHDN, Gauss-Krüger Zone 4
    { OcdGrid::Germany, 5, 31469 }, // DHDN, Gauss-Krüger Zone 5
    { OcdGrid::Germany, 6, 25831 }, // ETRS89 / UTM zone 31N
    { OcdGrid::Germany, 7, 25832 }, // ETRS89 / UTM zone 32N
    { OcdGrid::Germany, 8, 25833 }, // ETRS89 / UTM zone 33N
    { OcdGrid::Germany, 13, 3068 }, // DHDN / Soldner Berlin
    { OcdGrid::Germany, 14, 3068 }, // DHDN / Soldner Berlin
    { OcdGrid::GoogleTms, 0, 900913 }, // Google Mercator
    { OcdGrid::Greece, 0, 2100 }, // GGRS87 / Greek Grid
    { OcdGrid::Hongkong, 0, 2326 }, // Hong Kong 1980 Grid System
    { OcdGrid::Hungary, 0, 23700 }, // HD72 / EOV
    { OcdGrid::Iceland, 4, 3057 }, // ISN93 / Lambert 1993
    { OcdGrid::Iraq, 1, 2206 }, // ED50 / 3-degree Gauss-Kruger zone 9
    { OcdGrid::Iraq, 2, 2207 }, // ED50 / 3-degree Gauss-Kruger zone 10
    { OcdGrid::Iraq, 3, 2208 }, // ED50 / 3-degree Gauss-Kruger zone 11
    { OcdGrid::Iraq, 4, 2209 }, // ED50 / 3-degree Gauss-Kruger zone 12
    { OcdGrid::Iraq, 5, 2210 }, // ED50 / 3-degree Gauss-Kruger zone 13
    { OcdGrid::Iraq, 6, 2211 }, // ED50 / 3-degree Gauss-Kruger zone 14
    { OcdGrid::Iraq, 7, 2212 }, // ED50 / 3-degree Gauss-Kruger zone 15
    { OcdGrid::Iraq, 8, 7006 }, // Nahrwan 1934 / UTM zone 38N
    { OcdGrid::Ireland, 1, 29902 }, // TM65 / Irish Grid
    { OcdGrid::Ireland, 2, 29903 }, // TM75 / Irish Grid
    { OcdGrid::Ireland, 3, 2157 }, // IRENET95 / Irish Transverse Mercator
    { OcdGrid::IrelandTm65, 0, 29902 }, // TM65 / Irish Grid
    { OcdGrid::Israel, 0, 2039 }, // Israel 1993 / Israeli TM Grid
    { OcdGrid::Italy, 1, 4685 }, // 
    { OcdGrid::Italy, 2, 4686 }, // MAGNA-SIRGAS
    { OcdGrid::Italy, 3, 23031 }, // ED50 / UTM zone 31N
    { OcdGrid::Italy, 4, 23032 }, // ED50 / UTM zone 32N
    { OcdGrid::Italy, 5, 23033 }, // ED50 / UTM zone 33N
    { OcdGrid::Italy, 6, 23034 }, // ED50 / UTM zone 34N
    { OcdGrid::Italy, 7, 25832 }, // ETRS89 / UTM zone 32N
    { OcdGrid::Italy, 8, 25833 }, // ETRS89 / UTM zone 33N
    { OcdGrid::Italy, 9, 6707 }, // RDN2008 / UTM zone 32N (N-E)
    { OcdGrid::Italy, 10, 6708 }, // RDN2008 / UTM zone 33N (N-E)
    { OcdGrid::Italy, 11, 6875 }, // RDN2008 / Italy zone (N-E)
    { OcdGrid::Italy, 12, 6876 }, // RDN2008 / Zone 12 (N-E)
    { OcdGrid::IvoryCoast, 0, 2041 }, // Abidjan 1987 / UTM zone 30N
    { OcdGrid::Japan, 1, 30161 }, // Tokyo / Japan Plane Rectangular CS I
    { OcdGrid::Japan, 2, 30162 }, // Tokyo / Japan Plane Rectangular CS II
    { OcdGrid::Japan, 3, 30163 }, // Tokyo / Japan Plane Rectangular CS III
    { OcdGrid::Japan, 4, 30164 }, // Tokyo / Japan Plane Rectangular CS IV
    { OcdGrid::Japan, 5, 30165 }, // Tokyo / Japan Plane Rectangular CS V
    { OcdGrid::Japan, 6, 30166 }, // Tokyo / Japan Plane Rectangular CS VI
    { OcdGrid::Japan, 7, 30167 }, // Tokyo / Japan Plane Rectangular CS VII
    { OcdGrid::Japan, 8, 30168 }, // Tokyo / Japan Plane Rectangular CS VIII
    { OcdGrid::Japan, 9, 30169 }, // Tokyo / Japan Plane Rectangular CS IX
    { OcdGrid::Japan, 10, 30170 }, // Tokyo / Japan Plane Rectangular CS X
    { OcdGrid::Japan, 11, 30171 }, // Tokyo / Japan Plane Rectangular CS XI
    { OcdGrid::Japan, 12, 30172 }, // Tokyo / Japan Plane Rectangular CS XII
    { OcdGrid::Japan, 13, 30173 }, // Tokyo / Japan Plane Rectangular CS XIII
    { OcdGrid::Japan, 14, 30174 }, // Tokyo / Japan Plane Rectangular CS XIV
    { OcdGrid::Japan, 15, 30175 }, // Tokyo / Japan Plane Rectangular CS XV
    { OcdGrid::Japan, 16, 30176 }, // Tokyo / Japan Plane Rectangular CS XVI
    { OcdGrid::Japan, 17, 30177 }, // Tokyo / Japan Plane Rectangular CS XVII
    { OcdGrid::Japan, 18, 30178 }, // Tokyo / Japan Plane Rectangular CS XVIII
    { OcdGrid::Japan, 19, 30179 }, // Tokyo / Japan Plane Rectangular CS XIX
    { OcdGrid::Japan, 20, 2443 }, // JGD2000 / Japan Plane Rectangular CS I
    { OcdGrid::Japan, 21, 2444 }, // JGD2000 / Japan Plane Rectangular CS II
    { OcdGrid::Japan, 22, 2445 }, // JGD2000 / Japan Plane Rectangular CS III
    { OcdGrid::Japan, 23, 2446 }, // JGD2000 / Japan Plane Rectangular CS IV
    { OcdGrid::Japan, 24, 2447 }, // JGD2000 / Japan Plane Rectangular CS V
    { OcdGrid::Japan, 25, 2448 }, // JGD2000 / Japan Plane Rectangular CS VI
    { OcdGrid::Japan, 26, 2449 }, // JGD2000 / Japan Plane Rectangular CS VII
    { OcdGrid::Japan, 27, 2450 }, // JGD2000 / Japan Plane Rectangular CS VIII
    { OcdGrid::Japan, 28, 2451 }, // JGD2000 / Japan Plane Rectangular CS IX
    { OcdGrid::Japan, 29, 2452 }, // JGD2000 / Japan Plane Rectangular CS X
    { OcdGrid::Japan, 30, 2453 }, // JGD2000 / Japan Plane Rectangular CS XI
    { OcdGrid::Japan, 31, 2454 }, // JGD2000 / Japan Plane Rectangular CS XII
    { OcdGrid::Japan, 32, 2455 }, // JGD2000 / Japan Plane Rectangular CS XIII
    { OcdGrid::Japan, 33, 2456 }, // JGD2000 / Japan Plane Rectangular CS XIV
    { OcdGrid::Japan, 34, 2457 }, // JGD2000 / Japan Plane Rectangular CS XV
    { OcdGrid::Japan, 35, 2458 }, // JGD2000 / Japan Plane Rectangular CS XVI
    { OcdGrid::Japan, 36, 2459 }, // JGD2000 / Japan Plane Rectangular CS XVII
    { OcdGrid::Japan, 37, 2460 }, // JGD2000 / Japan Plane Rectangular CS XVIII
    { OcdGrid::Japan, 39, 2461 }, // JGD2000 / Japan Plane Rectangular CS XIX
    { OcdGrid::KenyaTanzania, 0, 21037 }, // Arc 1960 / UTM zone 37S
    { OcdGrid::Latvia, 0, 3059 }, // LKS92 / Latvia TM
    { OcdGrid::Lithuania, 0, 3346 }, // LKS94 / Lithuania TM
    { OcdGrid::Luxembourg, 1, 2169 }, // Luxembourg 1930 / Gauss
    { OcdGrid::Luxembourg, 2, 23031 }, // ED50 / UTM zone 31N
    { OcdGrid::Malaysia, 1, 3375 }, // GDM2000 / Peninsula RSO
    { OcdGrid::Malaysia, 2, 3376 }, // GDM2000 / East Malaysia BRSO
    { OcdGrid::Netherlands, 1, 28991 }, // Amersfoort / RD Old
    { OcdGrid::Netherlands, 2, 23031 }, // ED50 / UTM zone 31N
    { OcdGrid::Netherlands, 3, 23032 }, // ED50 / UTM zone 32N
    { OcdGrid::Netherlands, 4, 28992 }, // Amersfoort / RD New
    { OcdGrid::NewZealand, 0, 2193 }, // NZGD2000 / New Zealand Transverse Mercator 2000
    { OcdGrid::NewZealand49, 0, 4272 }, // NZGD49
    { OcdGrid::Norway, 1, 27391 }, // NGO 1948 (Oslo) / NGO zone I
    { OcdGrid::Norway, 2, 27392 }, // NGO 1948 (Oslo) / NGO zone II
    { OcdGrid::Norway, 3, 27393 }, // NGO 1948 (Oslo) / NGO zone III
    { OcdGrid::Norway, 4, 27394 }, // NGO 1948 (Oslo) / NGO zone IV
    { OcdGrid::Norway, 5, 27395 }, // NGO 1948 (Oslo) / NGO zone V
    { OcdGrid::Norway, 6, 27396 }, // NGO 1948 (Oslo) / NGO zone VI
    { OcdGrid::Norway, 7, 27397 }, // NGO 1948 (Oslo) / NGO zone VII
    { OcdGrid::Norway, 8, 27398 }, // NGO 1948 (Oslo) / NGO zone VIII
    { OcdGrid::Norway, 9, 23031 }, // ED50 / UTM zone 31N
    { OcdGrid::Norway, 10, 23032 }, // ED50 / UTM zone 32N
    { OcdGrid::Norway, 11, 23033 }, // ED50 / UTM zone 33N
    { OcdGrid::Norway, 12, 23034 }, // ED50 / UTM zone 34N
    { OcdGrid::Norway, 13, 23035 }, // ED50 / UTM zone 35N
    { OcdGrid::Norway, 14, 23036 }, // ED50 / UTM zone 36N
    { OcdGrid::Poland, 1, 2180 }, // ETRS89 / Poland CS92
    { OcdGrid::Poland, 2, 2176 }, // ETRS89 / Poland CS2000 zone 5
    { OcdGrid::Poland, 3, 2177 }, // ETRS89 / Poland CS2000 zone 6
    { OcdGrid::Poland, 4, 2178 }, // ETRS89 / Poland CS2000 zone 7
    { OcdGrid::Poland, 5, 2179 }, // ETRS89 / Poland CS2000 zone 8
    { OcdGrid::PopularVisualisation, 0, 3785 }, // 
    { OcdGrid::Portugal, 1, 27493 }, // Datum 73 / Modified Portuguese Grid
    { OcdGrid::Portugal, 2, 23028 }, // ED50 / UTM zone 28N
    { OcdGrid::Pulkovo42, 0, 2586 }, // Pulkovo 1942 / 3-degree Gauss-Kruger CM 33E
    { OcdGrid::Slovakia, 1, 5514 }, // S-JTSK Krovak East North
    { OcdGrid::Slovakia, 2, 28403 }, // Pulkovo_1942_GK_Zone_3
    { OcdGrid::Slovakia, 3, 28404 }, // Pulkovo 1942 / Gauss-Kruger zone 4
    { OcdGrid::Slovenia, 1, 3912 }, // MGI 1901 / Slovene National Grid
    { OcdGrid::Slovenia, 1, 3912 }, // MGI 1901 / Slovene National Grid
    { OcdGrid::Slovenia, 2, 3794 }, // Slovenia 1996 / Slovene National Grid
    { OcdGrid::SouthAfrica, 1, 2046 }, // Hartebeesthoek94 / Lo15
    { OcdGrid::SouthAfrica, 2, 2047 }, // Hartebeesthoek94 / Lo17
    { OcdGrid::SouthAfrica, 3, 2048 }, // Hartebeesthoek94 / Lo19
    { OcdGrid::SouthAfrica, 4, 2049 }, // Hartebeesthoek94 / Lo21
    { OcdGrid::SouthAfrica, 5, 2050 }, // Hartebeesthoek94 / Lo23
    { OcdGrid::SouthAfrica, 6, 2051 }, // Hartebeesthoek94 / Lo25
    { OcdGrid::SouthAfrica, 7, 2052 }, // Hartebeesthoek94 / Lo27
    { OcdGrid::SouthAfrica, 8, 2053 }, // Hartebeesthoek94 / Lo29
    { OcdGrid::SouthAfrica, 9, 2054 }, // Hartebeesthoek94 / Lo31
    { OcdGrid::SouthAfrica, 10, 2055 }, // Hartebeesthoek94 / Lo33
    { OcdGrid::Sweden, 1, 6124 }, // WGS 84 / EPSG Arctic zone 4-12
    { OcdGrid::Sweden, 2, 3006 }, // SWEREF99 TM
    { OcdGrid::Sweden, 3, 3007 }, // SWEREF99 12 00
    { OcdGrid::Sweden, 4, 3008 }, // SWEREF99 13 30
    { OcdGrid::Sweden, 5, 3009 }, // SWEREF99 15 00
    { OcdGrid::Sweden, 6, 3010 }, // SWEREF99 16 30
    { OcdGrid::Sweden, 7, 3011 }, // SWEREF99 18 00
    { OcdGrid::Sweden, 8, 3012 }, // SWEREF99 14 15
    { OcdGrid::Sweden, 9, 3013 }, // SWEREF99 15 45
    { OcdGrid::Sweden, 10, 3014 }, // SWEREF99 17 15
    { OcdGrid::Sweden, 11, 3015 }, // SWEREF99 18 45
    { OcdGrid::Sweden, 12, 3016 }, // SWEREF99 20 15
    { OcdGrid::Sweden, 13, 3017 }, // SWEREF99 21 45
    { OcdGrid::Sweden, 14, 3018 }, // SWEREF99 23 15
    { OcdGrid::Switzerland, 1, 21781 }, // CH1903 / LV03 -- Swiss CH1903 / LV03
    { OcdGrid::Switzerland, 2, 2056 }, // CH1903+ / LV95 -- Swiss CH1903+ / LV95
    { OcdGrid::Unknown21, 1, 23032 }, // ED50 / UTM zone 32N
    { OcdGrid::Unknown21, 2, 23033 }, // ED50 / UTM zone 33N
    { OcdGrid::Unknown21, 3, 25832 }, // ETRS89 / UTM zone 32N
    { OcdGrid::Unknown21, 4, 25833 }, // ETRS89 / UTM zone 33N
    { OcdGrid::Unknown30, 1, 23028 }, // ED50 / UTM zone 28N
    { OcdGrid::Unknown30, 2, 23029 }, // ED50 / UTM zone 29N
    { OcdGrid::Unknown30, 3, 23030 }, // ED50 / UTM zone 30N
    { OcdGrid::Unknown30, 4, 23030 }, // ED50 / UTM zone 30N
    { OcdGrid::Unknown30, 5, 23031 }, // ED50 / UTM zone 31N
    { OcdGrid::Unknown30, 6, 23031 }, // ED50 / UTM zone 31N
    { OcdGrid::Unknown32, 0, 23033 }, // ED50 / UTM zone 33N
    { OcdGrid::Unknown36, 1, 25833 }, // ETRS89 / UTM zone 33N
    { OcdGrid::Unknown36, 2, 25834 }, // ETRS89 / UTM zone 34N
    { OcdGrid::Unknown37, 1, 25834 }, // ETRS89 / UTM zone 34N
    { OcdGrid::Unknown37, 2, 25835 }, // ETRS89 / UTM zone 35N
    { OcdGrid::Unknown39, 0, 23033 }, // ED50 / UTM zone 33N
    { OcdGrid::Unknown40, 0, 23032 }, // ED50 / UTM zone 32N
    { OcdGrid::Unknown44, 0, 23033 }, // ED50 / UTM zone 33N
    { OcdGrid::Usa, 1, 26929 }, // NAD83 / Alabama East
    { OcdGrid::Usa, 2, 26930 }, // NAD83 / Alabama West
    { OcdGrid::Usa, 4, 26932 }, // NAD83 / Alaska zone 2
    { OcdGrid::Usa, 5, 26933 }, // NAD83 / Alaska zone 3
    { OcdGrid::Usa, 6, 26934 }, // NAD83 / Alaska zone 4
    { OcdGrid::Usa, 7, 26935 }, // NAD83 / Alaska zone 5
    { OcdGrid::Usa, 8, 26936 }, // NAD83 / Alaska zone 6
    { OcdGrid::Usa, 9, 26937 }, // NAD83 / Alaska zone 7
    { OcdGrid::Usa, 10, 26938 }, // NAD83 / Alaska zone 8
    { OcdGrid::Usa, 11, 26939 }, // NAD83 / Alaska zone 9
    { OcdGrid::Usa, 12, 26940 }, // NAD83 / Alaska zone 10
    { OcdGrid::Usa, 13, 26948 }, // NAD83 / Arizona East
    { OcdGrid::Usa, 14, 26949 }, // NAD83 / Arizona Central
    { OcdGrid::Usa, 15, 26950 }, // NAD83 / Arizona West
    { OcdGrid::Usa, 16, 26951 }, // NAD83 / Arkansas North
    { OcdGrid::Usa, 17, 26952 }, // NAD83 / Arkansas South
    { OcdGrid::Usa, 18, 26941 }, // NAD83 / California zone 1
    { OcdGrid::Usa, 19, 26942 }, // NAD83 / California zone 2
    { OcdGrid::Usa, 20, 26943 }, // NAD83 / California zone 3
    { OcdGrid::Usa, 21, 26944 }, // NAD83 / California zone 4
    { OcdGrid::Usa, 22, 26945 }, // NAD83 / California zone 5
    { OcdGrid::Usa, 23, 26946 }, // NAD83 / California zone 6
    { OcdGrid::Usa, 24, 26953 }, // NAD83 / Colorado North
    { OcdGrid::Usa, 25, 26954 }, // NAD83 / Colorado Central
    { OcdGrid::Usa, 26, 26955 }, // NAD83 / Colorado South
    { OcdGrid::Usa, 27, 26956 }, // NAD83 / Connecticut
    { OcdGrid::Usa, 28, 26957 }, // NAD83 / Delaware
    { OcdGrid::Usa, 29, 26958 }, // NAD83 / Florida East
    { OcdGrid::Usa, 30, 26959 }, // NAD83 / Florida West
    { OcdGrid::Usa, 31, 26960 }, // NAD83 / Florida North
    { OcdGrid::Usa, 32, 26966 }, // NAD83 / Georgia East
    { OcdGrid::Usa, 33, 26967 }, // NAD83 / Georgia West
    { OcdGrid::Usa, 34, 26961 }, // NAD83 / Hawaii zone 1
    { OcdGrid::Usa, 35, 26962 }, // NAD83 / Hawaii zone 2
    { OcdGrid::Usa, 36, 26963 }, // NAD83 / Hawaii zone 3
    { OcdGrid::Usa, 37, 26964 }, // NAD83 / Hawaii zone 4
    { OcdGrid::Usa, 38, 26965 }, // NAD83 / Hawaii zone 5
    { OcdGrid::Usa, 39, 26968 }, // NAD83 / Idaho East
    { OcdGrid::Usa, 40, 26969 }, // NAD83 / Idaho Central
    { OcdGrid::Usa, 41, 26970 }, // NAD83 / Idaho West
    { OcdGrid::Usa, 42, 26971 }, // NAD83 / Illinois East
    { OcdGrid::Usa, 43, 26972 }, // NAD83 / Illinois West
    { OcdGrid::Usa, 44, 26973 }, // NAD83 / Indiana East
    { OcdGrid::Usa, 45, 26974 }, // NAD83 / Indiana West
    { OcdGrid::Usa, 46, 26975 }, // NAD83 / Iowa North
    { OcdGrid::Usa, 47, 26976 }, // NAD83 / Iowa South
    { OcdGrid::Usa, 48, 26977 }, // NAD83 / Kansas North
    { OcdGrid::Usa, 49, 26978 }, // NAD83 / Kansas South
    { OcdGrid::Usa, 50, 26979 }, // NAD_1983_StatePlane_Kentucky_North_FIPS_1601
    { OcdGrid::Usa, 51, 26980 }, // NAD83 / Kentucky South
    { OcdGrid::Usa, 52, 26981 }, // NAD83 / Louisiana North
    { OcdGrid::Usa, 53, 26982 }, // NAD83 / Louisiana South
    { OcdGrid::Usa, 54, 32199 }, // NAD83 / Louisiana Offshore
    { OcdGrid::Usa, 55, 26983 }, // NAD83 / Maine East
    { OcdGrid::Usa, 56, 26984 }, // NAD83 / Maine West
    { OcdGrid::Usa, 57, 26985 }, // NAD83 / Maryland
    { OcdGrid::Usa, 58, 26986 }, // NAD83 / Massachusetts Mainland
    { OcdGrid::Usa, 59, 26987 }, // NAD83 / Massachusetts Island
    { OcdGrid::Usa, 60, 26988 }, // NAD83 / Michigan North
    { OcdGrid::Usa, 61, 26989 }, // NAD83 / Michigan Central
    { OcdGrid::Usa, 62, 26990 }, // NAD83 / Michigan South
    { OcdGrid::Usa, 63, 26991 }, // NAD83 / Minnesota North
    { OcdGrid::Usa, 64, 26992 }, // NAD83 / Minnesota Central
    { OcdGrid::Usa, 65, 26993 }, // NAD83 / Minnesota South
    { OcdGrid::Usa, 66, 26994 }, // NAD83 / Mississippi East
    { OcdGrid::Usa, 67, 26995 }, // NAD83 / Mississippi West
    { OcdGrid::Usa, 68, 26996 }, // NAD83 / Missouri East
    { OcdGrid::Usa, 69, 26997 }, // NAD83 / Missouri Central
    { OcdGrid::Usa, 70, 26998 }, // NAD83 / Missouri West
    { OcdGrid::Usa, 71, 32100 }, // NAD83 / Montana
    { OcdGrid::Usa, 72, 32104 }, // NAD83 / Nebraska
    { OcdGrid::Usa, 73, 32107 }, // NAD83 / Nevada East
    { OcdGrid::Usa, 74, 32108 }, // NAD83 / Nevada Central
    { OcdGrid::Usa, 75, 32109 }, // NAD83 / Nevada West
    { OcdGrid::Usa, 76, 32110 }, // NAD83 / New Hampshire
    { OcdGrid::Usa, 77, 32111 }, // NAD83 / New Jersey
    { OcdGrid::Usa, 78, 32112 }, // NAD83 / New Mexico East
    { OcdGrid::Usa, 79, 32113 }, // NAD83 / New Mexico Central
    { OcdGrid::Usa, 80, 32114 }, // NAD83 / New Mexico West
    { OcdGrid::Usa, 81, 32115 }, // NAD83 / New York East
    { OcdGrid::Usa, 82, 32116 }, // NAD83 / New York Central
    { OcdGrid::Usa, 83, 32117 }, // NAD83 / New York West
    { OcdGrid::Usa, 84, 32118 }, // NAD83 / New York Long Island
    { OcdGrid::Usa, 85, 32119 }, // NAD83 / North Carolina
    { OcdGrid::Usa, 86, 32120 }, // NAD83 / North Dakota North
    { OcdGrid::Usa, 87, 32121 }, // NAD83 / North Dakota South
    { OcdGrid::Usa, 88, 32122 }, // NAD83 / Ohio North
    { OcdGrid::Usa, 89, 32123 }, // NAD83 / Ohio South
    { OcdGrid::Usa, 90, 32124 }, // NAD83 / Oklahoma North
    { OcdGrid::Usa, 91, 32125 }, // NAD83 / Oklahoma South
    { OcdGrid::Usa, 92, 32126 }, // NAD83 / Oregon North
    { OcdGrid::Usa, 93, 32127 }, // NAD83 / Oregon South
    { OcdGrid::Usa, 94, 32128 }, // NAD83 / Pennsylvania North
    { OcdGrid::Usa, 95, 32129 }, // NAD83 / Pennsylvania South
    { OcdGrid::Usa, 96, 32130 }, // NAD83 / Rhode Island
    { OcdGrid::Usa, 97, 32133 }, // NAD83 / South Carolina
    { OcdGrid::Usa, 98, 32134 }, // NAD83 / South Dakota North
    { OcdGrid::Usa, 99, 32135 }, // NAD83 / South Dakota South
    { OcdGrid::Usa, 100, 32136 }, // NAD83 / Tennessee
    { OcdGrid::Usa, 101, 32137 }, // NAD83 / Texas North
    { OcdGrid::Usa, 102, 32138 }, // NAD83 / Texas North Central
    { OcdGrid::Usa, 103, 32139 }, // NAD83 / Texas Central
    { OcdGrid::Usa, 104, 32140 }, // NAD83 / Texas South Central
    { OcdGrid::Usa, 105, 32141 }, // NAD83 / Texas South
    { OcdGrid::Usa, 106, 32142 }, // NAD83 / Utah North
    { OcdGrid::Usa, 107, 32143 }, // NAD83 / Utah Central
    { OcdGrid::Usa, 108, 32144 }, // NAD83 / Utah South
    { OcdGrid::Usa, 109, 32145 }, // NAD83 / Vermont
    { OcdGrid::Usa, 110, 32146 }, // NAD83 / Virginia North
    { OcdGrid::Usa, 111, 32147 }, // NAD83 / Virginia South
    { OcdGrid::Usa, 112, 32148 }, // NAD83 / Washington North
    { OcdGrid::Usa, 113, 32149 }, // NAD83 / Washington South
    { OcdGrid::Usa, 114, 32150 }, // NAD83 / West Virginia North
    { OcdGrid::Usa, 115, 32151 }, // NAD83 / West Virginia South
    { OcdGrid::Usa, 116, 32152 }, // NAD83 / Wisconsin North
    { OcdGrid::Usa, 117, 32153 }, // NAD83 / Wisconsin Central
    { OcdGrid::Usa, 118, 32154 }, // NAD83 / Wisconsin South
    { OcdGrid::Usa, 119, 32155 }, // NAD83 / Wyoming East
    { OcdGrid::Usa, 120, 32156 }, // NAD83 / Wyoming East Central
    { OcdGrid::Usa, 121, 32157 }, // NAD83 / Wyoming West Central
    { OcdGrid::Usa, 122, 32158 }, // NAD83 / Wyoming West
    { OcdGrid::Usa, 123, 32158 }, // NAD83 / Wyoming West
    { OcdGrid::UsaFeet, 1, 102629 }, // NAD_1983_StatePlane_Alabama_East_FIPS_0101_Feet
    { OcdGrid::UsaFeet, 2, 102630 }, // NAD_1983_StatePlane_Alabama_West_FIPS_0102_Feet
    { OcdGrid::UsaFeet, 13, 2222 }, // NAD83 / Arizona East (ft)
    { OcdGrid::UsaFeet, 14, 2223 }, // NAD83 / Arizona Central (ft)
    { OcdGrid::UsaFeet, 15, 2224 }, // NAD83 / Arizona West (ft)
    { OcdGrid::UsaFeet, 16, 3433 }, // NAD83 / Arkansas North (ftUS)
    { OcdGrid::UsaFeet, 17, 3434 }, // NAD83 / Arkansas South (ftUS)
    { OcdGrid::UsaFeet, 18, 2225 }, // NAD83 / California zone 1 (ftUS)
    { OcdGrid::UsaFeet, 19, 2226 }, // NAD83 / California zone 2 (ftUS)
    { OcdGrid::UsaFeet, 20, 2227 }, // NAD83 / California zone 3 (ftUS)
    { OcdGrid::UsaFeet, 21, 2228 }, // NAD83 / California zone 4 (ftUS)
    { OcdGrid::UsaFeet, 22, 2229 }, // NAD83 / California zone 5 (ftUS)
    { OcdGrid::UsaFeet, 23, 2230 }, // NAD83 / California zone 6 (ftUS)
    { OcdGrid::UsaFeet, 24, 2231 }, // NAD83 / Colorado North (ftUS)
    { OcdGrid::UsaFeet, 25, 2232 }, // NAD83 / Colorado Central (ftUS)
    { OcdGrid::UsaFeet, 26, 2233 }, // NAD83 / Colorado South (ftUS)
    { OcdGrid::UsaFeet, 27, 2234 }, // NAD83 / Connecticut (ftUS)
    { OcdGrid::UsaFeet, 28, 2235 }, // NAD83 / Delaware (ftUS)
    { OcdGrid::UsaFeet, 29, 2236 }, // NAD83 / Florida East (ftUS)
    { OcdGrid::UsaFeet, 30, 2237 }, // NAD83 / Florida West (ftUS)
    { OcdGrid::UsaFeet, 31, 2238 }, // NAD83 / Florida North (ftUS)
    { OcdGrid::UsaFeet, 32, 2239 }, // NAD83 / Georgia East (ftUS)
    { OcdGrid::UsaFeet, 33, 2240 }, // NAD83 / Georgia West (ftUS)
    { OcdGrid::UsaFeet, 36, 3759 }, // NAD83 / Hawaii zone 3 (ftUS)
    { OcdGrid::UsaFeet, 39, 2241 }, // NAD83 / Idaho East (ftUS)
    { OcdGrid::UsaFeet, 40, 2242 }, // NAD83 / Idaho Central (ftUS)
    { OcdGrid::UsaFeet, 41, 2243 }, // NAD83 / Idaho West (ftUS)
    { OcdGrid::UsaFeet, 42, 3435 }, // NAD83 / Illinois East (ftUS)
    { OcdGrid::UsaFeet, 43, 3436 }, // NAD83 / Illinois West (ftUS)
    { OcdGrid::UsaFeet, 44, 2244 }, //
    { OcdGrid::UsaFeet, 45, 2245 }, //
    { OcdGrid::UsaFeet, 46, 3417 }, // NAD83 / Iowa North (ftUS)
    { OcdGrid::UsaFeet, 47, 3418 }, // NAD83 / Iowa South (ftUS)
    { OcdGrid::UsaFeet, 48, 3419 }, // NAD83 / Kansas North (ftUS)
    { OcdGrid::UsaFeet, 49, 3420 }, // NAD83 / Kansas South (ftUS)
    { OcdGrid::UsaFeet, 50, 2246 }, // NAD83 / Kentucky North (ftUS)
    { OcdGrid::UsaFeet, 51, 2247 }, // NAD83 / Kentucky South (ftUS)
    { OcdGrid::UsaFeet, 52, 3451 }, // NAD83 / Louisiana North (ftUS)
    { OcdGrid::UsaFeet, 53, 3452 }, // NAD83 / Louisiana South (ftUS)
    { OcdGrid::UsaFeet, 54, 3453 }, // NAD83 / Louisiana Offshore (ftUS)
    { OcdGrid::UsaFeet, 55, 26847 }, // NAD83 / Maine East (ftUS)
    { OcdGrid::UsaFeet, 56, 26848 }, // NAD83 / Maine West (ftUS)
    { OcdGrid::UsaFeet, 57, 2248 }, // NAD83 / Maryland (ftUS)
    { OcdGrid::UsaFeet, 58, 2249 }, // NAD83 / Massachusetts Mainland (ftUS)
    { OcdGrid::UsaFeet, 59, 2250 }, // NAD83 / Massachusetts Island (ftUS)
    { OcdGrid::UsaFeet, 60, 2251 }, // NAD83 / Michigan North (ft)
    { OcdGrid::UsaFeet, 61, 2252 }, // NAD83 / Michigan Central (ft)
    { OcdGrid::UsaFeet, 62, 2253 }, // NAD83 / Michigan South (ft)
    { OcdGrid::UsaFeet, 63, 26849 }, // NAD83 / Minnesota North (ftUS)
    { OcdGrid::UsaFeet, 64, 26850 }, // NAD83 / Minnesota Central (ftUS)
    { OcdGrid::UsaFeet, 65, 26851 }, // NAD83 / Minnesota South (ftUS)
    { OcdGrid::UsaFeet, 66, 2254 }, // NAD83 / Mississippi East (ftUS)
    { OcdGrid::UsaFeet, 67, 2255 }, // NAD83 / Mississippi West (ftUS)
    { OcdGrid::UsaFeet, 68, 102696 }, // NAD_1983_StatePlane_Missouri_East_FIPS_2401_Feet
    { OcdGrid::UsaFeet, 69, 102697 }, // NAD_1983_StatePlane_Missouri_Central_FIPS_2402_Feet
    { OcdGrid::UsaFeet, 70, 102698 }, // NAD_1983_StatePlane_Missouri_West_FIPS_2403_Feet
    { OcdGrid::UsaFeet, 71, 2256 }, // NAD83 / Montana (ft)
    { OcdGrid::UsaFeet, 72, 26852 }, // NAD83 / Nebraska (ftUS)
    { OcdGrid::UsaFeet, 73, 3421 }, // NAD83 / Nevada East (ftUS)
    { OcdGrid::UsaFeet, 74, 3422 }, // NAD83 / Nevada Central (ftUS)
    { OcdGrid::UsaFeet, 75, 3423 }, // NAD83 / Nevada West (ftUS)
    { OcdGrid::UsaFeet, 76, 3437 }, // NAD83 / New Hampshire (ftUS)
    { OcdGrid::UsaFeet, 77, 3424 }, // NAD83 / New Jersey (ftUS)
    { OcdGrid::UsaFeet, 78, 2257 }, // NAD83 / New Mexico East (ftUS)
    { OcdGrid::UsaFeet, 79, 2258 }, // NAD83 / New Mexico Central (ftUS)
    { OcdGrid::UsaFeet, 80, 2259 }, // NAD83 / New Mexico West (ftUS)
    { OcdGrid::UsaFeet, 81, 2260 }, // NAD83 / New York East (ftUS)
    { OcdGrid::UsaFeet, 82, 2261 }, // NAD83 / New York Central (ftUS)
    { OcdGrid::UsaFeet, 83, 2262 }, // NAD83 / New York West (ftUS)
    { OcdGrid::UsaFeet, 84, 2263 }, // NAD83 / New York Long Island (ftUS)
    { OcdGrid::UsaFeet, 85, 2264 }, // NAD83 / North Carolina (ftUS)
    { OcdGrid::UsaFeet, 86, 2265 }, // NAD83 / North Dakota North (ft)
    { OcdGrid::UsaFeet, 87, 2266 }, // NAD83 / North Dakota South (ft)
    { OcdGrid::UsaFeet, 88, 3734 }, // NAD83 / Ohio North (ftUS)
    { OcdGrid::UsaFeet, 89, 3735 }, // NAD83 / Ohio South (ftUS)
    { OcdGrid::UsaFeet, 90, 2267 }, // NAD83 / Oklahoma North (ftUS)
    { OcdGrid::UsaFeet, 91, 2268 }, // NAD83 / Oklahoma South (ftUS)
    { OcdGrid::UsaFeet, 92, 2269 }, // NAD83 / Oregon North (ft)
    { OcdGrid::UsaFeet, 93, 2270 }, // NAD83 / Oregon South (ft)
    { OcdGrid::UsaFeet, 94, 2271 }, // NAD83 / Pennsylvania North (ftUS)
    { OcdGrid::UsaFeet, 95, 2272 }, // NAD83 / Pennsylvania South (ftUS)
    { OcdGrid::UsaFeet, 96, 3438 }, // NAD83 / Rhode Island (ftUS)
    { OcdGrid::UsaFeet, 97, 2273 }, // NAD83 / South Carolina (ft)
    { OcdGrid::UsaFeet, 98, 3454 }, //
    { OcdGrid::UsaFeet, 99, 3455 }, // NAD83 / South Dakota South (ftUS)
    { OcdGrid::UsaFeet, 100, 2274 }, // NAD83 / Tennessee (ftUS)
    { OcdGrid::UsaFeet, 101, 2275 }, // NAD83 / Texas North (ftUS)
    { OcdGrid::UsaFeet, 102, 2276 }, // NAD83 / Texas North Central (ftUS)
    { OcdGrid::UsaFeet, 103, 2277 }, // NAD83 / Texas Central (ftUS)
    { OcdGrid::UsaFeet, 104, 2278 }, // NAD83 / Texas South Central (ftUS)
    { OcdGrid::UsaFeet, 105, 2279 }, // NAD83 / Texas South (ftUS)
    { OcdGrid::UsaFeet, 106, 3560 }, // NAD83 / Utah North (ftUS)
    { OcdGrid::UsaFeet, 107, 3566 }, // NAD83 / Utah Central (ftUS)
    { OcdGrid::UsaFeet, 108, 3567 }, // NAD83 / Utah South (ftUS)
    { OcdGrid::UsaFeet, 109, 102745 }, // NAD_1983_StatePlane_Vermont_FIPS_4400_Feet
    { OcdGrid::UsaFeet, 110, 2283 }, // NAD83 / Virginia North (ftUS)
    { OcdGrid::UsaFeet, 111, 2284 }, // NAD83 / Virginia South (ftUS)
    { OcdGrid::UsaFeet, 112, 2285 }, // NAD83 / Washington North (ftUS)
    { OcdGrid::UsaFeet, 113, 2286 }, // NAD83 / Washington South (ftUS)
    { OcdGrid::UsaFeet, 114, 26853 }, // NAD83 / West Virginia North (ftUS)
    { OcdGrid::UsaFeet, 115, 26854 }, // NAD83 / West Virginia South (ftUS)
    { OcdGrid::UsaFeet, 116, 2287 }, // NAD83 / Wisconsin North (ftUS)
    { OcdGrid::UsaFeet, 117, 2288 }, // NAD83 / Wisconsin Central (ftUS)
    { OcdGrid::UsaFeet, 118, 2289 }, // NAD83 / Wisconsin South (ftUS)
    { OcdGrid::UsaFeet, 119, 3736 }, // NAD83 / Wyoming East (ftUS)
    { OcdGrid::UsaFeet, 120, 3737 }, // NAD83 / Wyoming East Central (ftUS)
    { OcdGrid::UsaFeet, 121, 3738 }, // NAD83 / Wyoming West Central (ftUS)
    { OcdGrid::UsaFeet, 122, 3739 }, // NAD83 / Wyoming West (ftUS)
    { OcdGrid::UsaFeet, 123, 102761 }, // NAD_1983_StatePlane_Puerto_Rico_Virgin_Islands_FIPS_5200_Feet
    { OcdGrid::UtmEtrs89, 1, 25828 }, // ETRS89 / UTM zone 28N
    { OcdGrid::UtmEtrs89, 2, 25829 }, // ETRS89 / UTM zone 29N
    { OcdGrid::UtmEtrs89, 3, 25830 }, // ETRS89 / UTM zone 30N
    { OcdGrid::UtmEtrs89, 4, 25831 }, // ETRS89 / UTM zone 31N
    { OcdGrid::UtmEtrs89, 5, 25832 }, // ETRS89 / UTM zone 32N
    { OcdGrid::UtmEtrs89, 6, 25833 }, // ETRS89 / UTM zone 33N
    { OcdGrid::UtmEtrs89, 7, 25834 }, // ETRS89 / UTM zone 34N
    { OcdGrid::UtmEtrs89, 8, 25835 }, // ETRS89 / UTM zone 35N
    { OcdGrid::UtmEtrs89, 9, 25836 }, // ETRS89 / UTM zone 36N
    { OcdGrid::UtmEtrs89, 10, 25837 }, // ETRS89 / UTM zone 37N
    { OcdGrid::UtmEtrs89, 11, 25838 }, // ETRS_1989_UTM_Zone_38N
    { OcdGrid::UtmNad83, 1, 26901 }, // NAD83 / UTM zone 1N
    { OcdGrid::UtmNad83, 2, 26902 }, // NAD83 / UTM zone 2N
    { OcdGrid::UtmNad83, 4, 26903 }, // NAD83 / UTM zone 3N
    { OcdGrid::UtmNad83, 5, 26904 }, // NAD83 / UTM zone 4N
    { OcdGrid::UtmNad83, 6, 26905 }, // NAD83 / UTM zone 5N
    { OcdGrid::UtmNad83, 7, 26906 }, // NAD83 / UTM zone 6N
    { OcdGrid::UtmNad83, 8, 26907 }, // NAD83 / UTM zone 7N
    { OcdGrid::UtmNad83, 9, 26908 }, // NAD83 / UTM zone 8N
    { OcdGrid::UtmNad83, 10, 26909 }, // NAD83 / UTM zone 9N
    { OcdGrid::UtmNad83, 11, 26910 }, // NAD83 / UTM zone 10N
    { OcdGrid::UtmNad83, 12, 26911 }, // NAD83 / UTM zone 11N
    { OcdGrid::UtmNad83, 13, 26912 }, // NAD83 / UTM zone 12N
    { OcdGrid::UtmNad83, 14, 26913 }, // NAD83 / UTM zone 13N
    { OcdGrid::UtmNad83, 15, 26914 }, // NAD83 / UTM zone 14N
    { OcdGrid::UtmNad83, 16, 26915 }, // NAD83 / UTM zone 15N
    { OcdGrid::UtmNad83, 17, 26916 }, // NAD83 / UTM zone 16N
    { OcdGrid::UtmNad83, 18, 26917 }, // NAD83 / UTM zone 17N
    { OcdGrid::UtmNad83, 19, 26918 }, // NAD83 / UTM zone 18N
    { OcdGrid::UtmNad83, 20, 26919 }, // NAD83 / UTM zone 19N
    { OcdGrid::UtmNad83, 21, 26920 }, // NAD83 / UTM zone 20N
    { OcdGrid::UtmNad83, 22, 26921 }, // NAD83 / UTM zone 21N
    { OcdGrid::UtmNad83, 23, 26922 }, // NAD83 / UTM zone 22N
    { OcdGrid::UtmNad83, 24, 26923 }, // NAD83 / UTM zone 23N
    { OcdGrid::Wgs84PseudoMercator, 0, 3857 }, // WGS 84 / Pseudo-Mercator
};

/**
 * Plain translation from OCD grid/zone combination into a Mapper
 * scheme.
 *
 * @see toOcd
 */
std::pair<const MapperCrs, const int> fromOcd(const int combined_ocd_grid_zone)
{
	// while OCD codes can be negative, we are keeping grid_id always positive
	// to match the enums; signum is carried in zone_id
	const int ocd_grid_id = qAbs(combined_ocd_grid_zone / 1000),
	          ocd_zone_id = qAbs(combined_ocd_grid_zone % 1000)
	                        * (combined_ocd_grid_zone < 0 ? -1 : 1);

	switch (static_cast<OcdGrid>(ocd_grid_id))
	{
	case OcdGrid::Local:
		return { MapperCrs::Local, 0 };

	case OcdGrid::Utm:
		if (qAbs(ocd_zone_id) >= 1 && qAbs(ocd_zone_id) <= 60)
			return { MapperCrs::Utm, ocd_zone_id };
		break;

	case OcdGrid::Germany:
		// DHDN, Gauss-Krüger, Zone 2-5
		if (ocd_zone_id >= 2 && ocd_zone_id <= 5)
			return { MapperCrs::GaussKrueger, ocd_zone_id };
		break;

	default:
		for (const auto& record : grid_zone_epsg_codes)
		{
			if (static_cast<int>(record.ocd_grid_id) == ocd_grid_id
			    && record.ocd_zone_id == ocd_zone_id)
				return { MapperCrs::Epsg, record.epsg_code };
		}
	}

	return { MapperCrs::Invalid, 0 };
}

/**
 * Imports string 1039 field i.
 */
void applyGridAndZone(Georeferencing& georef,
                      const int combined_ocd_grid_zone,
                      const std::function<void(const QString&)>& warning_handler)
{
	// Mapper has multiple ways of expressing some of the CRS's
	// we are working with the first match
	auto result = fromOcd(combined_ocd_grid_zone);

	if (result.first == MapperCrs::Invalid)
	{
		warning_handler(::OpenOrienteering::OcdFileImport::tr(
		                    "Could not load the coordinate reference system '%1'.")
		                .arg(combined_ocd_grid_zone));
		return;
	}

	if (result.first == MapperCrs::Local)
		return;

	// look up CRS template by its unique name
	auto entry_it = std::find_if(std::begin(crs_string_map),
	                             std::end(crs_string_map),
	                             [&result](auto e) { return e.key == result.first; });
	if (entry_it == std::end(crs_string_map))
	{
		qWarning("Internal program error: unknown CRS id (%d)", static_cast<int>(result.first));
		return;
	}
	const QString crs_id_string = QLatin1String(entry_it->value);
	auto crs_template = CRSTemplateRegistry().find(crs_id_string);

	if (!crs_template)
	{
		qWarning("Internal program error: unknown CRS description string (%s)", qUtf8Printable(crs_id_string));
		return;
	}

	// get PROJ.4 spec template from the CRS
	auto spec = crs_template->specificationTemplate();

	// we are handling templates with single parameter only get template's first
	// parameter - it can be an instance of UTMZoneParameter, IntRangeParameter
	// (with "factor" and "bias") or FullSpecParameter
	auto param = crs_template->parameters()[0];

	// run every parameter value through the parameter processor which
	// converts it into a string list that can be consumed by the above
	// CRS specification template
	const QString crs_param_string =
	        result.first != MapperCrs::Utm || result.second >= 0
	        ? QString::number(result.second) // common case, plain number
	        : QString::number(-result.second) + QLatin1String(" S"); // UTM South zones

	for (const auto& spec_value : param->specValues(crs_param_string))
		spec = spec.arg(spec_value);

	// `spec` and the pair <`crs_id_string`, `crs_param_string`> both describe
	// the same thing at this place
	georef.setProjectedCRS(crs_id_string, spec, { crs_param_string });
}

/**
 * Combine grid/zone components into OCD code.
 */
int combineGridZone(const OcdGrid grid_id, const int zone_id)
{
	int i = static_cast<int>(grid_id) * 1000 + qAbs(zone_id);
	// propagate signum which we carry in zone_id
	return i * (zone_id < 0 ? -1 : 1);
}

/**
 * Provide an OCD grid/zone combination for a Mapper georeferencing.
 *
 * @see fromOcd
 */
int toOcd(const MapperCrs crs_unique_id,
          const int crs_param)
{
	switch (crs_unique_id)
	{
	case MapperCrs::Local:
		return combineGridZone(OcdGrid::Local, 0);
		break;

	case MapperCrs::Utm:
		if (qAbs(crs_param) >= 1 && qAbs(crs_param) <= 60)
			return combineGridZone(OcdGrid::Utm, crs_param);
		break;

	case MapperCrs::GaussKrueger:
		if (crs_param >= 2 && crs_param <= 5)
			return combineGridZone(OcdGrid::Germany, crs_param);
		break;

	case MapperCrs::Epsg:
		// handle UTM separately, the rest is in the table
		if (crs_param >= 32601 && crs_param <= 32660)
			return combineGridZone(OcdGrid::Utm, crs_param - 32600);
		else if (crs_param >= 32701 && crs_param <= 32760)
			return combineGridZone(OcdGrid::Utm, 32700 - crs_param);

		for (const auto& record : grid_zone_epsg_codes)
		{
			if (record.epsg_code == crs_param)
				return combineGridZone(record.ocd_grid_id, record.ocd_zone_id);
		}
		break;
	default:
		qWarning("Internal program error: translateMapperToOcd() should not"
		         " be fed with invalid values (%d)", static_cast<int>(crs_unique_id));
	}

	return combineGridZone(OcdGrid::Invalid, 0);
}

}  // anonymous namespace

bool operator==(const OcdGeorefFields& lhs, const OcdGeorefFields& rhs)
{
	return lhs.i == rhs.i
	        && lhs.m == rhs.m
	        && lhs.x == rhs.x
	        && lhs.y == rhs.y
	        && ((qIsNaN(lhs.a) && qIsNaN(rhs.a)) || qAbs(lhs.a - rhs.a) < 5e-9) // 8-digit precision or both NaN's
	        && lhs.r == rhs.r;
}

void OcdGeorefFields::setupGeoref(Georeferencing& georef,
                                  const std::function<void (const QString&)>& warning_handler) const
{
	if (m > 0)
		georef.setScaleDenominator(m);

	if (qIsFinite(a) && qAbs(a) >= 0.01)
		georef.setGrivation(a);

	if (r)
		applyGridAndZone(georef, i, warning_handler);

	QPointF proj_ref_point(x, y);
	georef.setProjectedRefPoint(proj_ref_point, false);
}

OcdGeorefFields OcdGeorefFields::fromGeoref(const Georeferencing& georef,
                                            const std::function<void (const QString&)>& warning_handler)
{
	OcdGeorefFields retval;

	// store known values early, they can be useful even if CRS translation fails
	retval.a = georef.getGrivation();
	retval.m = georef.getScaleDenominator();
	const QPointF offset(georef.toProjectedCoords(MapCoord{}));
	retval.x = qRound(offset.x()); // OCD easting and northing is integer
	retval.y = qRound(offset.y());

	// attempt translation from Mapper CRS reference into OCD one
	auto crs_id_string = georef.getProjectedCRSId();
	auto crs_param_strings = georef.getProjectedCRSParameters();
	auto report_warning = [&]() {
		QStringList params;
		std::for_each(begin(crs_param_strings), end(crs_param_strings),
		              [&](const QString& s) { params.append(s); });
		warning_handler(::OpenOrienteering::OcdFileExport::tr(
		                    "Could not translate coordinate reference system '%1:%2'.")
		                .arg(crs_id_string, params.join(QLatin1Char(','))));
	};

	// translate CRS string into a MapperCrs value
	auto entry_it = std::find_if(std::begin(crs_string_map),
	                             std::end(crs_string_map),
	                             [&crs_id_string](auto e) { return QLatin1String(e.value) == crs_id_string; });
	if (entry_it == std::end(crs_string_map))
	{
		report_warning();
		return retval;
	}
	const MapperCrs crs_id = entry_it->key;

	int crs_param = 0;

	if (crs_id != MapperCrs::Local)
	{
		// analyze CRS parameters
		if (crs_param_strings.empty())
		{
			report_warning();
			return retval;
		}

		bool param_conversion_ok;
		auto param_string = crs_param_strings[0];
		switch (crs_id)
		{
		case MapperCrs::Utm:
			if (param_string.endsWith(QLatin1String(" S")))
			{
				param_string.remove(QLatin1String(" S"));
				param_string.prepend(QLatin1String("-"));
			}
			else
			{
				param_string.remove(QLatin1String(" N"));
			}
			// fallthrough

		default:
			crs_param = param_string.toInt(&param_conversion_ok);
		}

		if (!param_conversion_ok)
		{
			report_warning();
			return retval;
		}
	}

	auto combined_ocd_grid_zone = toOcd(crs_id, crs_param);

	if (combined_ocd_grid_zone/1000 == static_cast<int>(OcdGrid::Invalid))
	{
		report_warning();
		return retval;
	}

	retval.i = combined_ocd_grid_zone;
	retval.r = 1; // we are done

	return retval;
}

}  // namespace OpenOrienteering
