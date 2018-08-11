---
title: Toolbars
authors:
  - Peter Hoban
  - Kai Pastor
  - Thomas Schoeps
keywords: Toolbars
edited: 5 June 2018
todo:
  - Split this page and update ALL context help in Mapper.
  - Update context help for zoom-in and -out in Mapper.
---

<!-- Note: Each item needs an ID. This ID is used to open the context-sensitive help. -->

 * [Toolbar positions and visibility](#toolbar-visibility)
 * [General toolbar](#general-toolbar)
 * [View toolbar](#view-toolbar)
 * [Map parts toolbar](#map-parts-toolbar)
 * [Drawing toolbar](#drawing-toolbar)
 * [Editing toolbar](#editing-toolbar)
 * [Advanced drawing](#advanced-drawing-toolbar)

## Toolbar positions and visibility  {#toolbar-visibility}

Toolbars may be moved to a more convenient position. To detach and move, pick the handle at the left end and drag the toolbar to your preferred position. The resulting order of your toolbars may thus be different from this page.

You may hide toolbars you do not use. To close or open a toolbar, right click on any open toolbar or docked window. This will open a small window where you can disable or enable toolbars and docked windows.


## General toolbar {#general-toolbar}

#### ![ ](../mapper-images/new.png) New map {#new_map}
Ctrl+N

Click to create a new map.

#### ![ ](../mapper-images/open.png) ![ ](../mapper-images/save.png) Open / save map {#open_save_map}
Ctrl+O / Ctrl+S

Click to open/save an existing map.

#### ![ ](../mapper-images/print.png) Print map {#print}
Ctrl+P

Click to open the print dialog.

#### ![ ](../mapper-images/cut.png)  ![ ](../mapper-images/copy.png) ![ ](../mapper-images/paste.png) Cut / copy / paste objects {#cut_copy_paste}
Ctrl+X / Ctrl+C / Ctrl+V

Click to cut / copy the currently selected objects to the clipboard, or to paste the clipboard contents into the map (at the center of the view).

#### ![ ](../mapper-images/undo.png) ![ ](../mapper-images/redo.png) Undo / redo object editing {#undo_redo}
Ctrl+Z / Ctrl+Y

Click to undo or redo the last map editing step.


## View toolbar {#view-toolbar}

#### ![ ](../mapper-images/grid.png) Grid {#grid}
G

Click the button to toggle showing the [map grid](grid.md). Click the arrow to the right of this button to show the [grid configuration](grid.md).

#### ![ ](../mapper-images/move.png) Pan map {#pan_map}
F6

Use this tool to move the map by clicking the map and dragging the mouse.

*Note:* You may always move the map by dragging with middle mouse button, or by using the arrow keys.

#### ![ ](../mapper-images/view-zoom-in.png) ![ ](../mapper-images/view-zoom-out.png) Zoom in / zoom out {#zoom_in_out}
F7, + / F8, -

Use these actions to zoom in (enlarge) or zoom out (shrink). The center of the view stays at the same position in the map.

#### ![ ](../mapper-images/view-show-all.png) Show whole map {#zoom_all}
Use this tool to display the whole map, including the currently visible templates, at maximum possible zoom.

This can be particularly useful when items (objects, templates) are accidentally moved far from your working area and out of sight. This tool brings them onscreen. If the map is just a dot in one corner then the rogue feature is near the other edge of the screen.


## Map parts toolbar {#map-parts-toolbar}

This toolbar will not be shown unless the map has multiple parts.

At the moment, the map parts toolbar has only a single drop down box for indicating and selecting the active map part.



## Drawing toolbar {#drawing-toolbar}

#### ![ ](../mapper-images/tool-edit.png) Edit objects {#tool_edit_point}
E

Using this tool, click on the map to select a single object. If the object you wish is not selected with the first click, click more times to go through all objects below the cursor. To select multiple objects, hold the Shift key when you click subsequent objects. If you Shift-click on an object which has already been selected, the click will deselect that object. A group of objects may also be selected by drawing a selection box with a left-click and drag.

If only a few objects are selected, the nodes defining these objects become visible. There are different types of nodes:
 - A square stands for a normal node.
 - A diamond depicts a [dash point (see below)](#dash-points).
 - A circle depicts a bezier curve handle, defining the tangent of a curve point.
Individual nodes can be dragged with the mouse to change their position. To change the tangent direction of a node on a bezier curve, click the end of the handle and drag it. Longer handles have greater influence, while shorter handles have only local effect.

To **add** an extra node to an existing line or curve, Ctrl-click on an empty spot of a path. By moving the mouse, the new node can be positioned in the same action. If a **dash point** is required, hold the space bar down while Ctrl-clicking. To **remove** a node, Ctrl-click an existing node.

To move the selected objects as a whole, click and drag the dashed box which encloses all selected objects. While dragging nodes or objects, there are means for more accurate positioning:
 - Holding **Ctrl** constrains the movement angle. For nodes and regularly shaped objects, these will be the parallel and perpendicular directions to the object.
 - Holding **Shift** makes the cursor snap to other existing objects.


#### ![ ](../mapper-images/tool-edit-line.png) Edit lines {#tool_edit_line}
L

Using this tool, objects can be selected like with the [point editing tool](#tool_edit_point).

If only a few objects are selected, line segments of these objects can be edited by clicking them and dragging the mouse. For straight lines, the angle constraint will be active automatically. This makes it easy to edit e.g. rectangular houses while ensuring that they stay rectangular. To toggle the angle constraint, hold Ctrl.

To switch a line segment between curved and straight, hold Ctrl while clicking the segment.


#### ![ ](../mapper-images/draw-point.png) Set point objects {#tool_draw_point}
S

This tool enables you to insert a point symbol from the map symbol window. This includes boulders, rocky ground, knolls, waterholes or any other symbol representing a point feature too small to be drawn to scale. Select the symbol for the feature to be inserted by clicking it on the symbol window, click the point tool (if it is not activated automatically) and then click on the map to insert that feature on the map.

Some point symbols may be rotated, like the cave or the elongated small knoll. To set the desired orientation, click and move the mouse into the desired direction while holding the left mouse button down. To change the orientation after the point object has been placed, use the [rotate pattern tool](#tool_rotate_pattern).


#### ![ ](../mapper-images/draw-path.png) Draw paths {#tool_draw_path}
P

The path tool draws line objects such as contour lines, watercourses and roads and area objects such as open land or different vegetation densities. To draw, first choose the symbol for the feature to be inserted by clicking it on the symbol window, then click the line tool if it has not been activated automatically.

To draw a path consisting of **straight segments**, like the shape of a house, proceed by clicking at the position of each corner on the map. At the last corner, right click or double click to finish the path. Area symbols will be closed automatically, so you do not need to click the first position again. In case you do not want right clicking to set the last path point in addition to finishing the path, you can change this behavior in the [program settings](settings.md).

To create a **smooth path** (a cubic bezier spline), click and drag the mouse away in the continuing direction. This creates a node with control handles which may be subsequently used to refine the shape of the curve.

*Note:* While this way of drawing curves may seem to be difficult at first, it is important to get used to it. Drawing "smooth" curves with many small straight segments instead is **not an alternative**, as these will look ugly when viewed closely. Just practice bezier curve drawing until you get used to it. Less nodes are often better than too many.

It is also possible to **mix straight and curved segments**: click once to create a corner point, click and drag to create a curve points. Segments between two curve points will be created as smooth curves, while all other segments will be straight.

To **snap** to existing objects when starting to draw or while drawing a path, hold Shift. To **follow** the shape of existing objects while drawing, click at the existing object while holding Shift and drag the mouse along it. Depending on the direction in which you drag the mouse, the object will be followed along this direction.

To constrain the **drawing angle**, hold Ctrl while drawing a path. The available angles will be intelligently chosen such that you can continue paths straight forward, or insert 90 or 45 degree corners.

To **pick a direction** of an existing object, Ctrl+click the desired object before starting to draw the path. Drawing angles will then be automatically constrained to the picked direction (and its perpendicular directions) until you press the Ctrl key. This is very useful for e.g. drawing parallel houses or hedges/fences in front of houses which are parallel to them.

To remove a misplaced point while drawing a line, use the backspace key to **undo** one step at a time, or leave it and correct the position using the [point editing tool](#tool_edit_point). If the partly drawn line is discovered to be a mistake, use the Esc key to abort and remove it entirely.

##### Closed paths
To draw a **closed path** (closed contour line, lake, etc.), press the return key to close the last segment of the path to the starting point. When drawing with area symbols, paths are closed automatically. However, there may be an unwanted corner at the start/end point if you just finish the path roughly at the position where you started it, instead of pressing return.

##### Dash points
Dash points serve different purposes, depending on the symbol on which they are used:

1. For dashed line symbols like small paths it is sometimes useful to be able to steer the positioning of the dashes, e.g. path crossings should preferably be at the center of line dashes. When a dash point is inserted in a line, dashes will be exactly centered on this point.
2. For lines with patterns such as fences, it is sometimes useful to be able to steer the positioning of the patterns. When a dash point is inserted in a line with such a pattern, patterns are shifted away from it. This is e.g. useful for corners of a fence to ensure a minimum distance of the dash patterns to the corners.
3. Some symbols (e.g. 516 Power line) require bars at some nodes (pylons) but for example not at the point where the line ends at the edge of the map (no pylon). The bar is defined as a dash symbol inside the line symbol for 516 Power line. It will appear only at dash points along a line.

In general, it is enough to remember that dash points steer the positioning of line dashes, patterns, and dash symbols. When drawing, just try out how it behaves. Dash point nodes have a diamond shape when the line is selected (ordinary nodes are square). Drawing of a node as a dash point is toggled with the space bar: dash points will be drawn while the helpful tip in the status bar at the bottom edge shows "**Dash points on**". This switch may be varied from node to node along the line.


#### ![ ](../mapper-images/draw-circle.png) Draw circles and ellipses {#tool_draw_circle}
O

The circle tool can be used to draw round lines and areas. To start drawing, first select a symbol of type line or area and the circle tool.

To draw a circle, either click on one point on the circle and then on the opposite point, or click and hold at the starting point and release the mouse button at the end point.

To draw an ellipse, click at one of the boundary points on the major or minor axis of the ellipse, then click and hold at the opposite point on the ellipse and define the extent by dragging the mouse away.

Circles and ellipses are represented by four bezier curves and can be edited like any other path.


#### ![ ](../mapper-images/draw-rectangle.png) Draw rectangles {#tool_draw_rectangle}
Ctrl+R

The rectangle tool is used to draw shapes with any number of 90 or 45 degree corners such as a building. To draw, first select the line or area symbol you want to draw with and select the rectangle tool. Then click once at each corner and double click or right click at the last corner of the rectangle. You can speed up drawing by clicking and dragging the mouse to draw two corners in one step: one at the click position and one at the release position.

While drawing, hold Ctrl to constrain the position of the next corner to positions which align with already drawn segments. For example, when drawing a small indentation, Ctrl can be held at its last corner to set it exactly at the position to which the line before the indentation would extend.

Rectangle objects can be edited like any path. However, consider using the [line edit tool](#tool_edit_line) to preserve the angles.


#### ![ ](../mapper-images/draw-freehand.png) Draw free-handedly {#tool_draw_freehand}
This tool draws both line and area objects by approximating the path taken by the cursor using straight line segments. To use this tool, click at the starting point, drag the cursor where you want the path to go then release the mouse to finish drawing. The object you have created can be edited in the same way as other line or area objects.

#### ![ ](../mapper-images/tool-fill.png) Fill bounded areas {#tool_fill}
F

This tool fills areas of unbroken white space with an area symbol. To use this tool, select the area symbol, then click on white space i.e. any place not already covered by another area object. Internally, this tool first finds all paths, which can belong to many different objects, that form a boundary around the unbroken white space. A new closed shape is created that has the same paths as the white space boundary and is filled with your chosen symbol. Also, if you use this tool with a line symbol selected, then a border around the white space will be created.

*Attention:* This tool will not work if your chosen white space is not completely contained by other path objects.

#### ![ ](../mapper-images/draw-text.png) Write text {#tool_draw_text}
T

This tool places text on the map. In order to be language-independent, orienteering maps generally do not have names or text information on features, but text is useful for map titles and version numbers for example. It is necessary to select a text symbol (which determines the font settings) before the text tool will become available.

Two different types of text objects can be placed with this tool:

 - Clicking once creates a text object with a single anchor point.
 - Clicking and dragging the mouse creates a text box (with automatic text wrap).

When text is edited, a small window with horizontal and vertical alignment options is shown. To edit text after it is created, choose the [edit objects tool](#tool_edit_point), select the text object, and click again inside the object.

#### ![ ](../mapper-images/pencil.png) Paint on template {#draw_on_template}
This tool enables the freehand drawing of lines, annotation and erasure on images loaded as [templates](templates.md), in a choice of 8 colors. It is intended for surveying with a mobile computer.

Click and hold the left mouse button to draw while the mouse is moved. Hold the right mouse button as the mouse is moved to erase. The markup is saved in the template image file **permanently**, so it is good practice to keep a copy of the template file in another place or use a blank transparent image for drawing.


## Editing toolbar {#editing-toolbar}

#### ![ ](../mapper-images/delete.png) Delete {#tool_delete}
Del

This tool deletes the currently selected map object(s).


#### ![ ](../mapper-images/tool-duplicate.png) Duplicate {#duplicate}
D

This tool creates a duplicate of each selected map object. Select the object(s), then click the tool to create identical cloned objects in the same place. As the duplicates are created in the same place, the appearance of the map does not immediately change. However, the duplicates can be selected and dragged to another location, leaving the original objects behind. To drag an object, select the [edit objects tool](#tool_edit_point), then click and hold on the enclosing box and drag as required. The duplicate tool is particularly useful when applied to create and move identical groups of items.


#### ![ ](../mapper-images/tool-switch-symbol.png) Switch symbol {#switch_symbol}
Ctrl+G

This tool changes the symbols of the selected map objects to another. To use it, select the object(s) to change on the map, then select the target symbol in the symbol window. (*Attention:* With default settings, it is not possible to do this the other way round as selecting an object will select its symbol, so the initial symbol selection is discarded.) Then click the tool and the map symbols will change to the target symbol - provided that the target symbol can be applied to the selected map objects (i.e. either both must be points, or both texts, or both one of line, area or combined symbols.)

#### ![ ](../mapper-images/tool-fill-border.png) Fill / Create border {#fill_create_border}
Ctrl+F

Having drawn a [closed boundary](#drawing-toolbar) which requires a fill (such as a fence containing a thicket), select the boundary on the map (using the [point edit tool](#tool_edit_point) tool), then choose the required fill symbol in the symbols window. (*Attention:* with default settings, it is not possible to do this the other way round as selecting a map object will select its symbol, so the initial symbol selection is discarded.) A left click on the fill tool will put the chosen fill into the selected boundary.

Internally, this tool creates a duplicate of the selected object and assigns it the selected symbol. So, in addition to filling closed boundaries, it is also possible to create a boundary around an area, or create duplicates of lines with another symbol using this tool.


#### ![ ](../mapper-images/tool-switch-dashes.png) Switch dash direction {#switch_dashes}
Ctrl+D

This tool changes the direction of the selected (line) objects, so e.g. dashes of fences or cliffs will be flipped to the other side. Internally, the tool just reverses the coordinates of the path objects.


#### ![ ](../mapper-images/tool-connect-paths.png) Connect path {#connect}
C

This tool enables two (or more) selected lines to be joined together to create a single line.

It is necessary that the ends to be joined are very close together, otherwise nothing will happen. It may be necessary to adjust the position and direction of the node at the join after connecting the lines.


#### ![ ](../mapper-images/tool-boolean-union.png) Unify areas {#unify_areas}
U

This tool merges two or more areas (having the same symbol) into one, deleting the overlapping parts. Simply select two or more areas using the [edit objects tool](#tool_edit_point) and then click on the Unify Areas tool to apply it.


#### ![ ](../mapper-images/tool-cut.png) Cut object {#cut_tool}
K

This tool cuts a selected existing line or area object into two parts.

To cut **lines**, click at the point where it is desired to break the line. The object will be split into two at this point and both will be selected, so it is possible to insert more cuts after the first. It is also possible to click on a line and drag the mouse along it to remove the marked part completely.

To cut an **area object**, select it and draw a cut line from one side of its boundary to another. Drawing works exactly as with the [draw path tool](#tool_draw_path). It is necessary to start and end the cut line on the boundary of the area to be cut. It is not sufficient to cross the boundary; both ends of the line must be on the boundary within a tolerance of 5 pixels (this may be altered in the [settings dialog](settings.md)). The cut line may contain any number of internal vertices and can be polygonal or curved. The cut occurs immediately when the line is finished on the boundary (with a right click or double click).


#### ![ ](../mapper-images/tool-cut-hole.png) Cut free form hole {#cut_hole}
H

This tool cuts a hole into an area object. To do so, first select the object with the [point edit tool](#tool_edit_point), then click the cut free form hole tool. Then simply draw the shape of the hole on the area as you would draw with the [draw path tool](#tool_draw_path). After finishing, the boundary of the hole may then be edited in the same way as any path.

Apart from free form holes, the menu shown by clicking the arrow to the right of this tool offers variants to cut circular or rectangular holes.

If the line describing the hole crosses the boundary of the object, the area outside the former boundary and inside the "hole" will be filled with the area symbol; however, this **must be avoided**! Features such as the unify areas tool or the area measurement will fail for such objects.


#### ![ ](../mapper-images/tool-rotate.png) Rotate object(s) {#rotate}
R

This tool rotates any selected object(s), which can include the whole map, about a selected pivot point and by any angle. Select the item(s) to be rotated using the [point edit tool](#tool_edit_point), then click the rotate tool.

The rotation center will be marked with a small circle. Initially it is located at the bounding box midpoint of the selected objects, but it can be set to any position by clicking on the map. Then click somewhere at a convenient radius from the rotation center and move the selected objects about the rotation center to the desired position by dragging the mouse. By holding Ctrl, the rotation angle can be constrained to angles in a fixed stepping.


#### ![ ](../mapper-images/tool-rotate-pattern.png) Rotate patterns {#tool_rotate_pattern}
This tool has two purposes:

 - Setting the orientation of **area symbol patterns**, e.g. forest runnable in one direction.
 - Adjusting the orientation of **rotatable point symbols**, e.g. caves, after they have been placed.

To use it, first select the object to be changed with the [point edit tool](#tool_edit_point), then click the rotate pattern tool. Then click any position on the map and drag the mouse into the desired direction to change the object's pattern to.


#### ![ ](../mapper-images/tool-scale.png) Scale object(s) {#scale}
Z

This tool scales the selected object(s), which can include the whole map. It works completely analogous to the [rotate tool](#rotate).


#### ![ ](../mapper-images/tool-measure.png) Measure lengths and areas {#measure}
M

This tool can be used to measure line lengths and area sizes. It will show both real world length or area in meters and the resulting length or area on the printed map.

When this tool is activated, a window appears containing the measurements for the selected object. To measure different objects, the selection can be changed using the edit tool while the window is active. It is also possible to draw new paths and have their length or area shown as they are drawn.


## Advanced drawing toolbar {#advanced-drawing-toolbar}

#### ![ ](../mapper-images/tool-cutout-physical.png) Cutout {#cutout_physical}
This cuts away all objects except inside a given region, making a map excerpt. It can also be used to cut away only a selected subset of objects outside the region. To use it:

 - Draw the cutout shape (with any line or area symbol). The shape must be closed, ensure this by finishing the drawing by pressing the return key.
 - Then select the cutout tool. The shape will be marked in red.
 - For making a cutout of the map, now press the return key.
 - For cutting only some objects, select those objects before pressing return.


#### ![ ](../mapper-images/tool-cutout-physical-inner.png) Cut away {#cutaway_physical}
This is the opposite to the above cutout tool. It cuts away all or a subset of objects inside a selected cutout region. Usage is identical to that of the cutout tool.

This tool is useful for making training maps where certain symbols are missing in some places.


#### ![ ](../mapper-images/tool-convert-to-curves.png) Convert to curves {#convert_to_curves}
N

This tool changes the shape of selected polygonal objects to smooth. It can be used to "prettify" objects which have been drawn as polygonal objects but are curved in reality (however, it is usually less effort to draw these as curved objects from the start). Note that this tool is still experimental and might take some time to compute for a large number of objects.


#### ![ ](../mapper-images/tool-simplify-path.png) Simplify path {#simplify_path}
Ctrl-M

This tool simplifies the shape of the selected paths by removing points which can be removed without changing the object's shape significantly. Note that this tool is still experimental and might take some time to compute for a large number of objects.


#### ![ ](../mapper-images/tool-distribute-points.png) Distribute points along path {#distribute_points}
This tool lets you create a number of point objects evenly distributed along a path.

To use this tool, you need to select a path object first, then select the point symbol. Now the action will open a dialog which lets you choose the number of objects and various other parameters.


#### ![ ](../mapper-images/tool-boolean-intersection.png) Intersect areas {#intersect_areas}
This tool deletes all of the selected area objects that do not intersect with first-selected area. The only part which will remain is that part of the first area selected which was previously overlapped by one or more of the other selected areas. Select two or more overlapping objects of the same area type and then click on this tool to use it.


#### ![ ](../mapper-images/tool-boolean-difference.png) Cut away from area {#area_difference}
This tool deletes all parts of the first selected area object that overlap with one of the other selected areas. Select two or more overlapping objects of the same area type and then click on this tool to use it.


#### ![ ](../mapper-images/tool-boolean-xor.png) Area XOr {#area_xor}
This tool will XOr all selected areas. This means that all parts of the selected areas that overlap with another selected area will be deleted. Select two or more objects of the same area type and then click on this tool to use it.


#### ![ ](../mapper-images/tool-boolean-merge-holes.png) Merge area holes {#area_merge_holes}
This tool unifies the areas of two or more overlapping holes in an area object into a single hole. These holes must be created using the [cut hole tools](#cut_hole). This tool is useful as when two area holes overlap, the overlapping portion(s) have the same fill as the parent area object instead of being empty, which may not be desired. This tool may also be useful after boolean operations on objects.
