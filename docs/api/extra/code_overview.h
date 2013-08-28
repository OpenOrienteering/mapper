/*
 *    Copyright 2012, 2013 Thomas Schöps, Kai Pastor
 *
 *    This file is part of OpenOrienteering.
 *
 *    OpenOrienteering is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    OpenOrienteering is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with OpenOrienteering.  If not, see <http://www.gnu.org/licenses/>.
 */

/**

\page code_overview Code overview

\author Kai Pastor
\author Thomas Schöps

\todo Review and update the code overview.


\section gui GUI

The topmost application windows are provided by the MainWindow class.
It is just responsible for the window itself; every MainWindow also has a
subclass of MainWindowController which is responsible for the content.
Currently, this can be the HomeScreenController or MapEditorController,
but in the future there could also be a controller for course setting, for example.

The MapEditorController has as most important members
a Map object which contains colors, symbols, objects etc.,
a MapView object which defines the view properties (zoom level, position offset, rotation)
and a MapWidget responsible for drawing.
It can also contain the MapEditorTool current_tool, for example the path drawing tool,
and a MapEditorActivity editor_activity.
Activities are used to display stuff that is common to multiple tools,
for example the GeoreferencingActivity displays the pass point positions when georeferencing.


\section core Core

Map is a (too) huge class which represents a complete map. It contains dynamic arrays of:

- Colors (MapColor)
- Symbols (Symbol)
- Templates (Template)
- Parts (MapPart) which in turn contain objects (Object).

It also keeps track of the currently selected map objects and contains an UndoManager object.

Furthermore, it has a RenderableContainer renderables whose purpose will be explained soon,
and contains the settings for the georeferencing and for printing / exporting the map.

Something which is common to all map objects are the coordinates. These are stored as a dynamic array of MapCoord.

- Point objects have only a single coordinate.
- Text objects have either one coordinate (if they have a single anchor point)
  or two (if they are box texts). In this case, the first coordinate gives
  the center of the box, the second one its width and height.

All other object types contain a "path" specified by the coordinates,
that is a line consisting of polygonal and/or cubic bezier curve segments.
To define theses paths, the extra flags which can be stored in MapCoord objects are used.
There are the following flags:

- curve start: if set, the coordinate is the first of 4 which define a bezier curve.
- dash point: used for line symbols, can be equivalent to an OCAD dash point or corner point
- hole point: if set, a break in the path starts at this coordinate and ends at the next one.
  This is different from OC*D's hole points for areas: in OC*D, the first point of a hole in an area
  is marked as a hole point, in OO Mapper the last point of the previous area is marked as hole point.
  This is done for consistency with hole points for paths, so there are no hacks needed
  for combined objects with lines, areas and holes in it.
- close point: if a path is closed and thus the last coordinate is at the same position
  as its first one, the last coordinate is marked as close point.

For path objects, the coordinates are also processed into another form
which approximates the path using only straight line segments: a vector of PathCoord.
These coordinates also contain information about the cumulative path length,
the parameter value if generated from a bezier curve, and from which segment
of the original coordinates they were generated.


\section rendering Rendering Process

The process to display map objects works like this:

- An Object is created, set up and added to a layer of a map.
- object->update(true) can be called to force an update of the object,
  otherwise it is updated the next time the map is drawn.
- This update (re)generates a set of Renderables which belong to the object.
  These are the individual parts that make up the object, for example the dots
  in a dotted line. How much is contained in a Renderable varies depending on
  the subclass, for example a dashed line can be made up of a single LineRenderable,
  but the important constraint is that every renderable uses just one map color.
- The renderables are inserted into the aforementioned RenderableContainer of the map,
  where they are sorted by their color priority.
- RenderableContainer::draw() goes through all Renderables in order,
  thus painting them with the correct color priority.

**/