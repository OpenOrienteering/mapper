---
title: File Menu
authors:
  - Peter Hoban
  - Kai Pastor
  - Thomas Schoeps
keywords: Menus
edited: 29 November 2015
---

These controls provide standard file management tools.

#### ![ ](../mapper-images/new.png) New
**Ctrl+N**

This shows the dialog to create a [new map](new_map.md).


#### ![ ](../mapper-images/open.png) Open...
**Ctrl+O**

Shows the dialog to open an existing map file. Currently supported formats are .omap, .xmap or .ocd (versions 6 to 12).


#### Open recent

This shows a list of recently opened files, recognizing that a file to be opened will often have been previously opened in the recent past.


#### ![ ](../mapper-images/save.png) Save
**Ctrl+S**

Saves the currently opened file. OCD files will be saved in version 8 format.


#### Save as...
**Ctrl+Shift+S**

Shows the file dialog for choosing a file name and file format to save the currently opened map.


---

#### Import...

Permits the import of maps or data in other formats. Currently the following formats are supported: .omap, .xmap, .ocd (version 6 to 11), .gpx, .osm, .dxf.


#### Export as...

Permits the export of the currently opened map to another format. Currently pdf and raster image export are supported.

For raster images, Mapper may additionally create world files. A world file is a small text file which defines the image's georeferencing relative to the map's projected coordinate reference system. Note that these files contain only six numerical parameters. They do not contain an identification of the projection, coordinate system or datum they refer to. The filename is derived from the image's filename, but with a modified filename extension usually ending with letter 'w'.


---

#### ![ ](../mapper-images/print.png) Print
**Ctrl+P**

Shows the print dialog to print the currently opened map.


---

#### Settings

Opens the [settings dialog](settings.md). (On OS X, the settings are in the usual place instead.)


---

#### ![ ](../mapper-images/close.png) Close
**Ctrl+W, Ctrl+F4**

Closes the current program window.


#### Exit
**Ctrl+Q**

Exits the program, which will close all open map windows.


