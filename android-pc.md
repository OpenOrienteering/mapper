---
title: Preparing a Map on the PC
---

The Android app cannot be used to create new maps because it would be cumbersome to setup the base maps there. Instead, maps should be prepared with templates and possibly georeferencing using the PC version of Mapper first and then transferred to the mobile device for surveying.

If you want to use live GPS display, the map must be georeferenced. There are several options how to do this:


## Georeferencing when you create a new map

The first thing to do after creating a new map should be to load a georeferenced template. If you do not have one, you may use for example an OpenStreetMap export in case there is sufficient data at the map's location. To do so, go to https://www.openstreetmap.org, locate the map region, and click 'Export'. Then select an area which covers all of your templates and save the export as .osm file. This file can then be loaded into Mapper as a georeferenced template.

On loading the first georeferenced template, Mapper will show the georeferencing dialog. This may look a bit intimidating at first, but when loading a template, usually little needs to be changed there. First, the projection type should be checked. If you do not know which projection to use, just select UTM. Second, the magnetic declination should be entered to align the map with magnetic north. You may simply use the lookup button to look it up on the internet for the place of the template you loaded. However, be aware that the lookup only uses a model which may deviate from the real declination. It may be advantageous to check this in the terrain. All remaining values in the georeferencing dialog are already initialized using the newly loaded template, so you can then finish the dialog.

Then load all remaining templates and finish preparing the map. Non-georeferenced templates will need to be moved to match the location of the georeferenced templates.


## Georeferencing when you use an existing map

In this case, the easiest way is to first create a new map with the same symbol set as the existing map. The old map can be selected as the symbol set source in the new map dialog to do this. Then load a georeferenced template as detailed in the paragraph above (use e.g. OpenStreetMap if you do not have a georeferenced template).

After that, load the existing map as template in the new map. Then use the alignment tools for templates to move the existing map to match with the new georeferenced template. When you are finished, import the existing map template into the new map file using the corresponding action in the template dock widget, and you should be done.

Attention: especially for old maps, the whole existing map may be locally distorted. In this case it will not be possible to match the old map and the georeferenced template and the only way to georeference the map is to do a tedious undistortion. Mapper does not provide a tool for this.


## Preparing for free-hand scribbling

![ ](../mapper-images/pencil.png)

Mapper supports free-hand drawing onto image templates using the tool with the pencil icon shown above. This is especially convenient to use in case you want to do only (or mostly) drafting while you are in the field, and draw the final version of the map at home on a PC. It has the advantage of being quick and it only alters the template you draw on, i.e. it does not create new map objects, so Mapper's map display does not become slower. This may be relevant on mobile devices because of little processing power or because of battery runtime.

If you want to use this functionality, you should best load a separate, initially transparent image template in png format for it. Suited images can be downloaded here:

 - [size 1024x1024](attachment/scribble_1024.png)
 - [size 2048x2048](attachment/scribble_2048.png)

The images contain a red border, but are transparent everywhere else. After downloading a file, load it in your map file and move it so it covers the area you want to draw scribbles on. Make sure that the image resolution of the template is good enough by drawing a test scribble. If not, you need to scale it down using the template settings. You may also create multiple copies of the file and load them as separate templates.

Direct drawing onto base map images is NOT recommended because you will not be able to use the eraser without also erasing the base map and because these images usually are in the lossy JPEG format which will introduce visible artifacts when saving the file and loading it again.

Note that to view the scribbles on a PC after creating them on a mobile device, you need to transfer not only the map file, but also the affected image template file(s) back to the PC, as the scribbles are saved directly in the image files.
