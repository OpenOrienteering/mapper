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

\date 2013-08-31
\author Kai Pastor
\author Thomas Schöps

\todo Review and update the code overview.

\section core Core

Map is a (too) huge class which represents a complete map. It contains dynamic arrays of:

- Colors (MapColor)
- Symbols (Symbol)
- Templates (Template)
- Parts (MapPart) which in turn contain objects (Object).

It also keeps track of the currently selected map objects and contains an UndoManager object.

Furthermore, it has a MapRenderables renderables whose purpose will be explained soon,
and contains the settings for the georeferencing and for printing / exporting the map.

Most features of a map are represented by Object. All map objects have got
coordinates, which are stored as a dynamic array of MapCoord.
A PointObject has got a single coordinate.
A TextObject has got either one coordinate (representing a single anchor point)
or two coordinate (representing a text box). For a text box, the first
coordinate gives the center of the box, and the second one its width and height.
For a PathObject, the coordinates are interpreted as a path,
i.e. a line consisting of polygonal and/or cubic bezier curve segments.

To define paths with different kind of segments, the MapCoord objects store
extra flags:

- curve start: if set, the coordinate is the first of 4 which define a bezier curve.
- dash point: used for line symbols, can be equivalent to an OC*D dash point or corner point
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


\section rendering Rendering

The process to display map objects works like this:

- An Object is created, set up and added to a layer of a map.
- object->update(true) can be called to force an update of the object,
  otherwise it is updated the next time the map is drawn.
- This update (re)generates the ObjectRenderables, a set of Renderable which
  belong to the object. Instances of Renderable are the individual elements
  that make up the visualization of the object, such as dots in a dotted line.
  The power of a single renderable depends on the actual subclass,
  ranging from a simple dot to a line of text.
  The constraint is that every renderable uses just one map color.
- The ObjectRenderables are inserted into the MapRenderables of the map,
  where they are sorted by their color priority.
- MapRenderables::draw() goes through all Renderables in order,
  thus painting them with the correct color priority.


\section gui GUI

The topmost application windows are provided by the MainWindow class.
It is just responsible for the window itself; every MainWindow also has a
subclass of MainWindowController which is responsible for the content.
Currently, this can be the HomeScreenController or MapEditorController,
but in the future there could also be a controller for course setting, for example.

The MapEditorController has as most important members

- a Map object which contains colors, symbols, objects etc.,
- a MapView object which defines the view properties (zoom level, position offset, rotation),
- a MapWidget responsible for drawing,
- a MapEditorTool current_tool, for example the path drawing tool,
- a MapEditorActivity editor_activity. Activities are used to display stuff
  that is common to multiple tools. Example: the TemplateAdjustActivity displays
  the pass point positions when adjusting a template.


**/
