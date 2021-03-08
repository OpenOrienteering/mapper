---
title: Map Parts
authors:
  - Kai Pastor
keywords: Map parts
last_modified_date: 10 January 2016
parent: Objects
nav_order: 0.3
---

Map parts partition the map in different collections of objects which can be worked on independently.
This can be used to organize the work of different people in a map making team.
Map parts are also used to separate imported objects from existing map content.

Every map object belongs to a particular map part.
Selected objects may be moved to a different map part.
You may also move all objects of the current part to another one,
or merge all other parts into the current one in a single step.
See the [Map menu](map_menu.md) for actions which create and change map parts.

Only a single map part can be the "current" one.
It is the context where all selection, editing and drawing operations take place.
The current map part is displayed and selected in the map part toolbar.
This toolbar is not displayed as long as the map has only one part.

Other than templates, map parts are not drawn sequentially one after the other,
but simultaneously according to the symbols and colors.
All map parts share the map's colors and symbol sets.
