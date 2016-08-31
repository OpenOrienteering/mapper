---
title: Storing Maps and Templates on Android Devices
keywords: Android
---

Storage locations
-----------------

Android organizes storage and permissions different than PCs.
Especially on SD cards, write access is restricted.

You can choose the following locations for storing data which want edit and display in OpenOrienteering Mapper:

- Top-level folder "OO-Mapper" on the primary storage volume. On most devices, the primary storage volume is a part of the built-in storage. 
  Files in this area are stored permanently, until you decide to remove them.
  Unfortunately, there is usually only limited capacity available - this memory is expensive when buying the device.

- App-specific folders on arbitrary storage volumes such as SD cards (since Mapper 0.6.5).
  The path of these folders is "Android/data/org.openorienteering.mapper/files".
  This is the only convenient way to store map data on cheap extra memory cards.
  However, you need to be aware of the fact that these folders are removed by Android when you uninstall the Mapper app.
  So please carefully save your changes to a PC.

OpenOrienteering Mapper for Android will create these folders when they are missing, and scan them for map files.
It will display warnings when you open maps from the non-permanent app-specific folders or from read-only locations.


File transfer
-------------

You can transfer files from and to a PC via a USB cable. Android supports multiple file transfer protocols.

- MTP is now the prefered method. It has litte interference with apps which accessing the storage at the same time.
  Via MTP, Android will only show files which are known to its Media Scanner. If you cannot find a file such a recorded GPX track, a reboot of the device could trigger a rescan.

- Mass Storage makes the storage unavailable for apps for the duration of the connection with the PC.
  (Android also needs to terminate apps which are stored on the volume which is provided as mass storage.)
  Unlike MTP, mass storage does not depend on the media scanner.

In some cases, users suffered from files being damaged during transfer. This can be worked around by choosing another method for file transfer. Remember to keep backups and to verify transferred files.
