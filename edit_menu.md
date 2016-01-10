---
title: Edit Menu
authors:
  - Peter Hoban
  - Thomas Schoeps
  - Kai Pastor
keywords: Menus
edited: 10 Januar 2016
---

<table>
<tr>
<td width="40"><img class="small" src="../mapper-images/undo.png" width="32" height="32" border="0" alt="" /></td>
<td width="100"><b>Undo</b></td>
<td width="70"><b>Ctrl+Z</b></td>
<td width="400">This most important function reverses the last change made to the map. Repeating 'Undo' will undo earlier changes, one step at a time.<br/>
<b>Attention:</b> While changes to map objects can be undone this way, not all map changes are covered by the undo system. This includes changes to colors, symbols or templates.</td>
</tr>
<tr>
<td><img class="small" src="../mapper-images/redo.png" width="32" height="32" border="0" alt="" /></td>
<td><b>Redo</b></td>
<td><b>Ctrl+Y, Ctrl+Shift+Z</b></td>
<td>When you have taken Undo too far this will reverse the process &#8212; again one step at a time.</td>
</tr>
<tr>
<td><img class="small" src="../mapper-images/cut.png" width="32" height="32" border="0" alt="" /></td>
<td><b>Cut</b></td>
<td><b>Ctrl+X</b></td>
<td>This removes the selected object(s) and moves them to the clipboard.</td>
</tr>
<tr>
<td><img class="small" src="../mapper-images/copy.png" width="32" height="32" border="0" alt="" /></td>
<td><b>Copy</b></td>
<td><b>Ctrl+C</b></td>
<td>This copies the selected object(s) to the clipboard.</td>
</tr>
<tr>
<td><img class="small" src="../mapper-images/paste.png" width="32" height="32" border="0" alt="" /></td>
<td><b>Paste</b></td>
<td><b>Ctrl+V</b></td>
<td>This inserts the map object(s) on the clipboard into the map. They will be centered at the view midpoint.<br/>
<b>Note:</b> When copying objects between different maps, their symbols and colors may be copied too in order to be able to display the objects in the new map in the same way as in the source map.</td>
</tr>
<tr>
<td></td>
<td><b>Clear undo / redo history</b></td>
<td></td>
<td>Clears the history of undo and redo steps, i.e. all undo and redo steps will be deleted. This reduces the file size for map file formats where the last undo and redo steps are stored, e.g. the omap format.</td>
</tr>
</table>
