---
title: File Menu
authors:
  - Peter Hoban
  - Kai Pastor
  - Thomas Schoeps
keywords: Menus
parent: Menus
grand_parent: Reference
nav_order: 0.1
last_modified_date: 1 June 2025
---

These controls provide standard file management tools.

#### ![ ](../mapper-images/new.png) New
**Ctrl+N**

This shows the dialog to create a [new map](new_map.md).


#### ![ ](../mapper-images/open.png) Open...
**Ctrl+O**

Shows the dialog to open an existing map file. Currently supported formats are .omap, .xmap or .ocd (versions 8 .. 12, 2018).


#### Open recent

This shows a list of recently opened files, recognizing that a file to be opened will often have been previously opened in the recent past.


#### ![ ](../mapper-images/save.png) Save
**Ctrl+S**

Saves the currently opened map. OCD files will be saved in version 8 .. 12 format.


#### Save as...
**Ctrl+Shift+S**

Shows the file dialog for choosing a file name and file format to save the currently opened map.


---

#### Import...

Permits the import of maps or data in other formats.
The following formats are supported:
OpenOrienteering Mapper (.omap, .xmap),
OCD version 8 .. 12, 2018 (.ocd),
geospatial vector data (formats supported by [GDAL](gdal.md)).


#### Export as...

Permits the export of the currently opened map or a part of it to another format.
The following formats are supported:
Image (raster graphics),
PDF,
KMZ,
simple course (XML or KML format),
geospatial vector data (formats supported by [GDAL](gdal.md)).

For raster images, Mapper may additionally create world files. A world file is a small text file which defines the image's georeferencing relative to the map's projected coordinate reference system. Note that these files contain only six numerical parameters. They do not contain an identification of the projection, coordinate system or datum they refer to. The filename is derived from the image's filename, but with a modified filename extension usually ending with letter 'w'.

For geospatial vector data, the actual GDAL driver is selected
by the extension (e.g. ".dxf") given to the filename.


---

#### ![ ](../mapper-images/print.png) Print...
**Ctrl+P**

Shows the print dialog to print the currently opened map.


---

#### Settings...

Opens the [settings dialog](settings.md). (On OS X, the settings are in the usual place instead.)


---

#### ![ ](../mapper-images/close.png) Close
**Ctrl+W, Ctrl+F4**

Closes the current program window.


#### Exit
**Ctrl+Q**

Exits the program, which will close all open map windows.


