---
title: Settings
authors:
  - Peter Hoban
  - Thomas Schoeps
keywords: Settings
last_modified_date: 25 February 2013
nav_order: 0.51
---

In the settings dialog, the program can be adjusted to suit your application.

### General page

#### Language

The program language may be selected here.

#### Open most recently used file

If this option is set, on program startup the last recently used file will be opened, otherwise the home screen will be shown.

#### Show tip of the day

This option controls whether the tip of the day should be included on the home screen.

### Editor page

#### High quality map display (antialiasing)

Map elements are drawn more smoothly (and less pixelated) if this option is set, however the drawing speed is lower.

#### High quality text display in map (antialiasing), slow

This controls the same option as the above for map texts. It is separate because text display is particularly slow.

#### Click tolerance

This controls how close you need to click on map objects or object handles for selecting them, but also for cut lines with the [cut tool](toolbars.md#cut_tool) for example. Set a larger value if you have difficulties (or if you use a tablet), however this may lead to some unintentional selections.

#### Snap distance (Shift)

This controls how close you need to be for snapping to an existing object when holding Shift while editing an object or drawing.

#### When editing an object, automatically select its symbol, too

When this box is checked selection of any object on the map will cause the the same symbol to be selected in the symbol pane. This is often convenient as symbols may be selected by simply clicking an object with this symbol on the map.

#### Zoom away from cursor when zooming out

When ticked the zoom function will zoom away from the cursor rather than from the middle of the window when using the scroll wheel to zoom out. Using the menu or toolbar zoom tools (or the function keys F7 & F8) will still zoom to/from the middle of the screen.

#### Drawing tools: set last point on finishing with right click

When this is activated, right clicking while drawing an object will set the last point **and** finish drawing the object. When disabled, right clicking will **only** finish the object without setting the last point.

#### Templates: keep settings of closed templates

When active, the settings of templates (like their positions) will be kept in the map file when they are closed, so they can be reopened later while restoring their settings. In essence, "closing" templates does not really close them this way. When the setting is off, closed templates cannot be restored later.

#### Edit tool: actions on deleting Bézier spline points

These settings control what happens when a node between two curves in a path is deleted. The first is used when a node is deleted normally by Ctrl+clicking it, the second is used when a node is deleted by Ctrl+Shift+clicking it.

 - **Retain old shape**: The program tries to move the remaining curve handles so the shape of the old curve is retained as much as possible in the remaining curve.
 - **Reset outer curve handles**: The outer handles (those of the remaining curve) are reset to the positions where they would be placed if the outer curve would have been drawn with the draw path tool. Note that this way the resulting shape is independent of the position of the deleted node.
 - **Keep outer curve handles**: Nothing happens apart from the deleted node and its two associated handles being deleted. The outer handles (those of the remaining curve) are simply kept at their current positions. Note that this way the resulting shape is independent of the position of the deleted node.

#### Rectangle tool: radius of helper cross

This controls the radius of the helper cross which is shown when using the rectangle tool, in pixels.

#### Rectangle tool: preview the width of lines with the helper cross

When this is checked, the helper cross adapts to the selected symbol: for each direction, two lines are drawn which show the radius of the used (line) symbol. This allows for better judgment of the result while drawing, but leads to more cluttering on the screen.

