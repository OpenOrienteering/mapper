---
title: Frequently Asked Questions (FAQ)
authors:
  - Peter Hoban
  - Kai Pastor
  - Thomas Schoeps
keywords: FAQ
edited: 29 November 2015
---

* TOC
{:toc}

## Navigation

### How do I move the map?
The easiest way is to hold the middle mouse button while moving the mouse. But you can also press F6 and move the mouse, or use the corresponding toolbar button, or use the cursor keys.

### How to zoom in and out?
You can use the mouse wheel, press F7/F8 or +/- keys, or click the zoom buttons in the toolbar.


## Drawing

### I want to start drawing a map, but have no idea how to.
Start by reading the [short introduction to o-mapping](mapping-introduction.md).

### How do I add points to an existing path or delete points from it?
With the path object selected using the edit tool, hold Ctrl and click a free spot on the path to create a new point there. You can even move the mouse in the same action to set the position of the new point. To delete a point, hold Ctrl and click an existing point.

### How do I change line segments from curved to straight and vice versa?
With the [line edit tool](toolbars.md#tool_edit_line), select the object you want to edit and hold Ctrl while clicking the line segment. This switches the segment between curved and straight.


## Advanced editing

### How can I cut out a part of a map to take it for a new map?
First you save the map to the new file. Then draw some closed line or area (symbol doesn't matter) arround the border of the part which you want to keep. Now when this border object is finished and selected, click on the "Cutout" icon or menu item, then press the enter key.


## File formats

### What are the advantages of the .omap format?
The .omap format is OpenOrienteering Mapper's native format. It is XML-based, which means for example that it is relatively easy for other developers to access the information in map files. Additionally, it is platform independent, error-resistant and makes it possible to keep a high compatibility between different versions of the format.

### What is the difference between .xmap and .omap format?
The .xmap format is nearly identical but more verbose, even suitable for direct manual editing. The .omap format is tuned towards minimum size and fast processing.
If you want to keep you data in a line-based version control system such as git, you must use the .xmap format.

### Can Mapper read and write .ocd files?
Yes, Mapper reads .ocd files from different version (8 .. 12). But currently only version 8 .ocd files can be written. Beware that some details are neither loaded from nor saved to .ocd files, and some additional inaccuracies might occur.


## Mobile devices

### Will Mapper be available for Android?
An Android version is available. It already has a special user interface, but it needs a lot of work. See the [instructions for the Android app](android-index.md).

### Will Mapper be available for iPhone / iPad?
Probably not, due to distribution issues. Technically, a build should be feasible.
