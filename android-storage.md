---
title: Storing Maps and Templates on Android Devices
keywords: Android
---

## Storage locations

Android organizes storage and permissions different than PCs.
Especially on SD cards, write access for apps is restricted.

You can choose the following locations for storing data which you want to edit and to display in OpenOrienteering Mapper:

 - Folder "OOMapper" at the top level of the primary storage volume. On most devices, the primary storage volume is a part of the built-in storage.
  Files in this area are stored permanently, until you decide to remove them.
  Unfortunately, there is usually only limited capacity available - this memory is expensive when buying the device.

 - Path "Android/data/org.openorienteering.mapper/files" on arbitrary storage volumes such as SD cards (since Mapper 0.6.5).
  This is the only convenient way to store map data on cheap extra memory cards.
  However, you need to be aware of the fact that these folders are removed by Android when you uninstall the Mapper app.
  So please carefully save your changes to a PC.

OpenOrienteering Mapper for Android will create these folders when they are missing, and scan them for map files. It will display warnings when you open maps from the non-permanent app-specific folders or from read-only locations. (Top-level "OOMapper" folders on secondary storage volumes are scanned, but cannot be written to.)


## File transfer

You can transfer files from and to a PC via a USB cable. Android supports multiple file transfer protocols.

- MTP is the preferred method now. There is hardly any interference for apps while the device is connected to the PC.
  Via MTP, Android will only show files which are known to its Media Scanner.  

- Mass Storage makes the storage unavailable for apps for the duration of the connection with the PC.
  (Android also needs to terminate apps which are stored on the volume which is provided as mass storage.)
  Unlike MTP, mass storage does not depend on the media scanner, so all files are always visible.

Note that after mapping, you might want to transfer back not only the modified map but also GPX tracks and templates you painted on.


## Data loss prevention and recovery

*Remember to keep backups and to verify transferred files.*

If a file appears to be corrupted after transfer back to PC, or you cannot find
a new file such as a recorded GPX track, a reboot of the Android device could
solve the issue. After the reboot, Android's media scanner will take notice of
new files and changed file sizes. However, Mapper v0.8.3 is expected to bring a
solution to this issue.

In some situations, the Mapper app might not be able to properly save data and
shutdown as quickly as requested by the Android operating system, for example
when you start other apps or when the device runs out of power.
