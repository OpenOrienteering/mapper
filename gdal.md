---
title: Geospatial data support with GDAL
keywords: GDAL
edited: 21 December 2019
---

OpenOrienteering Mapper uses the [GDAL library](https://gdal.org)
for supporting a broad range of geospatial raster and vector data.
GDAL's vector data support used to be known as OGR,
so you will sometimes find this name, too.

Geospatial raster data, such as GeoTIFF or JPEG2000 files,
can be used as [templates](templates.md) starting with Mapper v0.9.2.
Geospatial vector data, such as Shape, OSM, or DXF files,
may be used as templates, too.
But it also possible to open or import vector data as map data.

In addition, the [File menu](file_menu.md) offers the option to export a map
in selected geospatial vector data formats.
The actual GDAL driver is selected by the extension (e.g. ".dxf")
given to the filename.

When using GDAL formats as templates, some filename extensions may be supported
by multiple providers (plain image, GDAL raster data, GDAL vector data).
In these cases, a secondary provider from GDAL may be selected by prefixing the
regular extension with "raster." or "vector." as listed below.

Please note that some of the raster formats listed below are not used for
images but for actual geospatial metrics meant for algorithmic processing.


## GDAL formats offered by OpenOrienteering Mapper

The following list is valid for the official Android, macOS and Windows releases
as of v0.9.2.
Linux packages are likely to support additional formats,
depending on the configuration of the distribution's GDAL package.

### Raster import drivers

| Driver | Long name  | Extensions |
|--------|------------|------------|
| AAIGrid | [Arc/Info ASCII Grid](https://gdal.org/frmt_various.html#AAIGrid) | asc |
| ACE2 | [ACE2](https://gdal.org/frmt_various.html#ACE2) | ACE2 |
| ADRG | [ARC Digitized Raster Graphics](https://gdal.org/frmt_various.html#ADRG) | gen |
| BIGGIF | [Graphics Interchange Format (.gif)](https://gdal.org/frmt_gif.html) | raster.gif |
| BLX | [Magellan topo (.blx)](https://gdal.org/frmt_various.html#BLX) | blx |
| BMP | [MS Windows Device Independent Bitmap](https://gdal.org/frmt_bmp.html) | raster.bmp |
| BT | [VTP .bt (Binary Terrain) 1.3 Format](https://gdal.org/frmt_various.html#BT) | bt |
| BYN | [Natural Resources Canada's Geoid](https://gdal.org/frmt_byn.html) | byn err |
| CAD | [AutoCAD Driver](https://gdal.org/drv_cad.html) | raster.dwg |
| CALS | [CALS (Type 1)](https://gdal.org/frmt_cals.html) | .cal .ct1 |
| COASP | DRDC COASP SAR Processor Raster | hdr |
| DTED | [DTED Elevation Raster](https://gdal.org/frmt_various.html#DTED) | dt0 dt1 dt2 |
| E00GRID | [Arc/Info Export E00 GRID](https://gdal.org/frmt_various.html#E00GRID) | raster.e00 |
| ECRGTOC | [ECRG TOC format](https://gdal.org/frmt_various.html#ECRGTOC) | raster.xml |
| EHdr | [ESRI .hdr Labelled](https://gdal.org/frmt_various.html#EHdr) | bil |
| ENVI | [ENVI .hdr Labelled](https://gdal.org/frmt_various.html#ENVI) |  |
| ERS | [ERMapper .ers Labelled](https://gdal.org/frmt_ers.html) | ers |
| ESAT | [Envisat Image Format](https://gdal.org/frmt_various.html#Envisat) | n1 |
| FIT | [FIT Image](https://gdal.org/frmt_various.html#) |  |
| GFF | [Ground-based SAR Applications Testbed File Format (.gff)](https://gdal.org/frmt_various.html#GFF) | gff |
| GIF | [Graphics Interchange Format (.gif)](https://gdal.org/frmt_gif.html) | raster.gif |
| GPKG | [GeoPackage](https://gdal.org/drv_geopackage.html) | raster.gpkg |
| GRIB | [GRIdded Binary (.grb, .grb2)](https://gdal.org/frmt_grib.html) | grb grb2 grib2 |
| GS7BG | [Golden Software 7 Binary Grid (.grd)](https://gdal.org/frmt_various.html#GS7BG) | grd |
| GSAG | [Golden Software ASCII Grid (.grd)](https://gdal.org/frmt_various.html#GSAG) | grd |
| GSBG | [Golden Software Binary Grid (.grd)](https://gdal.org/frmt_various.html#GSBG) | grd |
| GTX | NOAA Vertical Datum .GTX | gtx |
| GTiff | [GeoTIFF](https://gdal.org/frmt_gtiff.html) | tif tiff |
| GXF | [GeoSoft Grid Exchange Format](https://gdal.org/frmt_various.html#GXF) | gxf |
| HF2 | [HF2/HFZ heightfield raster](https://gdal.org/frmt_hf2.html) | hf2 |
| HFA | [Erdas Imagine Images (.img)](https://gdal.org/frmt_hfa.html) | img |
| IGNFHeightASCIIGrid | [IGN France height correction ASCII Grid](https://gdal.org/frmt_various.html#IGNFHeightASCIIGrid) | mnt raster.txt gra |
| ILWIS | ILWIS Raster Map | mpr mpl |
| IRIS | [IRIS data (.PPI, .CAPPi etc)](https://gdal.org/frmt_various.html#IRIS) | ppi |
| ISIS3 | [USGS Astrogeology ISIS cube (Version 3)](https://gdal.org/frmt_isis3.html) | lbl cub |
| JDEM | [Japanese DEM (.mem)](https://gdal.org/frmt_various.html#JDEM) | mem |
| JP2OpenJPEG | [JPEG-2000 driver based on OpenJPEG library](https://gdal.org/frmt_jp2openjpeg.html) | jp2 j2k |
| JPEG | [JPEG JFIF](https://gdal.org/frmt_jpeg.html) | raster.jpg raster.jpeg |
| KMLSUPEROVERLAY | Kml Super Overlay | raster.kml kmz |
| KRO | KOLOR Raw | kro |
| LCP | [FARSITE v.4 Landscape File (.lcp)](https://gdal.org/frmt_lcp.html) | lcp |
| Leveller | [Leveller heightfield](https://gdal.org/frmt_leveller.html) | ter |
| MBTiles | [MBTiles](https://gdal.org/frmt_mbtiles.html) | mbtiles |
| MFF | [Vexcel MFF Raster](https://gdal.org/frmt_various.html#MFF) | hdr |
| MRF | [Meta Raster Format](https://gdal.org/frmt_marfa.html) | mrf |
| MSGN | [EUMETSAT Archive native (.nat)](https://gdal.org/frmt_msgn.html) | nat |
| NGSGEOID | [NOAA NGS Geoid Height Grids](https://gdal.org/frmt_ngsgeoid.html) | bin |
| NITF | [National Imagery Transmission Format](https://gdal.org/frmt_nitf.html) | ntf |
| NTv1 | NTv1 Datum Grid Shift | raster.dat |
| NTv2 | NTv2 Datum Grid Shift | gsb |
| NWT_GRC | [Northwood Classified Grid Format .grc/.tab](https://gdal.org/frmt_various.html#northwood_grc) | grc |
| NWT_GRD | [Northwood Numeric Grid Format .grd/.tab](https://gdal.org/frmt_nwtgrd.html) | grd |
| PCIDSK | [PCIDSK Database File](https://gdal.org/frmt_pcidsk.html) | pix |
| PDS4 | [NASA Planetary Data System 4](https://gdal.org/frmt_pds4.html) | raster.xml |
| PNG | [Portable Network Graphics](https://gdal.org/frmt_various.html#PNG) | raster.png |
| PNM | [Portable Pixmap Format (netpbm)](https://gdal.org/frmt_various.html#PNM) | pgm ppm pnm |
| PRF | [Racurs PHOTOMOD PRF](https://gdal.org/frmt_prf.html) | prf |
| R | [R Object Data Store](https://gdal.org/frmt_r.html) | rda |
| RDA | [DigitalGlobe Raster Data Access driver](https://gdal.org/frmt_rda.html) | dgrda |
| RIK | [Swedish Grid RIK (.rik)](https://gdal.org/frmt_various.html#RIK) | rik |
| RMF | [Raster Matrix Format](https://gdal.org/frmt_rmf.html) | rsw |
| RPFTOC | [Raster Product Format TOC format](https://gdal.org/frmt_various.html#RPFTOC) | toc |
| RRASTER | [R Raster](https://gdal.org/frmt_various.html#RRASTER) | grd |
| RST | [Idrisi Raster A.1](https://gdal.org/frmt_Idrisi.html) | rst |
| Rasterlite | [Rasterlite](https://gdal.org/frmt_rasterlite.html) | raster.sqlite |
| SAGA | [SAGA GIS Binary Grid (.sdat, .sg-grd-z)](https://gdal.org/frmt_various.html#SAGA) | sdat sg-grd-z |
| SDTS | [SDTS Raster](https://gdal.org/frmt_various.html#SDTS) | ddf |
| SGI | [SGI Image File Format 1.0](https://gdal.org/frmt_various.html#SGI) | rgb |
| SIGDEM | [Scaled Integer Gridded DEM .sigdem](https://gdal.org/frmt_various.html#SIGDEM) | sigdem |
| SNODAS | [Snow Data Assimilation System](https://gdal.org/frmt_various.html#SNODAS) | hdr |
| SRP | [Standard Raster Product (ASRP/USRP)](https://gdal.org/frmt_various.html#SRP) | img |
| SRTMHGT | [SRTMHGT File Format](https://gdal.org/frmt_various.html#SRTMHGT) | hgt |
| Terragen | [Terragen heightfield](https://gdal.org/frmt_terragen.html) | ter |
| USGSDEM | [USGS Optional ASCII DEM (and CDED)](https://gdal.org/frmt_usgsdem.html) | dem |
| VRT | [Virtual Raster](https://gdal.org/gdal_vrttut.html) | raster.vrt |
| XPM | [X11 PixMap Format](https://gdal.org/frmt_various.html#XPM) | xpm |
| XYZ | [ASCII Gridded XYZ](https://gdal.org/frmt_xyz.html) | xyz |
| ZMap | [ZMap Plus Grid](https://gdal.org/frmt_various.html#ZMap) | raster.dat |
{: .no_spell_check }

### Vector import drivers

| Driver | Long name  | Extensions |
|--------|------------|------------|
| AVCE00 | [Arc/Info E00 (ASCII) Coverage](https://gdal.org/drv_avce00.html) | e00 |
| BNA | [Atlas BNA](https://gdal.org/drv_bna.html) | bna |
| CAD | [AutoCAD Driver](https://gdal.org/drv_cad.html) | dwg |
| CSV | [Comma Separated Value (.csv)](https://gdal.org/drv_csv.html) | csv |
| DGN | [Microstation DGN](https://gdal.org/drv_dgn.html) | dgn |
| DXF | [AutoCAD DXF](https://gdal.org/drv_dxf.html) | dxf |
| EDIGEO | [French EDIGEO exchange format](https://gdal.org/drv_edigeo.html) | thf |
| ESRI Shapefile | [ESRI Shapefile](https://gdal.org/drv_shape.html) | shp dbf |
| ESRIJSON | [ESRIJSON](https://gdal.org/drv_esrijson.html) | json |
| GML | [Geography Markup Language (GML)](https://gdal.org/drv_gml.html) | gml xml |
| GPKG | [GeoPackage](https://gdal.org/drv_geopackage.html) | gpkg |
| GPSTrackMaker | [GPSTrackMaker](https://gdal.org/drv_gtm.html) | gtm gtz |
| GPX | [GPX](https://gdal.org/drv_gpx.html) | gpx |
| GeoJSON | [GeoJSON](https://gdal.org/drv_geojson.html) | json geojson |
| GeoJSONSeq | [GeoJSON Sequence](https://gdal.org/drv_geojsonseq.html) | geojsonl geojsons |
| Geoconcept | Geoconcept | gxt txt |
| Idrisi | Idrisi Vector (.vct) | vct |
| JML | [OpenJUMP JML](https://gdal.org/drv_jml.html) | jml |
| JP2OpenJPEG | [JPEG-2000 driver based on OpenJPEG library](https://gdal.org/frmt_jp2openjpeg.html) | vector.jp2 vector.j2k |
| KML | [Keyhole Markup Language (KML)](https://gdal.org/drv_kml.html) | kml |
| MBTiles | [MBTiles](https://gdal.org/frmt_mbtiles.html) | vector.mbtiles |
| MVT | [Mapbox Vector Tiles](https://gdal.org/drv_mvt.html) | mvt mvt.gz pbf |
| MapInfo File | [MapInfo File](https://gdal.org/drv_mitab.html) | tab mif mid |
| ODS | [Open Document/ LibreOffice / OpenOffice Spreadsheet ](https://gdal.org/drv_ods.html) | ods |
| OGR_GMT | [GMT ASCII Vectors (.gmt)](https://gdal.org/drv_gmt.html) | gmt |
| OGR_VRT | [VRT - Virtual Datasource](https://gdal.org/drv_vrt.html) | vrt |
| OSM | [OpenStreetMap XML and PBF](https://gdal.org/drv_osm.html) | osm pbf |
| OpenFileGDB | [ESRI FileGDB](https://gdal.org/drv_openfilegdb.html) | gdb |
| PCIDSK | [PCIDSK Database File](https://gdal.org/frmt_pcidsk.html) | vector.pix |
| PDS4 | [NASA Planetary Data System 4](https://gdal.org/frmt_pds4.html) | xml |
| REC | EPIInfo .REC  | rec |
| S57 | [IHO S-57 (ENC)](https://gdal.org/drv_s57.html) | 000 |
| SQLite | [SQLite / Spatialite](https://gdal.org/drv_sqlite.html) | sqlite db |
| SVG | [Scalable Vector Graphics](https://gdal.org/drv_svg.html) | svg |
| SXF | [Storage and eXchange Format](https://gdal.org/drv_sxf.html) | sxf |
| TopoJSON | [TopoJSON](https://gdal.org/drv_topojson.html) | json topojson |
| VDV | [VDV-451/VDV-452/INTREST Data Format](https://gdal.org/drv_vdv.html) | txt x10 |
| VFK | [Czech Cadastral Exchange Data Format](https://gdal.org/drv_vfk.html) | vfk |
| WAsP | [WAsP .map format](https://gdal.org/drv_wasp.html) | map |
| XLSX | [MS Office Open XML spreadsheet](https://gdal.org/drv_xlsx.html) | xlsx xlsm |
| XPlane | [X-Plane/Flightgear aeronautical data](https://gdal.org/drv_xplane.html) | dat |
{: .no_spell_check }

### Vector export drivers

| Driver | Long name  | Extensions |
|--------|------------|------------|
| BNA | [Atlas BNA](https://gdal.org/drv_bna.html) | bna |
| CSV | [Comma Separated Value (.csv)](https://gdal.org/drv_csv.html) | csv |
| DGN | [Microstation DGN](https://gdal.org/drv_dgn.html) | dgn |
| DXF | [AutoCAD DXF](https://gdal.org/drv_dxf.html) | dxf |
| ESRI Shapefile | [ESRI Shapefile](https://gdal.org/drv_shape.html) | shp dbf |
| GML | [Geography Markup Language (GML)](https://gdal.org/drv_gml.html) | gml xml |
| GPKG | [GeoPackage](https://gdal.org/drv_geopackage.html) | gpkg |
| GPSTrackMaker | [GPSTrackMaker](https://gdal.org/drv_gtm.html) | gtm gtz |
| GPX | [GPX](https://gdal.org/drv_gpx.html) | gpx |
| GeoJSON | [GeoJSON](https://gdal.org/drv_geojson.html) | json geojson |
| GeoJSONSeq | [GeoJSON Sequence](https://gdal.org/drv_geojsonseq.html) | geojsonl geojsons |
| Geoconcept | Geoconcept | gxt txt |
| JML | [OpenJUMP JML](https://gdal.org/drv_jml.html) | jml |
| KML | [Keyhole Markup Language (KML)](https://gdal.org/drv_kml.html) | kml |
| MBTiles | [MBTiles](https://gdal.org/frmt_mbtiles.html) | mbtiles |
| MapInfo File | [MapInfo File](https://gdal.org/drv_mitab.html) | tab mif mid |
| ODS | [Open Document/ LibreOffice / OpenOffice Spreadsheet ](https://gdal.org/drv_ods.html) | ods |
| OGR_GMT | [GMT ASCII Vectors (.gmt)](https://gdal.org/drv_gmt.html) | gmt |
| PCIDSK | [PCIDSK Database File](https://gdal.org/frmt_pcidsk.html) | pix |
| PDF | [Geospatial PDF](https://gdal.org/frmt_pdf.html) | pdf |
| PDS4 | [NASA Planetary Data System 4](https://gdal.org/frmt_pds4.html) | xml |
| PGDUMP | [PostgreSQL SQL dump](https://gdal.org/drv_pgdump.html) | sql |
| S57 | [IHO S-57 (ENC)](https://gdal.org/drv_s57.html) | 000 |
| SQLite | [SQLite / Spatialite](https://gdal.org/drv_sqlite.html) | sqlite db |
| VDV | [VDV-451/VDV-452/INTREST Data Format](https://gdal.org/drv_vdv.html) | txt x10 |
| WAsP | [WAsP .map format](https://gdal.org/drv_wasp.html) | map |
| XLSX | [MS Office Open XML spreadsheet](https://gdal.org/drv_xlsx.html) | xlsx xlsm |
{: .no_spell_check }

