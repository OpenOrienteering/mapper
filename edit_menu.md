---
title: Edit Menu
authors:
  - Peter Hoban
  - Thomas Schoeps
  - Kai Pastor
keywords: Menus
parent: Reference
nav_order: 0.2
last_modified_date: 21 January 2018
---

#### ![ ](../mapper-images/undo.png) Undo
**Ctrl+Z**

This most important function reverses the last change made to the map. Repeating 'Undo' will undo earlier changes, one step at a time. **Attention:** While changes to map objects can be undone this way, not all map changes are covered by the undo system. This includes changes to colors, symbols or templates.


#### ![ ](../mapper-images/redo.png) Redo
**Ctrl+Y, Ctrl+Shift+Z**

When you have taken Undo too far this will reverse the process &#8212; again one step at a time.


---

#### ![ ](../mapper-images/cut.png) Cut
**Ctrl+X**

This removes the selected object(s) and moves them to the clipboard.


#### ![ ](../mapper-images/copy.png) Copy
**Ctrl+C**

This copies the selected object(s) to the clipboard.


#### ![ ](../mapper-images/paste.png) Paste
**Ctrl+V**

This inserts the map object(s) on the clipboard into the map. They will be centered at the view midpoint. **Note:** When copying objects between different maps, their symbols and colors may be copied too in order to be able to display the objects in the new map in the same way as in the source map.


#### ![ ](../mapper-images/delete.png) Delete
**Del**

This action deletes the selected object(s).


---

#### Find...

Opens the ["Find objects" dialog](find_objects.md) which can be used to find objects by contained text, symbol name or tag keys and values.


#### Find next

Finds the next object matching the last query from the ["Find objects" dialog](find_objects.md).


---

#### Select all
**Ctrl+A**

This selects all objects in the current map part.


#### Select nothing
**Ctrl+Shift+A**

After this action, no object is selected.


#### Invert selection
**Ctrl+I**

This action inverts the selection in the current map part.


#### Select all objects with selected symbols

After this action, the selection consists of all objects in the current map part which have the selected symbol(s).


---

#### Clear undo / redo history

Clears the history of undo and redo steps, i.e. all undo and redo steps will be deleted. This reduces the file size for map file formats where the last undo and redo steps are stored, e.g. the omap format.


