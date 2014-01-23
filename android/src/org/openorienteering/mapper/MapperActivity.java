/*
 *    Copyright 2013 Thomas Schöps
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

package org.openorienteering.mapper;

import android.os.Bundle;
import android.os.Looper;
import android.os.Build;
import android.os.SystemClock;
import android.app.AlertDialog;
import android.content.Intent;
import android.content.DialogInterface;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.location.Location;
import android.location.LocationManager;
import android.location.LocationProvider;
import android.location.LocationListener;
import android.location.GpsStatus;
import android.provider.Settings;
import android.view.Surface;
import android.widget.Toast;


/**
 * Contains Android Java code for Mapper.
 */
public class MapperActivity extends org.qtproject.qt5.android.bindings.QtActivity
{
	private static MapperActivity instance;
	
	private LocationManager locationManager;
	private LocationListener locationListener;
	private Location lastLocation = null;
	
	private boolean haveGPSFix = false;
	private boolean gpsUpdatesEnabled;
	private int gpsUpdateInterval;
	private long lastLocationMillis;
	private String yes_string;
	private String no_string;
	private String gps_disabled_string;

	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		instance = this;
		gpsUpdatesEnabled = false;

		locationManager = (LocationManager) getSystemService(LOCATION_SERVICE);
		locationManager.addGpsStatusListener(new GpsStatus.Listener() {
			public void onGpsStatusChanged(int event) {
				switch (event) {
				case GpsStatus.GPS_EVENT_SATELLITE_STATUS:
					if (lastLocation != null)
					{
						boolean haveGPSFixNow = (SystemClock.elapsedRealtime() - lastLocationMillis) < 5000;
						
						if (haveGPSFix && !haveGPSFixNow)
							updateTimeout();
						haveGPSFix = haveGPSFixNow;
					}
					break;
				
				case GpsStatus.GPS_EVENT_FIRST_FIX:
					haveGPSFix = true;
					break;
				}
			}
		});
		locationListener = new LocationListener() {
			public void onLocationChanged(Location location) {
				if (location == null)
					return;
				
				haveGPSFix = true;
				lastLocationMillis = SystemClock.elapsedRealtime();
				positionUpdated((float)location.getLatitude(), (float)location.getLongitude(), (float)location.getAltitude(), location.getAccuracy());
				lastLocation = location;
			}

			public void onStatusChanged(String provider, int status, Bundle extras)
			{
				if (provider != LocationManager.GPS_PROVIDER)
					return;
				
				switch (status) {
				case LocationProvider.OUT_OF_SERVICE:
					error();
					break;
				case LocationProvider.TEMPORARILY_UNAVAILABLE:
					updateTimeout();
					break;
				case LocationProvider.AVAILABLE:
					// do nothing
					break;
				}
			}

			public void onProviderEnabled(String provider)
			{
				// nothing yet
			}

			public void onProviderDisabled(String provider)
			{
				if (provider == LocationManager.GPS_PROVIDER)
					error();
			}
		};
	}
	
	@Override
	public void onPause()
	{
		super.onPause();
		if (gpsUpdatesEnabled)
			instance.locationManager.removeUpdates(locationListener);
	}
	
	@Override
	public void onResume()
	{
		super.onResume();
		if (gpsUpdatesEnabled)
			instance.locationManager.requestLocationUpdates(LocationManager.GPS_PROVIDER, gpsUpdateInterval, 0, locationListener, Looper.getMainLooper());
	}
	
	/** Checks if GPS is enabled in the Android settings and if not, prompts the user to enable it.
	 *  The dialog box works asynchronously, so the method cannot return the result. */
	void checkIfGPSEnabled()
	{
		boolean enabled = instance.locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER);
		if (!enabled)
		{
			runOnUiThread(new Runnable() {
				public void run() {
					DialogInterface.OnClickListener dialogClickListener = new DialogInterface.OnClickListener() {
						@Override
						public void onClick(DialogInterface dialog, int which) {
							switch (which){
							case DialogInterface.BUTTON_POSITIVE:
								Intent intent = new Intent(Settings.ACTION_LOCATION_SOURCE_SETTINGS);
								startActivity(intent);
								break;

							case DialogInterface.BUTTON_NEGATIVE:
								//No button clicked
								break;
							}
						}
					};

					AlertDialog.Builder builder = new AlertDialog.Builder(instance);
					builder.setMessage(gps_disabled_string)
						.setPositiveButton(yes_string, dialogClickListener)
						.setNegativeButton(no_string, dialogClickListener)
						.show();
				}
			});
		}
	}
	
	
	// Static methods to be called from C++
	
	public static void setTranslatableStrings(String yes_string, String no_string, String gps_disabled_string)
	{
		instance.yes_string = yes_string;
		instance.no_string = no_string;
		instance.gps_disabled_string = gps_disabled_string;
	}
	
	public static void showShortMessage(final String message)
	{
		instance.runOnUiThread(new Runnable() {
			public void run() {
				Toast.makeText(instance, message, Toast.LENGTH_SHORT).show();
			}
		});
	}
	
	public static void startGPSUpdates(int updateInterval)
	{
		instance.checkIfGPSEnabled();

		instance.locationManager.requestLocationUpdates(LocationManager.GPS_PROVIDER, updateInterval, 0, instance.locationListener, Looper.getMainLooper());
		instance.gpsUpdateInterval = updateInterval;
		instance.gpsUpdatesEnabled = true;
	}
	
	public static void stopGPSUpdates()
	{
		instance.locationManager.removeUpdates(instance.locationListener);
		instance.gpsUpdatesEnabled = false;
	}
	
	/** Locks the current display orientation.
	 *  While a native "locked" mode comes in API level 18,
	 *  this method tries to determine and lock the current orientation
	 *  even on devices with lower API level. On these devices, the screen
	 *  may be temporary in reverse orientation.
	 */
	public static void lockOrientation()
	{
		// ActivityInfo.SCREEN_ORIENTATION_LOCKED == 14 comes with API level 18
		if (Build.VERSION.SDK_INT >= 18)
		{
			instance.setRequestedOrientation(14);
			return;
		}
		
		int orientation = instance.getResources().getConfiguration().orientation;
		int rotation = instance.getWindowManager().getDefaultDisplay().getRotation();
		
		if (orientation == Configuration.ORIENTATION_PORTRAIT)
		{
			if (rotation == Surface.ROTATION_180)
				instance.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_REVERSE_PORTRAIT);
			else
				instance.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
		}
		else if (orientation == Configuration.ORIENTATION_LANDSCAPE)
		{
			if (rotation == Surface.ROTATION_180)
				instance.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_REVERSE_LANDSCAPE);
			else
				instance.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
		}
		
		// If we read another value now, then we must reverse the rotation.
		// Maybe this can occasionally return the old value (i.e. the value 
		// before the requested rotation takes effect).
		int new_rotation = instance.getWindowManager().getDefaultDisplay().getRotation();
		if (new_rotation != rotation)
		{
			// first try didn't lock the original rotation, retry reverse.
			if (orientation == Configuration.ORIENTATION_PORTRAIT)
			{
				if (new_rotation == Surface.ROTATION_180)
					instance.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
				else
					instance.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_REVERSE_PORTRAIT);
			}
			else if (orientation == Configuration.ORIENTATION_LANDSCAPE)
			{
				if (new_rotation == Surface.ROTATION_180)
					instance.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
				else
					instance.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_REVERSE_LANDSCAPE);
			}
		}
	}
	
	/** Unlocks the display orientation
	 *  by setting the requested orientation to unspecified.
	 */
	public static void unlockOrientation()
	{
		instance.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED);
	}
	
	/** Returns the display's current rotation.
	 */
	public static int getDisplayRotation()
	{
		return instance.getWindowManager().getDefaultDisplay().getRotation();
	}
	
	// Native C++ method declarations
	
	private static native void positionUpdated(float latitude, float longitude, float altitude, float horizontal_stddev);
	private static native void error();
	private static native void updateTimeout();
}
