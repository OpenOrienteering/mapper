---
title: Introduction to mapping for orienteering
author:
  - Thomas Schoeps
  - Kai Pastor
last_modified_date: 30 May 2025
nav_exclude: true
---

*Disclaimer:* This is is mostly an empty place holder, not a complete introduction to orienteering mapping. Contributions welcome.

* TOC
{:toc}

## Topics to be covered

 - **Suitability of orienteering terrains, permissions, scale:**
   Things to consider before starting a new map.
 - **Acquiring map templates (aka base maps):**
   Good templates save you lots of time, so it is often worth looking for the best templates available.
 - **Loading and matching templates:**
   After getting the templates, the map file is created where they need to be put into a common coordinate system.
 - **Orienteering symbol sets:**
   Before starting to map, you should become (more) familiar with the orienteering symbol sets.
 - **Surveying (classic method):**
   Surveying the terrain by printing out templates, drawing in map details, scanning your drawings and doing the final drawing at home.
 - **Surveying (digital method):**
   Surveying the terrain with help of mobile computers.
 - **Drawing:**
   Doing the final drawing with OpenOrienteering Mapper.
 - **Generalization:**
   The importance of generalization.

## Surveying (classic method)

### Prerequisites

It is very beneficial to get experience running orienteering before starting with orienteering mapping. This way, most symbols on orienteering maps and what to use them for will be known to you already and you have a feeling for the level of generalization which is expected in orienteering maps.

Depending on the method you want to use for surveying terrain, you will need material such as a compass, printer, scanner, a clipboard, suited pens, or a mobile computer such as a tablet running Windows or Linux.

### Fieldwork

Accurate mapping depends on high quality fieldwork. Ordinarily, fieldwork will usually produce a paper sketch with all the detail required to produce the map but often marked up with shorthand codes which are meaningful to the mapper but which will not appear on the finished map. Instead of paper the mapper may use Mylar or other robust drafting film. Working on transparent film also permits the use of an underlay which may have a coordinate grid or an old map visible underneath (at an appropriate scale).

The fieldwork will usually be done at a larger scale than the final map. With convenient rescaling of computer graphic images it is now possible for fieldwork to be undertaken at any convenient scale such as 1:5000 or even larger. This demands less skill in field penmanship than the traditional fieldwork scale of 1:7500.

The fieldwork is scanned - about 200dpi is usually quite sufficient resolution - now save the image as a .png file in your map file directory ( .jpg image files also work). This file is then loaded as a template underneath the new map screen and the features are inserted on the map on top of the fieldwork image. The fieldwork must be scaled and positioned appropriately for this purpose.

## Symbols

Symbols for orienteering maps are prescribed in the [IOF documents](https://orienteering.sport/iof/mapping/) ISOM (International Specification for Orienteering Maps), ISSprOM (Sprint maps), ISMTBOM (MTBO maps), and ISSkiOM (Ski orienteering maps).

It is very desirable that all orienteering maps should conform to the appropriate standard, and that includes conforming to the standard symbols for the discipline.

A full set of symbols for each standard will be made available with OpenOrienteering Mapper, and ideally all maps will be prepared using only the unmodified symbols described in the standards and provided in these symbol sets. This enables international competitors to receive a locally produced map and find it portrays the terrain in exactly the same way as in their home country. It also enables local orienteers to become familiar with the mapping conventions that are used for major events elsewhere. The use of local symbol variations defeats the purpose of mapping standards and confuses visiting competitors.

### Symbols Window

This area on the right hand side of the OpenOrienteering Mapper screen shows all the available symbols in the loaded standard set. Use the scroll-bar to move the array up and down to reveal symbols not showing.

Each symbol has a text (a part of the standard) describing where and how it may be used. That text can be made visible in the symbol window by pressing F1 while hovering over the symbol of interest.

## Colors

The standard colors of orienteering maps are defined in the terms of the Pantone Matching System (PMS) and are intended to describe the exact color of inks to be used in offset printing processes for the production of orienteering maps.

With the advent of inexpensive ink-jet printers and high quality printing papers it has become possible to print water resistant orienteering maps at very modest cost and with excellent control of the process and the colors used. In this context there has also been considerable discussion about appropriate colors to better enable color blind competitors to discriminate map features and it is possible that some changes may be adopted as a result.

### Color Window

This tool defines colors in terms of the CMYK system or the RGB system. Default colors for orienteering maps are entered but the colors that are actually printed by a particular printer on a particular paper type can produce a result which is quite different from the PMS color intended.

The IOF Map Commission has prepared an offset printed paper test sheet (IOF Print Test Sheet - available from your national mapping officer) and as a map file *PrintTestSheet_2025_v1.omap* (available as download from the [IOF](https://orienteering.sport/iof/mapping/)). The offset printed paper sheet has the currently prescribed colors for the production of maps (Store in a heavy envelope, do not use copies which have been kept loose and may be faded). The colors for a particular printer and paper may be adjusted using the Color Window so that a better match with the offset printed test sheet is achieved. The colors yellow (open space on the terrain) and brown (contour lines) are commonly quite divergent from the standard.

### Color adjustment

1. Scan the offset printed IOF Print Test Sheet in any scanner and use a color picker to get the CMYK values on screen for each color. By way of example this scan may yield CMYK values of 000, .800, .991, .569 respectively for 100% brown.
2. Load the file *PrintTestSheet_2025_v1.omap* into OpenOrienteering Mapper and print the page on your printer. Take this freshly printed page and scan that print on the same scanner and use the same color picker to get the color values you have in the scan. An example of this scan for 100% brown may yield CMYK values of 000, .689, .942, .255 respectively.
3. The difference between those two sets of values will indicate the adjustments required. Using the example values the Color Window line for 100% brown should have no adjustment to the cyan field, +.111 to magenta, +.049 to yellow, and +.314 to black. Those adjustments must be made in the OpenOrienteering Mapper Color Window. Having made similar adjustments for all colors, then run the step 2 print trial again.
4. In a perfect world the differences will all reduce to zero. In practice they may not because of non-linearities in the hardware and colors like brown are poorly defined on the color palette. Several iterations may be required to get a good match.

The same general approach may be employed but using the RGB color space instead.

When a contour is printed in an area of green on the map the brown of the contour loses contrast and the contour line becomes harder to see.  If the map is offset printed the green is laid down without a gap and the brown is printed over it while the green ink is still wet, with the result that the green and brown blend together where they are coincident. This creates a segment of line which is somewhat darker than the 100% brown line outside the green and the darker color is noted to be easier to see. This very desirable result is called an over-printing effect.

The Print Test Sheet is designed to demonstrate beneficial over-printing effects where these are achieved. Consider the visibility of the wavy contour lines (and blue creek lines) printed against a variety of backgrounds on the lower left of the sheet.

## Map scales

Map scales are set in accordance with the International Specification for Orienteering Maps (ISOM). The traditional competition format is at 1:15000. More recently maps may be printed at 1:10000 for shorter courses. Maps at 1:10k are visually identical to those at 1:15k. The ISOM emphatically requires that a larger scale 1:10k shall not be used in order to fit more detail onto the map but simply permits the map to be more easily read by older competitors who will generally have shorter courses and not require large maps. Sprint maps made to the ISSprOM standard may be at either 1:5000 or 1:4000 and use a different symbol set appropriate to that scale.

Printing of a map prepared at 1:15000 may be at either scale (and conversely a map prepared at 1:10k can be printed at 1:15k) with no loss of accuracy or information. Most course planning software provides for this re-scaling at the point of printing.

The ISOM details the size, shape and use of all the permitted symbols and codes on the map, and these are the only acceptable symbols and sizes on a map to be printed at a scale of 1:15000. If it is desired to prepare the original map at a scale of 1:10000 (or some other scale) a new symbol set appropriate to that scale would need to be created with proportionately larger symbols. There seems little point in this.
Do not confuse this with the scale of fieldwork which is sensibly at a larger scale than the finished map. Fieldwork is rescaled to the scale of the map when it is loaded as a [template](templates.md).

Using a larger scale with symbol sizes appropriate to a smaller scale provides more white space and enables more detail to be included but this is at the expense of readability, and the additional detail is almost always unnecessary and thus unhelpful.
