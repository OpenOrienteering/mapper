/*
 *    Copyright 2013 Thomas Schöps
 *    Copyright 2014 Thomas Schöps, Kai Pastor
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

// Until Qt Creator and androiddeployqt fully support per-flavor app IDs,
// we must change the ID in AndroidManifest.xml, and thus the BuildConfig
// and R classes are generated in the MAPPER_APP_ID namespace.
import @MAPPER_APP_ID@.BuildConfig;
import @MAPPER_APP_ID@.R;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.location.GpsStatus;
import android.location.Location;
import android.location.LocationManager;
import android.location.LocationProvider;
import android.location.LocationListener;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.CountDownTimer;
import android.os.Looper;
import android.os.PowerManager;
import android.os.SystemClock;
import android.provider.Settings;
import android.view.Gravity;
import android.view.Surface;
import android.widget.Toast;


/**
 * Contains Android Java code for Mapper.
 */
public class MapperActivity extends org.qtproject.qt5.android.bindings.QtActivity
{
	private static MapperActivity instance;
	
	private String yes_string;
	private String no_string;
	private String gps_disabled_string;
	
	private static Toast toast;
	private static CountDownTimer toast_reset;
	private static boolean service_started = false;
	private static boolean optimization_request_done = false;

	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		instance = this;
	}
	
	/** Call setIntent, as recommended for singleTask launch mode. */
	@Override
	public void onNewIntent(Intent intent)
	{
		setIntent(intent);
	}
	
	/** Returns the data string from the intent, and resets the intent. */
	public String takeIntentPath()
	{
		String result = "";
		Intent intent = getIntent();
		if (intent != null)
		{
			String action = intent.getAction();
			if (action == Intent.ACTION_EDIT || action == Intent.ACTION_VIEW)
			{
				result = intent.getDataString();
			}
			setIntent(null);
		}
		return result;
	}
	
	/**
	 * Request android to disable battery optimization for Mapper.
	 * 
	 * "An app holding the REQUEST_IGNORE_BATTERY_OPTIMIZATIONS permission can
	 * trigger a system dialog to let the user add the app to the whitelist
	 * directly, without going to settings."
	 * 
	 * "Google Play policies prohibit apps from requesting direct exemption from
	 * Power Management features in Android 6.0+ (Doze and App Standby) unless
	 * the core function of the app is adversely affected."
	 * 
	 * @see https://developer.android.com/training/monitoring-device-state/doze-standby#support_for_other_use_cases
	 */
	void requestIgnoreBatteryOptimizations()
	{
		if (Build.VERSION.SDK_INT < 23)
		{
			return;
		}
		
		if (optimization_request_done)
		{
			return;
		}
		
		String app_id = BuildConfig.APPLICATION_ID;
		PowerManager pm = (PowerManager) getSystemService(POWER_SERVICE);
		if (pm != null && !pm.isIgnoringBatteryOptimizations(app_id))
		{
			Intent intent = new Intent();
			intent.setAction(Settings.ACTION_REQUEST_IGNORE_BATTERY_OPTIMIZATIONS);
			intent.setData(Uri.parse("package:" + app_id));
			startActivity(intent);
		}
		optimization_request_done = true;  // until app is restarted
	}
	
	
	// Static methods to be called from C++
	
	/** Checks if GPS is enabled in the Android settings and if not, prompts the user to enable it.
	 *  The dialog box works asynchronously, so the method cannot return the result. */
	static void checkGPSEnabled()
	{
		LocationManager locationManager = (LocationManager) instance.getSystemService(LOCATION_SERVICE);
		boolean enabled = locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER);
		if (!enabled)
		{
			instance.runOnUiThread(new Runnable() {
				public void run() {
					DialogInterface.OnClickListener dialogClickListener = new DialogInterface.OnClickListener() {
						@Override
						public void onClick(DialogInterface dialog, int which) {
							switch (which){
							case DialogInterface.BUTTON_POSITIVE:
								Intent intent = new Intent(Settings.ACTION_LOCATION_SOURCE_SETTINGS);
								instance.startActivity(intent);
								break;

							case DialogInterface.BUTTON_NEGATIVE:
								//No button clicked
								break;
							}
						}
					};

					AlertDialog.Builder builder = new AlertDialog.Builder(instance);
					builder.setMessage(instance.gps_disabled_string)
						.setPositiveButton(instance.yes_string, dialogClickListener)
						.setNegativeButton(instance.no_string, dialogClickListener)
						.show();
				}
			});
		}
	}
	
	public static void setTranslatableStrings(String yes_string, String no_string, String gps_disabled_string)
	{
		instance.yes_string = yes_string;
		instance.no_string = no_string;
		instance.gps_disabled_string = gps_disabled_string;
	}
	
	public static void showToast(final String message, final int duration)
	{
		instance.runOnUiThread(new Runnable() {
			public void run() {
				if (toast_reset != null)
					toast_reset.cancel();
				
				if (toast == null)
				{
					toast = Toast.makeText(instance, "", Toast.LENGTH_SHORT);
					toast.setGravity(Gravity.BOTTOM|Gravity.CENTER_HORIZONTAL, 0, 4);
				}
				
				toast.setText(message);
				toast.show();
				
				if (duration <= 0)
					return;
				
				toast_reset = new CountDownTimer(duration, 500) {
					public void onTick(long millisUntilFinished)
					{
						toast.show();
					}
					
					public void onFinish() {
						toast.cancel();
					}
				};
				toast_reset.start();
			}
		} );
	}
	
	public static void hideToast()
	{
		instance.runOnUiThread(new Runnable() {
			public void run() {
				if (toast_reset != null)
					toast_reset.cancel();
				if (toast != null)
					toast.cancel();
			}
		} );
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
	
	/** Starts a foreground service with the given notification message.
	 */
	public static void startService(String message)
	{
		if (!service_started)
		{
			service_started = true;
			Intent intent = new Intent(instance, MapperService.class);
			intent.putExtra("message", message);
			instance.startService(intent);
			instance.requestIgnoreBatteryOptimizations();
		}
	}
	
	/** Stops the foreground service.
	 */
	public static void stopService()
	{
		if (service_started)
		{
			Intent intent = new Intent(instance, MapperService.class);
			instance.stopService(intent);
			service_started = false;
		}
	}
}
