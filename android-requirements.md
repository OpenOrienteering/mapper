---
title: Device Requirements and Recommendations
keywords: Android
---

## Android version

The minimum version for running the app is 2.3. Write access to SD cards may require Android 4.4 (cf. [Storage locations](android-storage.md))


## Water-proofing

For obvious reasons, water-proof devices may be advantageous.


## Stylus

For doing any serious work, a stylus is a must-have (because of precision & speed, and because of temperature during winter). You should preferably look for devices which come with an adapted stylus such as the Samsung Galaxy Note series, as general styluses for capacitive touch screens have rather wide tips.


## Screen size

There is a compromise between ease of handling (better on smaller devices) and overview (better on larger devices).


## Battery

It is very desirable to use a device with replaceable battery. It may be possible to use an external battery pack for devices with integrated battery.


## Sensors

OpenOrienteering Mapper can use integrated GPS receivers. However, their accuracy may be low, so it may be advantageous to connect to an external GPS receiver. There is no direct support for external devices in Mapper, but some third party apps bridge the gap. For example the [Bluetooth GPS app](https://play.google.com/store/apps/details?id=googoo.android.btgps&rdid=googoo.android.btgps) or [Bluetooth GNSS app](https://play.google.com/store/apps/details?id=com.clearevo.bluetooth_gnss) provide a "mock" location provider which can be used when Android is put in developer mode.

The app can use a magnetometer and accelerometer as a compass. Almost all modern devices should contain these sensors. If a gyroscope is also available, it will be used to improve the compass stability. However, these sensors are typically not accurate enough to be used for measurements. They can be used for convenience when not relying on a compass, but for accurate measurements, an external compass is recommended.
