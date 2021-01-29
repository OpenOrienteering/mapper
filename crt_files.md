---
title: CRT Files
authors:
  - Kai Pastor
last_modified_date: 21 January 2018
nav_order: 0.35
---

## Introduction

Cross Reference Table (CRT) files provide rules for assigning and replacing symbols.
They are used in the following situations:

 - replacing a map's symbol set (menu Symbols > Replace symbol set...),
 - replacing symbols within the current map (menu Symbols > Load CRT file...),
 - importing geospatial vector data (menu File > Import...).

To facilitate the automatic lookup of a suitable CRT file, CRT files may be placed in the symbol set folder
and named following this pattern:

SOURCE_ID-TARGET_ID.crt

where SOURCE_ID is a symbol set ID (menu Symbols > Symbol set ID...) or an import driver name (e.g. OCD, OSM, DXF),
and TARGET_ID is another symbol set ID (either of this map or of the replacement symbol set).
Examples:

 - OSM-ISOM2017.crt will be used when importing data through the OSM driver into an ISOM2017 map.
 - ISOM2000-ISOM2017.crt will be used when replacing the symbols of an ISOM2000 map with an ISOM2017 symbol set.


## CRT file syntax

Empty lines are ignored. Lines starting with a # character are considered comments, i.e. they are ignored, too.
Every other line is a replacement rule and consists of two parts.

The first part of the rule is the target symbol number. It determines which symbol will be assigned to matching objects.

The second part of the rule, separated from the first part by white space, determines the objects this rule applies to.
This can be given either as a symbol number or as a [object query](find_objects.md#advanced-query-language).
When using object queries, more specific rules need to follow more general rules.

Example from OSM-ISOM2017.crt:

 - 406    natural = wood
 - 408    natural = wood AND (wood:age = young OR wood:age = very_young)
 - 404    natural = wood AND wood:density = sparse
 - 408    natural = wood AND wood:density = dense

Example from ISOM2000-ISOM2017.crt:

 - 102       102
 - 103       103
 - 101.1     104
 - 102.1     105



## CRT file editing

CRT files may be edited with any text editor, e.g. notepad in Windows.
However the .crt filename extension is commonly associated with certificate files.
You will probably have to select an "All files" filter in the editor's Open-file dialog,
or temporarily change the extension to .txt.

When Mapper displays the symbol replacement dialog, modified symbol assignments can be saved to a CRT file via the "Symbol mapping" button.
Note that this dialog shows the assigned symbol in the second column, while in the CRT file, assigned symbols are recorded at the beginning of the line.

