---
title: Edit Menu
authors:
  - Peter Hoban
  - Thomas Schoeps
keywords: Menus
edited: 24 February 2013
---

<table><tr><td width="100"><h4>Undo</h4></td><td width="70"><h4>Ctrl+Z</h4></td><td width="40"><img class="small" src="../mapper-images/undo.png" width="32" height="32" border="0" alt="" /></td><td width="400">
<p>This most important function reverses the last change made to the map. Repeating 'Undo' will undo earlier changes, one step at a time.<br/><br/>
<b>Attention</b>: while changes to map objects can be undone this way, not all map changes are covered by the undo system. This includes changes to colors, symbols or templates. Because of this, it is very advisable to make a backup of your map before making such changes.</p>
</td></tr>

<tr><td><h4>Redo</h4></td><td><h4>Ctrl+Y</h4></td><td><img class="small" src="../mapper-images/redo.png" width="32" height="32" border="0" alt="" /></td><td>
<p>When you have taken Undo too far this will reverse the process &#8212; again one step at a time.</p></td></tr>

<tr><td><h4>Cut</h4></td><td><h4>Ctrl+X</h4></td><td><img class="small" src="../mapper-images/cut.png" width="32" height="32" border="0" alt="" /></td><td>
<p>This removes the selected object(s) and moves them to the clipboard.</p></td></tr>

<tr><td><h4>Copy</h4></td><td><h4>Ctrl+C</h4></td><td><img class="small" src="../mapper-images/copy.png" width="32" height="32" border="0" alt="" /></td><td>
<p>This copies the selected object(s) to the clipboard.</p></td></tr>

<tr><td><h4>Paste</h4></td><td><h4>Ctrl+V</h4></td><td><img class="small" src="../mapper-images/paste.png" width="32" height="32" border="0" alt="" /></td><td>
<p>This inserts the map object(s) on the clipboard into the map. They will be centered at the view midpoint. <br/><br/>
<b>Note</b>: when copying ojects between different maps, their symbols and colors may be copied too in order to be able to display the objects in the new map in the same way as in the source map.</p></td></tr>

<tr><td><h4>Clear undo / redo history</h4></td><td><h4></h4></td><td></td><td>
<p>Clears the undo / redo history, i.e. all undo and redo steps will be deleted. This reduces the file size for map file formats where the last undo and redo steps are stored, e.g. the omap format.</p></td></tr>

</table>


<h3>Editing tools</h3>
<p>OOMapper offers many editing tools which may be accessed through the <a href="tools_menu.md">Tools menu</a> or the <a href="toolbars.md">toolbars</a>.</p>

