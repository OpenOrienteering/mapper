The symbol sets to be edited are stored in the src subdirectory in the verbose
.xmap format. The symbol sets to be distributed are stored in the more compact
.omap format in one subdirectory per scale. The test subdirectory builds (under
CMake) an executable (test) `symbol_set_t` which updates compressed files from
a list of known files in the src directory, thus automating the following steps.

Symbol set for different scales for ISOM and ISSprOM are shear enlargements of 
the basic scales which are 1:15000 for ISOM and 1:4000 for ISSprOM.

For ISMTBOM the scaling is more nuanced. While 1:15000 is the basic scale. The 
enlargment for symbol set and course setting symbols is 1x1.2 for 1:12500, 
1x 1.5 for 1:10000 and also 1x 1.5 for 1:7500 and 1:5000.[3]

For ISSkiOM the scaling is also more nuanced. 1:15000 is the basic scale symbol
set (SS) and course setting (CS). For the 1:12500 the factors are SS x1 and CS x1.2.
For the 1:10000 the factors are SS x1 and CS x1.5. For 1:7500 are SS x 1.33 and CS x 1.5.
For the 1:5000 the factors are SS x 1.5 and CS x 1.5. All enlargement factors are relative 
to the basic scale symbol set. [4]

As even in the shear enlargement cases we have symbols we do not want to enlarge
(e.g. the 999 Open Orienteering Mapper Logo) you would create scaled copies of the basic
scale sets the following way:

- Select all symbols to be enlarged uniformly. right click > Scale (choose enlargment percentage x1.5 = 150%).
- Repeat for other symbols to be enlarged by another factor.
- Do not (!!) enlarge the 999 Open Orienteering Mapper Logo.
- Change the scale of the symbol set Mapper>Map>Change Map scale **unchecking the Scale Symbol Sizes checkbox
**.
- Save the file follwing the established naming convention.

[1] ISOM 2019-2 https://1drv.ms/b/s!As4LDA11gDVmgt8CiJdF8MdoLshsLg
[2] ISSprOM 2019-2 https://1drv.ms/b/s!As4LDA11gDVmgvJrcMQn6mK9JsdRgA
[3] ISMTBOM 2022 https://1drv.ms/b/s!As4LDA11gDVmgvJnTyKq5MzVljdQMw
[4] ISSkiOM 2019 https://1drv.ms/b/s!As4LDA11gDVmgt9HH965LY27OyWsfg
