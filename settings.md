---
title: Settings
authors:
  - Peter Hoban
  - Thomas Schoeps
keywords: Settings
edited: 25 February 2013
---

<p>In the settings dialog, the program can be adjusted to suit your application.</p>

<h3>General page</h3>

<h4 id="language">Language</h4>
<p>The program language may be selected here.</p>

<h4 id="recently_used_file">Open most recently used file</h4>
<p>If this option is set, on program startup the last recently used file will be opened, otherwise the home screen will be shown.</p>

<h4 id="tip_of_the_day">Show tip of the day</h4>
<p>This option controls whether the tip of the day should be included on the home screen.</p>


<h3>Editor page</h3>

<h4 id="antialiasing">High quality map display (antialiasing)</h4>
<p>Map elements are drawn more smoothly (and less pixelated) if this option is set, however the drawing speed is lower.</p>

<h4 id="antialiasing_text">High quality text display in map (antialiasing), slow</h4>
<p>This controls the same option as the above for map texts. It is separate because text display is particularly slow.</p>

<h4 id="tolerance">Click tolerance</h4>
<p>This controls how close you need to click on map objects or object handles for selecting them, but also for cut lines with the <a href="toolbars.md#cut_tool">cut tool</a> for example. Set a larger value if you have difficulties (or if you use a tablet), however this may lead to some unintentional selections.</p>

<h4 id="snap_distance">Snap distance (Shift)</h4>
<p>This controls how close you need to be for snapping to an existing object when holding Shift while editing an object or drawing.</p>

<h4 id="selection">When editing an object, automatically select its symbol, too</h4>
<p>When this box is checked selection of any object on the map will cause the the same symbol to be selected in the symbol pane. This is often convenient as symbols may be selected by simply clicking an object with this symbol on the map.</p>

<h4 id="zoomaway">Zoom away from cursor when zooming out</h4>
<p>When ticked the zoom function will zoom away from the cursor rather than from the middle of the window when using the scroll wheel to zoom out. Using the menu or toolbar zoom tools (or the function keys F7 & F8) will still zoom to/from the middle of the screen.</p>

<h4 id="drawing_set_last_point">Drawing tools: set last point on finishing with right click</h4>
<p>When this is activated, right clicking while drawing an object will set the last point <b>and</b> finish drawing the object. When disabled, right clicking will <b>only</b> finish the object without setting the last point.</p>

<h4 id="keep_closed_templates">Templates: keep settings of closed templates</h4>
<p>When active, the settings of templates (like their positions) will be kept in the map file when they are closed, so they can be reopened later while restoring their settings. In essence, "closing" templates does not really close them this way. When the setting is off, closed templates cannot be restored later.</p>

<h4 id="edit_tool_delete_spline_point">Edit tool: actions on deleting bezier spline points</h4>
<p>These settings control what happens when a node between two curves in a path is deleted. The first is used when a node is deleted normally by Ctrl+clicking it, the second is used when a node is deleted by Ctrl+Shift+clicking it.</p>

<ul>
<li><b>Retain old shape</b>: The program tries to move the remaining curve handles so the shape of the old curve is retained as much as possible in the remaining curve.</li>
<li><b>Reset outer curve handles</b>: The outer handles (those of the remaining curve) are reset to the positions where they would be placed if the outer curve would have been drawn with the draw path tool. Note that this way the resulting shape is independent of the position of the deleted node.</li>
<li><b>Keep outer curve handles</b>: Nothing happens apart from the deleted node and its two associated handles being deleted. The outer handles (those of the remaining curve) are simply kept at their current positions. Note that this way the resulting shape is independent of the position of the deleted node.</li>
</ul>

<h4 id="rectangle_cross_radius">Rectangle tool: radius of helper cross</h4>
<p>This controls the radius of the helper cross which is shown when using the rectangle tool, in pixels.</p>

<h4 id="rectange_preview_width">Rectangle tool: preview the width of lines with the helper cross</h4>
<p>When this is checked, the helper cross adapts to the selected symbol: for each direction, two lines are drawn which show the radius of the used (line) symbol. This allows for better judgement of the result while drawing, but leads to more cluttering on the screen.</p>

