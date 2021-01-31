---
title: Geospatial data support with GDAL
keywords: GDAL, Templates
parent: Templates and Data
last_modified_date: 24 December 2020
nav_order: 0.5
---

OpenOrienteering Mapper uses the [GDAL library](https://gdal.org)
for supporting a broad range of geospatial
raster data, such as GeoTIFF or JPEG2000 files,
vector data, such as Shape, OSM, or DXF files,
and hybrid formats such as PDF.
GDAL's vector data support used to be known as OGR,
so you will sometimes find this name, too.

Geospatial raster data and vector data can be used as templates.
However, currently only graphical templates can be displayed;
Mapper does not yet process raw raster data, such as altitudes,
which first need to be rendered in other software.
It also possible to open vector data files as maps
and to import vector data into map files.

In addition, the [File menu](file_menu.md) offers the option to export a map
in selected geospatial vector data formats.
The actual GDAL driver is selected by the extension (e.g. ".dxf")
given to the filename.

When using GDAL formats as templates, some filename extensions may be supported
by multiple providers (plain image, GDAL raster data, GDAL vector data).

Please note that some of the raster formats listed below are not used for
images but for actual geospatial metrics meant for algorithmic processing.


## GDAL drivers offered by OpenOrienteering Mapper

The following list is valid for the official Android, macOS and Windows releases
as of v0.9.4.
Linux packages are likely to support a slightly different set of formats,
depending on the configuration of the distribution's GDAL package.

### Raster import drivers

| Driver | Long name  | Extensions |
|--------|------------|------------|
| AAIGrid | [Arc/Info ASCII Grid](https://gdal.org/drivers/raster/aaigrid.html) | asc |
| ACE2 | [ACE2](https://gdal.org/drivers/raster/ace2.html) | ACE2 |
| ADRG | [ARC Digitized Raster Graphics](https://gdal.org/drivers/raster/adrg.html) | gen |
| AIG | [Arc/Info Binary Grid](https://gdal.org/drivers/raster/aig.html) |  |
| ARG | [Azavea Raster Grid format](https://gdal.org/drivers/raster/arg.html) |  |
| AirSAR | [AirSAR Polarimetric Image](https://gdal.org/drivers/raster/airsar.html) |  |
| BIGGIF | [Graphics Interchange Format (.gif)](https://gdal.org/drivers/raster/gif.html) | raster.gif |
| BLX | [Magellan topo (.blx)](https://gdal.org/drivers/raster/blx.html) | blx |
| BMP | [MS Windows Device Independent Bitmap](https://gdal.org/drivers/raster/bmp.html) | raster.bmp |
| BSB | [Maptech BSB Nautical Charts](https://gdal.org/drivers/raster/bsb.html) |  |
| BT | [VTP .bt (Binary Terrain) 1.3 Format](https://gdal.org/drivers/raster/bt.html) | bt |
| BYN | [Natural Resources Canada's Geoid](https://gdal.org/drivers/raster/byn.html) | byn err |
| CAD | [AutoCAD Driver](https://gdal.org/drv_cad.html) | dwg |
| CALS | [CALS (Type 1)](https://gdal.org/drivers/raster/cals.html) | cal ct1 |
| CEOS | [CEOS Image](https://gdal.org/drivers/raster/ceos.html) |  |
| COASP | [DRDC COASP SAR Processor Raster](https://gdal.org/drivers/raster/coasp.html) | hdr |
| COSAR | [COSAR Annotated Binary Matrix (TerraSAR-X)](https://gdal.org/drivers/raster/cosar.html) |  |
| CPG | Convair PolGASP |  |
| CTG | [USGS LULC Composite Theme Grid](https://gdal.org/drivers/raster/ctg.html) |  |
| CTable2 | CTable2 Datum Grid Shift |  |
| DAAS | [Airbus DS Intelligence Data As A Service driver](https://gdal.org/drivers/raster/daas.html) |  |
| DERIVED | [Derived datasets using VRT pixel functions](https://gdal.org/drivers/raster/derived.html) |  |
| DIMAP | [SPOT DIMAP](https://gdal.org/drivers/raster/dimap.html) |  |
| DIPEx | DIPEx |  |
| DOQ1 | [USGS DOQ (Old Style)](https://gdal.org/drivers/raster/doq1.html) |  |
| DOQ2 | [USGS DOQ (New Style)](https://gdal.org/drivers/raster/doq2.html) |  |
| DTED | [DTED Elevation Raster](https://gdal.org/drivers/raster/dted.html) | dt0 dt1 dt2 |
| E00GRID | [Arc/Info Export E00 GRID](https://gdal.org/drivers/raster/e00grid.html) | e00 |
| ECRGTOC | [ECRG TOC format](https://gdal.org/drivers/raster/ecrgtoc.html) | xml |
| EEDAI | [Earth Engine Data API Image](https://gdal.org/drivers/raster/eedai.html) |  |
| EHdr | [ESRI .hdr Labelled](https://gdal.org/drivers/raster/ehdr.html) | bil |
| EIR | [Erdas Imagine Raw](https://gdal.org/drivers/raster/eir.html) |  |
| ELAS | ELAS |  |
| ENVI | [ENVI .hdr Labelled](https://gdal.org/drivers/raster/envi.html) |  |
| ERS | [ERMapper .ers Labelled](https://gdal.org/drivers/raster/ers.html) | ers |
| ESAT | [Envisat Image Format](https://gdal.org/drivers/raster/esat.html) | n1 |
| FAST | [EOSAT FAST Format](https://gdal.org/drivers/raster/fast.html) |  |
| FIT | [FIT Image](https://gdal.org/drivers/raster/fit.html) |  |
| FujiBAS | [Fuji BAS Scanner Image](https://gdal.org/drivers/raster/fujibas.html) |  |
| GFF | [Ground-based SAR Applications Testbed File Format (.gff)](https://gdal.org/drivers/raster/gff.html) | gff |
| GIF | [Graphics Interchange Format (.gif)](https://gdal.org/drivers/raster/gif.html) | raster.gif |
| GPKG | [GeoPackage](https://gdal.org/drv_geopackage.html) | gpkg |
| GRASSASCIIGrid | [GRASS ASCII Grid](https://gdal.org/drivers/raster/grassasciigrid.html) |  |
| GRIB | [GRIdded Binary (.grb, .grb2)](https://gdal.org/drivers/raster/grib.html) | grb grb2 grib2 |
| GS7BG | [Golden Software 7 Binary Grid (.grd)](https://gdal.org/drivers/raster/gs7bg.html) | grd |
| GSAG | [Golden Software ASCII Grid (.grd)](https://gdal.org/drivers/raster/gsag.html) | grd |
| GSBG | [Golden Software Binary Grid (.grd)](https://gdal.org/drivers/raster/gsbg.html) | grd |
| GSC | [GSC Geogrid](https://gdal.org/drivers/raster/gsc.html) |  |
| GTX | NOAA Vertical Datum .GTX | gtx |
| GTiff | [GeoTIFF](https://gdal.org/drivers/raster/gtiff.html) | tif tiff |
| GXF | [GeoSoft Grid Exchange Format](https://gdal.org/drivers/raster/gxf.html) | gxf |
| GenBin | [Generic Binary (.hdr Labelled)](https://gdal.org/drivers/raster/genbin.html) |  |
| HF2 | [HF2/HFZ heightfield raster](https://gdal.org/drivers/raster/hf2.html) | hf2 |
| HFA | [Erdas Imagine Images (.img)](https://gdal.org/drivers/raster/hfa.html) | img |
| IDA | [Image Data and Analysis](https://gdal.org/drivers/raster/ida.html) |  |
| IGNFHeightASCIIGrid | [IGN France height correction ASCII Grid](https://gdal.org/drivers/raster/ignfheightasciigrid.html) | mnt txt gra |
| ILWIS | ILWIS Raster Map | mpr mpl |
| INGR | [Intergraph Raster](https://gdal.org/drivers/raster/intergraphraster.html) |  |
| IRIS | [IRIS data (.PPI, .CAPPi etc)](https://gdal.org/drivers/raster/iris.html) | ppi |
| ISCE | [ISCE raster](https://gdal.org/drivers/raster/isce.html) |  |
| ISG | [International Service for the Geoid](https://gdal.org/drivers/raster/isg.html) | isg |
| ISIS2 | [USGS Astrogeology ISIS cube (Version 2)](https://gdal.org/drivers/raster/isis2.html) |  |
| ISIS3 | [USGS Astrogeology ISIS cube (Version 3)](https://gdal.org/drivers/raster/isis3.html) | lbl cub |
| JAXAPALSAR | [JAXA PALSAR Product Reader (Level 1.1/1.5)](https://gdal.org/drivers/raster/palsar.html) |  |
| JDEM | [Japanese DEM (.mem)](https://gdal.org/drivers/raster/jdem.html) | mem |
| JP2OpenJPEG | [JPEG-2000 driver based on OpenJPEG library](https://gdal.org/drivers/raster/jp2openjpeg.html) | jp2 j2k |
| JPEG | [JPEG JFIF](https://gdal.org/drivers/raster/jpeg.html) | raster.jpg raster.jpeg |
| KMLSUPEROVERLAY | Kml Super Overlay | kml kmz |
| KRO | KOLOR Raw | kro |
| L1B | [NOAA Polar Orbiter Level 1b Data Set](https://gdal.org/drivers/raster/l1b.html) |  |
| LAN | [Erdas .LAN/.GIS](https://gdal.org/drivers/raster/lan.html) |  |
| LCP | [FARSITE v.4 Landscape File (.lcp)](https://gdal.org/drivers/raster/lcp.html) | lcp |
| LOSLAS | NADCON .los/.las Datum Grid Shift |  |
| Leveller | [Leveller heightfield](https://gdal.org/drivers/raster/leveller.html) | ter |
| MAP | [OziExplorer .MAP](https://gdal.org/drivers/raster/map.html) |  |
| MBTiles | [MBTiles](https://gdal.org/drivers/raster/mbtiles.html) | mbtiles |
| MFF | [Vexcel MFF Raster](https://gdal.org/drivers/raster/mff.html) | hdr |
| MFF2 | [Vexcel MFF2 (HKV) Raster](https://gdal.org/drivers/raster/mff2.html) |  |
| MRF | [Meta Raster Format](https://gdal.org/drivers/raster/marfa.html) | mrf |
| MSGN | [EUMETSAT Archive native (.nat)](https://gdal.org/drivers/raster/msgn.html) | nat |
| NDF | [NLAPS Data Format](https://gdal.org/drivers/raster/ndf.html) |  |
| NGSGEOID | [NOAA NGS Geoid Height Grids](https://gdal.org/drivers/raster/ngsgeoid.html) | bin |
| NGW | [NextGIS Web](https://gdal.org/drv_ngw.html) |  |
| NITF | [National Imagery Transmission Format](https://gdal.org/drivers/raster/nitf.html) | ntf |
| NTv1 | NTv1 Datum Grid Shift | dat |
| NTv2 | NTv2 Datum Grid Shift | gsb gvb |
| NWT_GRC | [Northwood Classified Grid Format .grc/.tab](https://gdal.org/drivers/raster/nwtgrd.html#driver-capabilities-nwt-grc) | grc |
| NWT_GRD | [Northwood Numeric Grid Format .grd/.tab](https://gdal.org/drivers/raster/nwtgrd.html) | grd |
| OZI | [OziExplorer Image File](https://gdal.org/drivers/raster/ozi.html) |  |
| PAux | [PCI .aux Labelled](https://gdal.org/drivers/raster/paux.html) |  |
| PCIDSK | [PCIDSK Database File](https://gdal.org/drivers/raster/pcidsk.html) | pix |
| PDF | [Geospatial PDF](https://gdal.org/drivers/raster/pdf.html) | pdf |
| PDS | [NASA Planetary Data System](https://gdal.org/drivers/raster/pds.html) |  |
| PDS4 | [NASA Planetary Data System 4](https://gdal.org/drivers/raster/pds4.html) | xml |
| PLMOSAIC | [Planet Labs Mosaics API](https://gdal.org/drivers/raster/plmosaic.html) |  |
| PLSCENES | [Planet Labs Scenes API](https://gdal.org/drv_plscenes.html) |  |
| PNG | [Portable Network Graphics](https://gdal.org/drivers/raster/png.html) | raster.png |
| PNM | [Portable Pixmap Format (netpbm)](https://gdal.org/drivers/raster/pnm.html) | pgm ppm pnm |
| PRF | [Racurs PHOTOMOD PRF](https://gdal.org/drivers/raster/prf.html) | prf |
| R | [R Object Data Store](https://gdal.org/drivers/raster/r.html) | rda |
| RDA | [DigitalGlobe Raster Data Access driver](https://gdal.org/drivers/raster/rda.html) | dgrda |
| RIK | [Swedish Grid RIK (.rik)](https://gdal.org/drivers/raster/rik.html) | rik |
| RMF | [Raster Matrix Format](https://gdal.org/drivers/raster/rmf.html) | rsw |
| ROI_PAC | [ROI_PAC raster](https://gdal.org/drivers/raster/roi_pac.html) |  |
| RPFTOC | [Raster Product Format TOC format](https://gdal.org/drivers/raster/rpftoc.html) | toc |
| RRASTER | [R Raster](https://gdal.org/drivers/raster/rraster.html) | grd |
| RS2 | [RadarSat 2 XML Product](https://gdal.org/drivers/raster/rs2.html) |  |
| RST | [Idrisi Raster A.1](https://gdal.org/drivers/raster/Idrisi.html) | rst |
| Rasterlite | [Rasterlite](https://gdal.org/drivers/raster/rasterlite.html) | sqlite |
| SAFE | [Sentinel-1 SAR SAFE Product](https://gdal.org/drivers/raster/safe.html) |  |
| SAGA | [SAGA GIS Binary Grid (.sdat, .sg-grd-z)](https://gdal.org/drivers/raster/sdat.html) | sdat sg-grd-z |
| SAR_CEOS | [CEOS SAR Image](https://gdal.org/drivers/raster/sar_ceos.html) |  |
| SDTS | [SDTS Raster](https://gdal.org/drivers/raster/sdts.html) | ddf |
| SENTINEL2 | [Sentinel 2](https://gdal.org/drivers/raster/sentinel2.html) |  |
| SGI | [SGI Image File Format 1.0](https://gdal.org/drivers/raster/sgi.html) | rgb |
| SIGDEM | [Scaled Integer Gridded DEM .sigdem](https://gdal.org/drivers/raster/sigdem.html) | sigdem |
| SNODAS | [Snow Data Assimilation System](https://gdal.org/drivers/raster/snodas.html) | hdr |
| SRP | [Standard Raster Product (ASRP/USRP)](https://gdal.org/drivers/raster/srp.html) | img |
| SRTMHGT | [SRTMHGT File Format](https://gdal.org/drivers/raster/srtmhgt.html) | hgt |
| TIL | [EarthWatch .TIL](https://gdal.org/drivers/raster/til.html) |  |
| TSX | [TerraSAR-X Product](https://gdal.org/drivers/raster/tsx.html) |  |
| Terragen | [Terragen heightfield](https://gdal.org/drivers/raster/terragen.html) | ter |
| USGSDEM | [USGS Optional ASCII DEM (and CDED)](https://gdal.org/drivers/raster/usgsdem.html) | dem |
| VICAR | [MIPL VICAR file](https://gdal.org/drivers/raster/vicar.html) |  |
| VRT | [Virtual Raster](https://gdal.org/drivers/raster/vrt.html) | vrt |
| WCS | [OGC Web Coverage Service](https://gdal.org/drivers/raster/wcs.html) |  |
| WEBP | [WEBP](https://gdal.org/drivers/raster/webp.html) | webp |
| WMS | [OGC Web Map Service](https://gdal.org/drivers/raster/wms.html) |  |
| WMTS | [OGC Web Map Tile Service](https://gdal.org/drivers/raster/wmts.html) |  |
| XPM | [X11 PixMap Format](https://gdal.org/drivers/raster/xpm.html) | xpm |
| XYZ | [ASCII Gridded XYZ](https://gdal.org/drivers/raster/xyz.html) | xyz |
| ZMap | [ZMap Plus Grid](https://gdal.org/drivers/raster/zmap.html) | dat |
{: .no_spell_check }

### Vector import drivers

| Driver | Long name  | Extensions |
|--------|------------|------------|
| ARCGEN | [Arc/Info Generate](https://gdal.org/drv_arcgen.html) |  |
| AVCBin | [Arc/Info Binary Coverage](https://gdal.org/drv_avcbin.html) |  |
| AVCE00 | [Arc/Info E00 (ASCII) Coverage](https://gdal.org/drv_avce00.html) | e00 |
| AeronavFAA | [Aeronav FAA](https://gdal.org/drv_aeronavfaa.html) |  |
| AmigoCloud | [AmigoCloud](https://gdal.org/drv_amigocloud.html) |  |
| BNA | [Atlas BNA](https://gdal.org/drv_bna.html) | bna |
| CAD | [AutoCAD Driver](https://gdal.org/drv_cad.html) | dwg |
| CSV | [Comma Separated Value (.csv)](https://gdal.org/drv_csv.html) | csv |
| CSW | [OGC CSW (Catalog  Service for the Web)](https://gdal.org/drv_csw.html) |  |
| Carto | [Carto](https://gdal.org/drv_carto.html) |  |
| Cloudant | [Cloudant / CouchDB](https://gdal.org/drv_cloudant.html) |  |
| CouchDB | [CouchDB / GeoCouch](https://gdal.org/drv_couchdb.html) |  |
| DGN | [Microstation DGN](https://gdal.org/drv_dgn.html) | dgn |
| DXF | [AutoCAD DXF](https://gdal.org/drv_dxf.html) | dxf |
| EDIGEO | [French EDIGEO exchange format](https://gdal.org/drv_edigeo.html) | thf |
| EEDA | [Earth Engine Data API](https://gdal.org/drivers/vector/eeda.html) |  |
| ESRI Shapefile | [ESRI Shapefile](https://gdal.org/drv_shape.html) | shp dbf shz shp.zip |
| ESRIJSON | [ESRIJSON](https://gdal.org/drv_esrijson.html) | json |
| Elasticsearch | [Elastic Search](https://gdal.org/drv_elasticsearch.html) |  |
| FlatGeobuf | [FlatGeobuf](https://gdal.org/drivers/vector/flatgeobuf.html) | fgb |
| GML | [Geography Markup Language (GML)](https://gdal.org/drv_gml.html) | gml xml |
| GPKG | [GeoPackage](https://gdal.org/drv_geopackage.html) | gpkg |
| GPSBabel | [GPSBabel](https://gdal.org/drv_gpsbabel.html) |  |
| GPSTrackMaker | [GPSTrackMaker](https://gdal.org/drv_gtm.html) | gtm gtz |
| GPX | [GPX](https://gdal.org/drv_gpx.html) | gpx |
| GeoJSON | [GeoJSON](https://gdal.org/drv_geojson.html) | json geojson |
| GeoJSONSeq | [GeoJSON Sequence](https://gdal.org/drv_geojsonseq.html) | geojsonl geojsons |
| GeoRSS | [GeoRSS](https://gdal.org/drv_georss.html) |  |
| Geoconcept | Geoconcept | gxt txt |
| HTF | [Hydrographic Transfer Vector](https://gdal.org/drv_htf.html) |  |
| Idrisi | Idrisi Vector (.vct) | vct |
| JML | [OpenJUMP JML](https://gdal.org/drv_jml.html) | jml |
| JP2OpenJPEG | [JPEG-2000 driver based on OpenJPEG library](https://gdal.org/drivers/raster/jp2openjpeg.html) | jp2 j2k |
| KML | [Keyhole Markup Language (KML)](https://gdal.org/drv_kml.html) | kml |
| MBTiles | [MBTiles](https://gdal.org/drivers/raster/mbtiles.html) | mbtiles |
| MVT | [Mapbox Vector Tiles](https://gdal.org/drv_mvt.html) | mvt mvt.gz pbf |
| MapInfo File | [MapInfo File](https://gdal.org/drv_mitab.html) | tab mif mid |
| MapML | [MapML](https://gdal.org/drivers/vector/mapml.html) |  |
| NGW | [NextGIS Web](https://gdal.org/drv_ngw.html) |  |
| OAPIF | [OGC API - Features](https://gdal.org/drivers/vector/oapif.html) |  |
| ODS | [Open Document/ LibreOffice / OpenOffice Spreadsheet ](https://gdal.org/drv_ods.html) | ods |
| OGR_GMT | [GMT ASCII Vectors (.gmt)](https://gdal.org/drv_gmt.html) | gmt |
| OGR_PDS | [Planetary Data Systems TABLE](https://gdal.org/drv_pds.html) |  |
| OGR_SDTS | [SDTS](https://gdal.org/drv_sdts.html) |  |
| OGR_VRT | [VRT - Virtual Datasource](https://gdal.org/drv_vrt.html) | vrt |
| OSM | [OpenStreetMap XML and PBF](https://gdal.org/drv_osm.html) | osm pbf |
| OpenAir | [OpenAir](https://gdal.org/drv_openair.html) |  |
| OpenFileGDB | [ESRI FileGDB](https://gdal.org/drv_openfilegdb.html) | gdb |
| PCIDSK | [PCIDSK Database File](https://gdal.org/drivers/raster/pcidsk.html) | pix |
| PDF | [Geospatial PDF](https://gdal.org/drivers/raster/pdf.html) | pdf |
| PDS4 | [NASA Planetary Data System 4](https://gdal.org/drivers/raster/pds4.html) | xml |
| PLSCENES | [Planet Labs Scenes API](https://gdal.org/drv_plscenes.html) |  |
| REC | EPIInfo .REC  | rec |
| S57 | [IHO S-57 (ENC)](https://gdal.org/drv_s57.html) | 000 |
| SEGUKOOA | [SEG-P1 / UKOOA P1/90](https://gdal.org/drv_segukooa.html) |  |
| SEGY | [SEG-Y](https://gdal.org/drv_segy.html) |  |
| SQLite | [SQLite / Spatialite](https://gdal.org/drv_sqlite.html) | sqlite db |
| SUA | [Tim Newport-Peace's Special Use Airspace Format](https://gdal.org/drv_sua.html) |  |
| SVG | [Scalable Vector Graphics](https://gdal.org/drv_svg.html) | svg |
| SXF | [Storage and eXchange Format](https://gdal.org/drv_sxf.html) | sxf |
| Selafin | [Selafin](https://gdal.org/drv_selafin.html) |  |
| TIGER | [U.S. Census TIGER/Line](https://gdal.org/drv_tiger.html) |  |
| TopoJSON | [TopoJSON](https://gdal.org/drv_topojson.html) | json topojson |
| UK .NTF | [UK .NTF](https://gdal.org/drv_ntf.html) |  |
| VDV | [VDV-451/VDV-452/INTREST Data Format](https://gdal.org/drv_vdv.html) | txt x10 |
| VFK | [Czech Cadastral Exchange Data Format](https://gdal.org/drv_vfk.html) | vfk |
| VICAR | [MIPL VICAR file](https://gdal.org/drivers/raster/vicar.html) |  |
| WAsP | [WAsP .map format](https://gdal.org/drv_wasp.html) | map |
| WFS | [OGC WFS (Web Feature Service)](https://gdal.org/drv_wfs.html) |  |
| XLSX | [MS Office Open XML spreadsheet](https://gdal.org/drv_xlsx.html) | xlsx xlsm |
| XPlane | [X-Plane/Flightgear aeronautical data](https://gdal.org/drv_xplane.html) | dat |
{: .no_spell_check }

### Vector export drivers

| Driver | Long name  | Extensions |
|--------|------------|------------|
| AmigoCloud | [AmigoCloud](https://gdal.org/drv_amigocloud.html) |  |
| BNA | [Atlas BNA](https://gdal.org/drv_bna.html) | bna |
| CSV | [Comma Separated Value (.csv)](https://gdal.org/drv_csv.html) | csv |
| Carto | [Carto](https://gdal.org/drv_carto.html) |  |
| Cloudant | [Cloudant / CouchDB](https://gdal.org/drv_cloudant.html) |  |
| CouchDB | [CouchDB / GeoCouch](https://gdal.org/drv_couchdb.html) |  |
| DGN | [Microstation DGN](https://gdal.org/drv_dgn.html) | dgn |
| DXF | [AutoCAD DXF](https://gdal.org/drv_dxf.html) | dxf |
| ESRI Shapefile | [ESRI Shapefile](https://gdal.org/drv_shape.html) | shp dbf shz shp.zip |
| Elasticsearch | [Elastic Search](https://gdal.org/drv_elasticsearch.html) |  |
| FlatGeobuf | [FlatGeobuf](https://gdal.org/drivers/vector/flatgeobuf.html) | fgb |
| GML | [Geography Markup Language (GML)](https://gdal.org/drv_gml.html) | gml xml |
| GPKG | [GeoPackage](https://gdal.org/drv_geopackage.html) | gpkg |
| GPSBabel | [GPSBabel](https://gdal.org/drv_gpsbabel.html) |  |
| GPSTrackMaker | [GPSTrackMaker](https://gdal.org/drv_gtm.html) | gtm gtz |
| GPX | [GPX](https://gdal.org/drv_gpx.html) | gpx |
| GeoJSON | [GeoJSON](https://gdal.org/drv_geojson.html) | json geojson |
| GeoJSONSeq | [GeoJSON Sequence](https://gdal.org/drv_geojsonseq.html) | geojsonl geojsons |
| GeoRSS | [GeoRSS](https://gdal.org/drv_georss.html) |  |
| Geoconcept | Geoconcept | gxt txt |
| JML | [OpenJUMP JML](https://gdal.org/drv_jml.html) | jml |
| KML | [Keyhole Markup Language (KML)](https://gdal.org/drv_kml.html) | kml |
| MBTiles | [MBTiles](https://gdal.org/drivers/raster/mbtiles.html) | mbtiles |
| MapInfo File | [MapInfo File](https://gdal.org/drv_mitab.html) | tab mif mid |
| MapML | [MapML](https://gdal.org/drivers/vector/mapml.html) |  |
| NGW | [NextGIS Web](https://gdal.org/drv_ngw.html) |  |
| ODS | [Open Document/ LibreOffice / OpenOffice Spreadsheet ](https://gdal.org/drv_ods.html) | ods |
| OGR_GMT | [GMT ASCII Vectors (.gmt)](https://gdal.org/drv_gmt.html) | gmt |
| PCIDSK | [PCIDSK Database File](https://gdal.org/drivers/raster/pcidsk.html) | pix |
| PDF | [Geospatial PDF](https://gdal.org/drivers/raster/pdf.html) | pdf |
| PDS4 | [NASA Planetary Data System 4](https://gdal.org/drivers/raster/pds4.html) | xml |
| PGDUMP | [PostgreSQL SQL dump](https://gdal.org/drv_pgdump.html) | sql |
| S57 | [IHO S-57 (ENC)](https://gdal.org/drv_s57.html) | 000 |
| SQLite | [SQLite / Spatialite](https://gdal.org/drv_sqlite.html) | sqlite db |
| Selafin | [Selafin](https://gdal.org/drv_selafin.html) |  |
| TIGER | [U.S. Census TIGER/Line](https://gdal.org/drv_tiger.html) |  |
| VDV | [VDV-451/VDV-452/INTREST Data Format](https://gdal.org/drv_vdv.html) | txt x10 |
| VICAR | [MIPL VICAR file](https://gdal.org/drivers/raster/vicar.html) |  |
| WAsP | [WAsP .map format](https://gdal.org/drv_wasp.html) | map |
| XLSX | [MS Office Open XML spreadsheet](https://gdal.org/drv_xlsx.html) | xlsx xlsm |
{: .no_spell_check }

