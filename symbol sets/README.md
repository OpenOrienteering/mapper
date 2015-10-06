The symbol sets to be edited are stored in the src subdirectory in the verbose
.xmap format. The symbol sets to be distributed are stored in the more compact
.omap format in one subdirectory per scale. The test subdirectory builds (under
CMake) an executable (test) `symbol_set_t` which updates compressed files from
a list of known files in the src directory, thus automating the following steps.

The ISOM 1:10000 symbol set can be created from the 1:15000 version by
following these steps:

 - Map -> Change map scale... from 1:15000 to 1:10000, all options ticked on.
 - Set north line patterns spacing from 50 mm to 40 mm.
 - Delete the 999 OpenOrienteering Logo symbol and the course setting symbols.
 - Copy the original version of these symbols without adjusting scale.
 - Likewise, delete/replace the 602 Registration mark symbol.

The ISSOM 1:4000 symbol set can be created from the 1:5000 version by
following these steps:

 - Map -> Change map scale... from 1:5000 to 1:4000, but
   **deactivate option "Scale symbols"**.
 - Set north line patterns spacing from 30 mm to 37.5 mm.
