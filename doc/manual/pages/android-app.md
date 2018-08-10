---
title: The Mapper App for Android
authors:
  - Kai Pastor
  - Thomas Schoeps
keywords: Android
edited: 3 February 2018
redirect_from:
  - /android/
  - '/Android Manual/'
  - /mapper-manual/pages/android-index.html
---

## Preliminary remarks

The Android version of OpenOrienteering Mapper is not as self-descriptive as
the desktop version, so please read this first for important instructions.

[Device requirements and recommendations](android-requirements.md){: .subpage}

[Preparing a map on the PC](android-pc.md){: .subpage}

[Storing Maps and Templates on Android Devices](android-storage.md){: .subpage}

{% if doxygen %}
**Note:** The [online version](https://www.openorienteering.org/mapper-manual/android/) of this documentation may contain additions and corrections.
{% endif %}


## Using the Mapper app

On startup, the app shows a list of map files available from the [supported storage locations](android-storage.md). To start editing a map, touch its filename.

![ ](images/Android_UI_explanation.png)

The map editor is slightly different from the desktop version. This page focuses on specific features of the mobile version. To find out about the other symbols, see [Toolbars](toolbars.md) in the desktop application's manual.

#### ![ ](../mapper-images/arrow-thin-upleft.png) Hide the top bar

Using this button, you may hide the top bar, leaving more screen for the map. A similar button will remain visible for restoring the top bar again.


#### ![ ](../mapper-images/compass.png) Compass display toggle

When this item is activated, a compass is shown in the top-left corner of the screen. The red line indicates the north direction, while the black-white lines show the up direction of your device. If both are aligned, the device is aligned with magnetic north. When the needle is close to the north direction, a green circle appears which quickly shows how well the device is aligned: the larger it is, the better the alignment is. The compass works three-dimensionally, so you do not need to hold the device in a flat pose for it to work.

Note: the usefulness of the compass depends on the presence of a gyroscope, as mentioned in the device recommendations section.

Attention: the compass is very sensitive to magnetic materials. Android tries the eliminate the influence of local magnetic fields, but this requires calibration. To do this, move the device around in the shape of an 8 while rotating it and while the compass is active. If the local influence changes, re-calibration is required.


#### ![ ](../mapper-images/tool-gps-display.png) Location display toggle

If the currently active map work has valid georeferencing, this control will be enabled and will show a dot at your current position as obtained from GPS or other positioning services. In addition, in case an accuracy estimate (HDOP) is available, this shows a circle around the dot which indicates the positioning accuracy. The chance of your real position being in the circle is approximately 70%.

While this is active, your track is automatically recorded into a GPX file in the directory of your map, named "<map> - GPS-<YYYY-MM-DD>.gpx". This file is included as a template in the map. You can simply show it using the template visibility control button to view your track.

Note that track recording might be interrupted while the screen is off or the app is in the background to save battery. Keep the app active if you want to ensure continuous track recording.


#### ![ ](../mapper-images/gps-distance-rings.png) Distance rings toggle

This button is active if location display is active. When activated, this will display rings in 10 meter and 20 meter distance from the current location which can be used to do rough distance estimates.


#### ![ ](../mapper-images/rotate-map.png) Automatic north alignment toggle

When this toggle is active, the map is automatically rotated to north using the device's internal compass. The update frequency is 1 update per second. This is quite low on purpose to save battery, but you probably still need some replacement batteries when using this mode. While interacting with the map and for another 1.5 seconds after that, the map is not rotated to allow editing without unwanted rotations. If GPS position display is activated at the same time, a line starting from the GPS position and going in the direction of the up direction of your device is displayed which shows your viewing direction (assuming you are holding the device upright).


-----

#### ![ ](../mapper-images/map-parts.png) Map parts selector

This button opens a popup for changing the active map part.


#### ![ ](../mapper-images/tool-touch-cursor.png) Touch cursor toggle

This symbol activates a helper cursor which allows slow, but precise editing with fingers only. The cursor consists of a circular area and a pointer above the circle.

![ ](images/touch_cursor.png)

Touching the map anywhere outside the circle moves the pointer to this position. Touching the circle simulates a real touch at the pointer position above the circle.


#### ![ ](../mapper-images/templates.png) Template control

This shows a list of all opened templates. Touching a template toggles its visibility.


#### ![ ](../mapper-images/close.png) Close button

This closes the active map and returns to the map selection screen.


#### ![ ](../mapper-images/three-dots.png) Overflow button

Depending on the screen size of your device, some of the symbols available not fit onto the screen. They are instead placed in a list which is shown on touching the overflow button. If all symbols fit on the screen, this button is unused.


-----

#### ![ ](../mapper-images/view-zoom-out.png) Zoom out / Zoom to fixed

A short press will zoom out by one step. A long press will open a menu where you can jump to a fixed zoom (cf. screenshot).


#### ![ ](../mapper-images/move-to-gps.png) Move to location

When positioning services are enabled, this will move the map so that your current location is in the middle of the screen.


#### ![ ](../mapper-images/gps-temporary-point.png) Mark temporary position

This records a single point at your current location to act as a drawing help. Note that this marker is not saved in the map.


#### ![ ](../mapper-images/gps-temporary-path.png) Record temporary trace

This symbol is enabled when the location display is active. It records a temporary trace of the location, which is intended to act as a guideline for drawings. Using the trace directly is rarely useful because of the inaccurateness of location services.
Note that the temporary trace is not saved in the map.


#### ![ ](../mapper-images/gps-temporary-clear.png) Clear temporary markers

This removes the temporay traces and position markers. (They will also be discarded when closing the map file.)


#### ![ ](../mapper-images/paint-on-template-settings.png) Paint-on-template settings

This button allows to select the active image for scribbling (using the symbol above it).


-----

#### ![ ](../mapper-images/draw-freehand.png) Free-hand draw tool

Allows to draw paths free-handedly.


#### ![ ](../mapper-images/draw-point-gps.png) Draw at location tool

Sets a point object at your current location. Selecting this button first enters an averaging mode. While it is active, the input from the location service is averaged to get a more accurate estimate. Touch the map display anywhere to finish the averaging.


#### Symbol selector

Displays the active symbol, and opens the symbol selection screen on short press.

A long press of the symbol selector opens a popup menu which allows to read the symbol name and description, and to toggle the visibility and the protection state of the symbol.
