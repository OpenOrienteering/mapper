<!DOCTYPE html PUBLIC "-//W3C//DTD html 4.01 Transitional//EN">
<html>
<head>
<title>OpenOrienteering Mapper Help - Toolbars</title>
<link rel="stylesheet" href="oomap.css" type="text/css" title="OOMapper stylesheet">
<meta name="author" content="Peter Hoban, Thomas Schoeps">
<meta name="description" content="Open Orienteering Mapper help">
<meta name="keywords" content="Help, Orienteering, mapping">
</head>
<body>


<h3>Toolbars</h3>
<ul>
<li><a href="#general">General</a></li>
<li><a href="#view">View</a></li>
<li><a href="#drawing">Drawing</a></li>
<li><a href="#advanceddrawing">Advanced drawing</a></li>
</ul>
<p><i>Note:</i> Toolbars may be moved to a more convenient position.  To detach and move, pick the handle (the vertical line of dots) at the left end and drag the toolbar to your preferred position.  The resulting order of your tools may thus be different from this page.</p>


<h3 id="general">General toolbar</h3>

<h4 id="new_map">New map</h4>
<p><img class=small src="../../images/new.png" width="32" height="32" border="0" /> Click to create a new map.</p><br clear=all />

<h4 id="open_save_map">Open / Save map</h4>
<p><img class=small src="../../images/open.png" width="32" height="32" border="0" /> <img class=small src="../../images/save.png" width="32" height="32" border="0" /> Click to open / save an existing map.</p><br clear=all />

<h4 id="print">Print</h4>
<p><img class=small src="../../images/print.png" width="32" height="32" border="0" /> Click to toggle showing the print widget. <!-- TODO: write and link to print widget documentation --></p><br clear=all />

<h4 id="cut_copy_paste">Cut / Copy / Paste objects</h4>
<p><img class=small src="../../images/cut.png" width="32" height="32" border="0" /> <img class=small src="../../images/copy.png" width="32" height="32" border="0" /> <img class=small src="../../images/paste.png" width="32" height="32" border="0" /> Click to cut / copy the currently selected objects or to paste the clipboard contents into the map (at the center of the view).</p><br clear=all />

<h4 id="undo_redo">Undo / Redo object editing</h4>
<p><img class=small src="../../images/undo.png" width="32" height="32" border="0" /> <img class=small src="../../images/redo.png" width="32" height="32" border="0" /> Click to undo / redo the last map editing step.</p><br clear=all />


<h3 id="view">View toolbar</h3>

<h4 id="grid">Grid</h4>
<p><img class=small src="../../images/grid.png" width="32" height="32" border="0" /> Click this button to toggle showing the <a href="grid.html">map grid</a>. Click the arrow to the right of this button to show the <a href="grid.html">grid configuration</a>.</p><br clear=all />

<h4 id="pan_map">Pan map</h4>
<p><img class=small src="../../images/move.png" width="32" height="32" border="0" /> Use this tool <b>or the middle mouse button, or F6, or the arrow keys</b> to move the map by clicking the map and dragging the mouse.</p><br clear=all />

<h4 id="zoom_in">Zoom in</h4>
<p><img class=small src="../../images/view-zoom-in.png" width="32" height="32" border="0" alt="" /> Use this tool <b>or F7</b> to zoom in (enlarge) to view the map closer, centered on the present view. </p><br clear=all />

<h4 id="zoom_out">Zoom out</h4>
<p><img class=small src="../../images/view-zoom-out.png" width="32" height="32" border="0" alt="" /> Use this tool <b>or F8</b> to zoom out (shrink) the map to see more of it, centered on the present view.</p><br clear=all />

<h4 id="zoom_all">Show whole map</h4>
<p><img class=small src="../../images/view-show-all.png" width="32" height="32" border="0" alt="" /> Use this tool to display the whole map on your screen.<br/>This can be particularly useful when items are accidentally moved far from your working area and out of sight.  This tool brings it onscreen - if your map is just a dot in one corner then the rogue feature is near the other edge of the screen.</p><br clear=all />


<h3 id="drawing">Drawing toolbar</h3>

<h4 id="tool_edit_point">Object selector and point edit tool</h4>
<p><img class=small src="../../images/tool-edit.png" width="32" height="32" border="0" alt="" />Using this tool, click on the map to select a single object. If the object you wish is not selected with the first click, click more times to go through all objects below the cursor. To select multiple object hold the Shift key as you click subsequent objects. If you Shift-click on an object which has been already been selected the click will deselect that object. A group of objects may also be selected by drawing a selection box with a left-click and drag.</p>

<p>If only a few objects are selected, the nodes defining these object become visible. There are different types of nodes:
<ul><li>A square stands for a normal node</li>
<li>A diamond depicts a <a href="#dashpoint">dash point (see below)</a></li>
<li>A circle depicts a bezier curve handle, defining the tangent of a curve point</li></ul></p>

<p>Individual nodes can be dragged with the mouse to change their position. To change the tangent direction of a node on a bezier curve click the end of the handle and drag it. Longer handles have greater influence, while shorter handles have only local effect.</p>

<p>To <b>add</b> an extra node to an existing line or curve, ctrl-click on an empty spot of a path. By moving the mouse, the new node can be positioned in the same action. If a <b>dash point</b> is required, hold the space bar down while ctrl-clicking. To <b>remove</b> a node, ctrl-click an existing node.</p>

<p>To move the selected objects as a whole, click and drag the dashed box which encloses all selected objects.</p>

<p>While dragging nodes or objects, there are means for more accurate positioning:</p>
<ul>
<li>Holding <b>Ctrl</b> constrains the movement angle. For nodes and regularly shaped objects, these will be the parallel and perpendicular directions to the object.</li>
<li>Holding <b>Shift</b> makes the cursor snap to other existing objects.</li>
</ul>
<br clear=all />

<h4 id="tool_edit_line">Object selector and line edit tool</h4>
<p><img class=small src="../../images/tool-edit-line.png" width="32" height="32" border="0" alt="" />
Using this tool, objects can be selected like with the <a href="#tool_edit_point">point edit tool</a>.</p>

<p>If only a few objects are selected, line segments of these objects can be edited by clicking them and dragging the mouse. For straight lines, the angle constraint will be active automatically. This makes it easy to edit e.g. rectangular houses while ensuring that they stay rectangular. To toggle the angle constraint, hold Ctrl.</p>

<p>To switch a line segment between curved and straight, hold Ctrl while clicking the segment.</p><br clear=all />

<h4 id="tool_draw_point">Draw point tool</h4>
<p><img class=small src="../../images/draw-point.png" width="32" height="32" border="0">This tool enables you to insert a point symbol from the map symbol window. This includes boulders, rocky ground, knolls, waterholes or any other symbol representing a point feature too small to be drawn to scale. Select the symbol for the feature to be inserted by clicking it on the symbol window, click the point tool (if it is not activated automatically) and then click on the map to insert that feature on the map.</p>

<p>Some point symbols may be rotated, like the cave or the elongated small knoll. To set the desired orientation, click and move the mouse into the desired direction while holding the left mouse button down. To change the orientation after the point object has been placed, use the <a href="#tool_rotate_pattern">rotate pattern tool</a>.</p><br clear=all />

<h4 id="tool_draw_path">Draw path tool (to draw lines and areas)</h4>
<p><img class=small src="../../images/draw-path.png" width="32" height="32" border="0">The path tool draws line objects such as contour lines, watercourses and roads and area objects such as open land or different vegetation densities. To draw, first choose the symbol for the feature to be inserted by clicking it on the symbol window, then click the line tool if it has not been activated automatically.</p>

<p>To draw a path consisting of <b>straight segments</b>, like the shape of a house, proceed by clicking at the position of each corner on the map. At the last corner, right click or double click to finish the path. Area symbols will be closed automatically, so you do not need to click the first position again. In case you do not want right clicking to set the last path point in addition to finishing the path, you can change this behavior in the <a href="settings.html">program settings</a>.</p>

<p>To create a <b>smooth path</b> (a cubic bezier spline), click and drag the mouse away in the continuing direction. This creates a node with control handles which may be subsequently used to refine the shape of the curve.<br/>
<b>Important notice</b>: while this way of drawing curves may seem to be difficult at first, it is important to get used to it. Drawing "smooth" curves with many small straight segments instead is <b>not an alternative</b>, as these will look ugly when viewed closely. Just practice bezier curve drawing until you get used to it. Less nodes are often better than too many.</p>

<p>It is also possible to <b>mix straight and curved segments</b>: click once to create a corner point, click and drag to create a curve points. Segments between two curve points will be created as smooth curves, while all other segments will be straight.</p>

<p><a name="closed_object"></a>To draw a <b>closed path</b> (closed contour line, lake, etc.), press the return key to close the last segment of the path to the starting point. When drawing with area symbols, paths are closed automatically, however there may be an unwanted corner at the start/end point if you just finish the path roughly at the position where you started it, instead of pressing return.</p>

<p>To <b>snap</b> to existing objects when starting to draw or while drawing a path, hold Shift. To <b>follow</b> the shape of existing objects while drawing, click at the existing object while holding Shift and drag the mouse along it. Depending on the direction in which you drag the mouse, the object will be followed along this direction.</p>

<p>To constrain the <b>drawing angle</b>, hold Ctrl while drawing a path. The available angles will be intelligently chosen such that you can continue paths straight forward, or insert 90 or 45 degree corners.</p>

<p>To <b>pick a direction</b> of an existing object, Ctrl+click the desired object before starting to draw the path. Drawing angles will then be automatically constrained to the picked direction (and its perpendicular directions) until you press the Ctrl key. This is very useful for e.g. drawing parallel houses or hedges/fences in front of houses which are parallel to them.</p>

<p>To remove a misplaced point while drawing a line use the backspace key to <b>undo</b> one step at a time, or leave it and correct the position using the <a href="#tool_edit_point">edit tool</a>. If the partly drawn line is discovered to be a mistake use the Esc key to abort and remove it entirely.</p>

<h4 id name="dashpoint">Dash points</h4>
<p>Dash points serve different purposes, depending on the symbol on which they are used:</p>

<p><b>1.</b> For dashed line symbols like small paths it is sometimes useful to be able to steer the positioning of the dashes, e.g. path crossings should preferably be at the center of line dashes. When a dash point is inserted in a line, dashes will be exactly centered on this point.</p>

<p><b>2.</b> For lines with patterns such as fences, it is sometimes useful to be able to steer the positioning of the patterns. When a dash point is inserted in a line with such a pattern, patterns are shifted away from it. This is e.g. useful for corners of a fence to ensure a minimum distance of the dash patterns to the corners.</p>

<p><b>3.</b> Some symbols (e.g. 516 Power line) require bars at some nodes (pylons) but for example not at the point where the line ends at the edge of the map (no pylon). The bar is defined as a dash symbol inside the line symbol for 516 Power line. It will appear only at dash points along a line.</p>

<p>In general, it is enough to remember that dash points steer the positioning of line dashes, patterns, and dash symbols. When drawing, just try out how it behaves. Dash point nodes have a diamond shape when the line is selected (ordinary nodes are square). Drawing of a node as a dash point is toggled with the space bar: dash points will be drawn while the helpful tip at the bottom edge shows "<b>Dash points on</b>". This switch may be varied from node to node along the line.<br clear=all />

<h4 id="tool_draw_circle">Draw circle / ellipse tool (to draw lines and areas)</h4>
<p><img class=small src="../../images/draw-circle.png" width="32" height="32" border="0">The circle tool can be used to draw round lines and areas. To start drawing, first select a symbol of type line or area and the circle tool.</p>

<p>To draw a circle, either click on one point on the circle and then on the opposite point, or click and hold at the starting point and release the mouse button at the end point.</p>

<p>To draw an ellipse, click at one of the boundary points on the major or minor axis of the ellipse, then click and hold at the opposite point on the ellipse and define the extent by dragging the mouse away.</p>

<p>Circles and ellipses are represented by four bezier curves and can be edited like any other path.</p><br clear=all />

<h4 id="tool_draw_rectangle">Draw rectangle tool (to draw lines and areas)</h4>
<p><img class=small src="../../images/draw-rectangle.png" width="32" height="32" border="0">The rectangle tool is used to draw shapes with any number of 90 or 45 degree corners such as a building. To draw, first select the line or area symbol you want to draw with and select the rectangle tool. Then click once at each corner and double click or right click at the last corner of the rectangle. You can speed up drawing by clicking and dragging the mouse to draw two corners in one step: one at the click position and one at the release position.</p>

<p>While drawing, hold Ctrl to constrain the position of the next corner to positions which align with already drawn segments. For example, when drawing a small indentation, Ctrl can be held at its last corner to set it exactly at the position to which the line before the indentation would extend.</p>

<p>Rectangle objects can be edited like any path. However, consider using the <a href="#tool_edit_line">line edit tool</a> to preserve the angles.</p><br clear=all />

<h4 id="tool_draw_text">Write text tool</h4>
<p><img class=small src="../../images/draw-text.png" width="32" height="32" border="0">This tool places text on the map. In order to be language-independent orienteering maps generally do not have names or text information on features,  but text is useful for map titles and version numbers for example. It is necessary to select a text symbol (which determines the font settings) before the text tool will become available.</p>

<p>Two different types of text objects can be placed with this tool:</p>
<ul>
<li>Clicking once creates a text object with a single anchor point</li>
<li>Clicking and dragging the mouse creates a text box (with automatic text wrap)</li>
</ul>

<p>When text is edited, a small window with horizontal and vertical alignment options is shown. To edit text after it is created, choose the <a href="#tool_edit_point">point edit tool</a>, select the text object, and click again inside the object.</p><br clear=all />

<h4 id="draw_on_template">Template markup tool</h4>
<p><img class=small src="../../images/pencil.png" width="32" height="32" border="0">This tool enables the freehand drawing of lines, annotation and erasure on images loaded as <a href="templates.html">templates</a>, in a choice of 8 colors. It is intended for surveying with a mobile computer.</p>

<p>Click and hold the left mouse button to draw while the mouse is moved. Hold the right mouse button as the mouse is moved to erase. The markup is saved in the template image file <b>permanently</b>, so it is good practice to keep a copy of the template file in another place or use a blank transparent image for drawing.</p><br clear=all />

<h4 id="duplicate">Duplicate objects</h4>
<p><img class=small src="../../images/tool-duplicate.png" width="32" height="32" border="0" />This tool creates a duplicate of any selected object. Select the object(s), then click the tool to create an identical second symbol in the same place. As the duplicate is created in the same place, the appearance of the map does not immediately change. However, the duplicate can be selected and dragged to another location leaving the original symbol behind. To drag a symbol select the <a href="#tool_edit_point">point edit tool</a>, then click and hold on the enclosing box and drag as required. The duplicate tool is particularly useful when applied to create and move identical groups of items.</p><br clear=all />

<h4 id="switch_symbol">Switch symbol</h4>
<p><img class=small src="../../images/tool-switch-symbol.png" width="32" height="32" border="0">This tool changes the symbols of the selected objects to another. To use it, select the object(s) to change on the map, then select the target symbol in the symbol window. (<b>Attention</b>: with default settings, it is not possible to do this the other way round as selecting an object will select its symbol, so the initial symbol selection is discarded.) Then click the tool and the map symbols will change to the target symbol - provided that the target symbol can be applied to the selected map objects (i.e. either both must be points, or both texts, or both one of line, area or combined symbols.)</p><br clear=all />

<h4 id="fill_create_border">Fill or create border</h4>
<p><img class=small src="../../images/tool-fill-border.png" width="32" height="32" border="0">Having drawn a <a href="drawing_toolbar.html#closed_object">closed boundary</a> which requires a fill (such as a fence containing a thicket) select the boundary on the map (using the <a href="#tool_edit_point">point edit tool</a> tool), then choose the required fill symbol in the symbols window. (<b>Attention</b>: with default settings, it is not possible to do this the other way round as selecting an object will select its symbol, so the initial symbol selection is discarded.) A left click on the fill tool will put the chosen fill into the selected boundary.</p>

<p>Internally, this tool creates a duplicate of the selected object and assigns it the selected symbol. So, in addition to filling closed boundaries, it is also possible to create a boundary around an area, or create duplicates of lines with another symbol using this tool.</p><br clear=all />

<h4 id="switch_dashes">Switch dash direction</h4>
<p><img class=small src="../../images/tool-switch-dashes.png" width="32" height="32" border="0">This tool changes the direction of the selected (line) objects, so e.g. dashes of fences or cliffs will be flipped to the other side. Internally, the tool just reverses the coordinates of the path objects.</p><br clear=all />

<h4 id="connect">Connect lines</h4>
<p><img class=small src="../../images/tool-connect-paths.png" width="32" height="32" border="0">This tool enables two (or more) selected lines to be joined together to create a single line.</p>

<p>It is necessary that the ends to be joined are very close together, otherwise nothing will happen. It may be necessary to adjust the position and direction of the node at the join after connecting the lines.</p><br clear=all />

<h4 id="unify_areas">Unify areas</h4>
<p><img class=small src="../../images/tool-boolean-union.png" width="32" height="32" border="0">This tool merges two or more areas (having the same symbol) into one, deleting the overlapping parts. Simply select two or more areas using the <a href="#tool_edit_point">point edit tool</a> and then click on the Unify Areas tool to apply it.</p><br clear=all />

<h4 id="cut_tool">Cut lines and areas</h4>
<p><img class=small src="../../images/tool-cut.png" width="32" height="32" border="0">This tool cuts a selected existing line or area object into two parts.</p>

<p>To cut <b>lines</b>, click at the point where it is desired to break the line. The object will be split into two at this point and both will be selected, so it is possible to insert more cuts after the first. It is also possible to click on a line and drag the mouse along it to remove the marked part completely.</p>

<p>To cut an <b>area object</b>, select it and draw a cut line from one side of its boundary to another. Drawing works exactly as with the <a href="#tool_draw_path">draw path tool</a>. It is necessary to start and end the cut line on the boundary of the area to be cut. It is not sufficient to cross the boundary; both ends of the line must be on the boundary within a tolerance of 5 pixels (this may be altered in the <a href="settings.html">settings dialog</a>). The cut line may contain any number of internal vertices and can be polygonal or curved. The cut occurs immediately when the line is finished on the boundary (with a right click or double click).</p><br clear=all />

<h4 id="cut_hole">Cut holes</h4>
<p><img class=small src="../../images/tool-cut-hole.png" width="32" height="32" border="0">This tool cuts a hole into an area object. To do so, first select the object with the <a href="#tool_edit_point">point edit tool</a>, then click the cut holes tool. Then simply draw the shape of the hole on the area as you would draw with the <a href="#tool_draw_path">draw path tool</a>. After finishing, the boundary of the hole may then be edited in the same way as any path.</p>

<p>The menu shown by clicking the arrow to the right of this tool offers variants to cut circular or rectangular holes.</p>

<p>If the line describing the hole crosses the boundary of the object, the area outside the former boundary and inside the "hole" will be filled with the area symbol; however, this <b>must be avoided</b>! Features such as the unify areas tool or the area measurement will fail for such objects.</p><br clear=all />

<h4 id="rotate">Rotate objects</h4>
<p><img class=small src="../../images/tool-rotate.png" width="32" height="32" border="0">This tool rotates any selected object(s), which can include the whole map, about a selected pivot point and by any angle. Select the item(s) to be rotated using the <a href="#tool_edit_point">point edit tool</a>, then click the rotate tool. The rotation center will be marked with a small circle. Initially it is located at the bounding box midpoint of the selected objects, but it can be set to any position by clicking on the map. Then click somewhere at a convenient radius from the rotation center and move the selected objects about the rotation center to the desired position by dragging the mouse. By holding Ctrl, the rotation angle can be constrained to angles in a fixed stepping.</p><br clear=all />

<h4 id="tool_rotate_pattern">Rotate patterns</h4>
<p><img class=small src="../../images/tool-rotate-pattern.png" width="32" height="32" border="0" />This tool has two purposes:</p>
<ul>
<li>Setting the orientation of <b>area symbol patterns</b>, e.g. forest runnable in one direction.</li>
<li>Adjusting the orientation of <b>rotatable point symbols</b>, e.g. caves, after they have been placed.</li>
</ul>
<p>To use it, first select the object to be changed with the <a href="#tool_edit_point">point edit tool</a>, then click the rotate pattern tool. Then click any position on the map and drag the mouse into the desired direction to change the object's pattern to.</p><br clear=all />

<h4 id="scale">Scale objects</h4>
<p><img class=small src="../../images/tool-scale.png" width="32" height="32" border="0" />This tool scales the selected object(s), which can include the whole map. It works completely analogous to the <a href="#rotate">rotate tool</a>.</p><br clear=all />

<h4 id="measure">Measure lengths and areas</h4>
<p><img class=small src="../../images/tool-measure.png" width="32" height="32" border="0">This tool can be used to measure line lengths and area sizes. It will show both real world length or area in meters and the resulting length or area on the printed map.<br/>
When this tool is activated, a window appears containing the measurements for the selected object. To measure different objects, the selection can be changed using the edit tool while the window is active. It is also possible to draw new paths and have their length or area shown as they are drawn.</p><br clear=all />


<h3 id="advanceddrawing">Advanced drawing toolbar</h3>

<h4 id="cutout_physical">Cutout</h4>
<p><img class=small src="../../images/tool-cutout-physical.png" width="32" height="32" border="0" /> This cuts away all objects except inside a given region, making a map excerpt. It can also be used to cut away only a selected subset of objects outside the region.<br/>
To use it:
<ul>
<li>Draw the cutout shape (with any line or area symbol). The shape must be closed, ensure this by finishing the drawing by pressing the return key.</li>
<li>Then select the cutout tool. The shape will be marked in red.</li>
<li>For making a cutout of the map, now press the return key.</li>
<li>For cutting only some object, select those objects before pressing return.</li>
</ul></p><br clear=all />

<h4 id="cutaway_physical">Cut away</h4>
<p><img class=small src="../../images/tool-cutout-physical-inner.png" width="32" height="32" border="0" /> This is the opposite to the above cutout tool. It cuts away all or a subset of objects inside a selected cutout region. Usage is identical to those of the cutout tool.<br/>
This tool is useful for making training maps where certain symbols are missing in some places. For example, to create a contour-only region in a map, proceed like this:
<ul>
<li>Draw the cutout shape (with any line or area symbol). The shape must be closed, ensure this by finishing the drawing by pressing the return key.</li>
<li>Click the cut away tool. The shape will be marked in red.</li>
<li>In the symbol pane, select all symbols you want to cut away, e.g. everything except contours. You can select a range of symbols by clicking the first symbol, then clicking the last symbol while holding the Shift key.</li>
<li>Press the return key.</li>
</ul>
</p><br clear=all />

<h4 id="convert_to_curves">Convert to curves</h4>
<p><img class=small src="../../images/tool-convert-to-curves.png" width="32" height="32" border="0" /> This tool changes the shape of selected polygonal objects to smooth. It can be used to "prettify" objects which have been drawn as polygonal objects but are curved in reality (however, it is usually less effort to draw these as curved objects from the start). Note that this tool is still experimental and might take some time to compute for a large number of objects.</p><br clear=all />

<h4 id="simplify_path">Simplify path</h4>
<p><img class=small src="../../images/tool-simplify-path.png" width="32" height="32" border="0" /> This tool simplifies the shape of the selected paths by removing points which can be removed without changing the object's shape significantly. Note that this tool is still experimental and might take some time to compute for a large number of objects.</p><br clear=all />

<h4 id="intersect_areas">Intersect areas</h4>
<p><img class=small src="../../images/tool-boolean-intersection.png" width="32" height="32" border="0">This tool deletes all of the selected area objects that do not intersect with first-selected area. The only part which will remain is that part of the first area selected which was previously overlapped by one or more of the other selected areas. Select two or more overlapping objects of the same area type and then click on this tool to use it.</p><br clear=all />

<h4 id="area_difference">Area difference</h4>
<p><img class=small src="../../images/tool-boolean-difference.png" width="32" height="32" border="0">This tool deletes all parts of the first selected area object that overlap with one of the other selected areas. Select two or more overlapping objects of the same area type and then click on this tool to use it.</p><br clear=all />

<h4 id="area_xor">Area XOr</h4>
<p><img class=small src="../../images/tool-boolean-xor.png" width="32" height="32" border="0">This tool will XOr all selected areas. This means that all parts of the selected areas that overlap with another selected area will be deleted. Select two or more objects of the same area type and then click on this tool to use it.</p><br clear=all />


<p>&nbsp;</p>
<hr/>
<p><small>Updated on February 25th, 2013</small></p>
</body>
</html>
