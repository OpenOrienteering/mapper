---
title: Map Grid
authors:
  - Peter Hoban
  - Thomas Schoeps
keywords: Map
nav_order: 0.13
last_modified_date: 25 February 2013
---

![Grid icon](../mapper-images/grid.png) The map grid can be activated by clicking the corresponding button in the [view toolbar](toolbars.md#view-toolbar). Settings for the grid can be changed by clicking the arrow to the right of this button.

### Grid settings

![ ](images/grid_settings.png)

#### Show grid

Toggles the grid's visibility.

#### Snap to grid

If activated, holding the Shift key while drawing or editing objects will snap not only to existing objects, but also to intersections of grid lines if the grid is visible.

#### Line color

The line color can be adjusted here.

#### Display

Here the display can be constrained to horizontal or vertical lines only.

#### Alignment

Here you can choose whether the vertical grid lines should align with magnetic north (which is assumed to be identical with the up direction of the paper), the grid north or true north directions. The latter work only if the map is [georeferenced](georeferencing.md). If grid north is selected, in addition the origin of the grid will be shifted to the grid coordinates origin. Furthermore, it is possible to define an offset angle which is added to the north angle to calculate the grid orientation.

#### Positioning

The spacing of the grid lines can be specified here. Normally, at the coordinate system origin there will always be a grid line intersection. Using the offset values, it can be shifted away from the coordinate system origin.
