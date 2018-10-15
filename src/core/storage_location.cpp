/*
 *    Copyright 2016 Kai Pastor
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


#include "storage_location.h"

#include <cstddef>

#include <QtGlobal>
#include <QStandardPaths>
#include <QStringList>
#include <QTemporaryFile>


#ifdef Q_OS_ANDROID
#include <QtAndroid>
#include <QAndroidActivityResultReceiver>
#include <QAndroidJniEnvironment>
#include <QAndroidJniObject>
#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QVector>


namespace OpenOrienteering {

namespace Android {

/**
 * The cache of known locations.
 */
static std::shared_ptr<const std::vector<StorageLocation>> locations_cache;


void mediaScannerScanFile(const QString& path)
{
	static const auto ACTION_MEDIA_SCANNER_SCAN_FILE = 
	        QAndroidJniObject::getStaticObjectField<jstring>("android/content/Intent",
	                                                         "ACTION_MEDIA_SCANNER_SCAN_FILE");
	auto intent = QAndroidJniObject { "android/content/Intent",
	                                  "(Ljava/lang/String;)V",
	                                  ACTION_MEDIA_SCANNER_SCAN_FILE.object<jstring>() };
	auto file = QAndroidJniObject { "java/io/File",
	                                "(Ljava/lang/String;)V",
	                                QAndroidJniObject::fromString(path).object<jstring>() };
	auto uri  = QAndroidJniObject::callStaticObjectMethod(
	                "android/net/Uri",
	                "fromFile",
	                "(Ljava/io/File;)Landroid/net/Uri;",
	                file.object());
	            
	intent.callObjectMethod("setData",
	                        "(Landroid/net/Uri;)Landroid/content/Intent;",
	                        uri.object());
	auto activity = QtAndroid::androidActivity();
	auto context = activity.callObjectMethod(
	                   "getApplicationContext",
	                   "()Landroid/content/Context;");
	context.callMethod<void>("sendBroadcast", "(Landroid/content/Intent;)V", intent.object());
}

/**
 * Returns writable application-specific directories on all external storage volumes.
 * 
 * These directories are named "Android/data/PACKAGENAME" and use "synthesized
 * permissions", i.e. apps from the package may write to these folders without needing
 * explicit permissions. Other apps may read these files.
 * 
 * Files in this location will be deleted when the app is uninstalled.
 * 
 * Requires API level 19.
 * 
 * \see https://developer.android.com/reference/android/content/Context.html#getExternalFilesDirs(java.lang.String)
 */
std::vector<QString> getExternalFilesDirs(jstring* type)
{
	auto activity = QtAndroid::androidActivity();
	auto context = activity.callObjectMethod(
	                   "getApplicationContext",
	                   "()Landroid/content/Context;");
	
	// Get directories with synthesized permissions on all external storage volumes
	auto external_files_dirs = context.callObjectMethod(
	                               "getExternalFilesDirs",
	                               "(Ljava/lang/String;)[Ljava/io/File;",
	                               type);
	
	QAndroidJniEnvironment jni;
	const auto length = jni->GetArrayLength(external_files_dirs.object<jarray>());
	
	std::vector<QString> locations;
	locations.reserve(std::size_t(length));
	for (auto i = 0; i < length; ++i)
	{
		auto location_jni = jni->GetObjectArrayElement(external_files_dirs.object<jobjectArray>(), i);
		auto location = QAndroidJniObject{ location_jni }.toString();
		
		const auto warning_path = QString(location + QLatin1String("/README.html"));
		QFile warning(warning_path);
		if (warning.open(QIODevice::WriteOnly | QIODevice::Truncate))
		{
			auto android_start = location.indexOf(QLatin1String("/Android/data/"));
			warning.write("<html><body><p>\n");
			warning.write(StorageLocation::fileHintTextTemplate(StorageLocation::HintApplication).arg(location.mid(android_start+1)).toUtf8());
			warning.write("\n</p></body></html>");
			warning.close();
			locations.push_back(location);
			mediaScannerScanFile(warning_path);
		}
	}
	return locations;
}

/**
 * Constructs a list of OOMapper folders on secondary storage volumes
 */
std::vector<QString> getLegacySecondaryStorage(const QString& primary_storage, const std::vector<QString>& external_files_dirs)
{
	std::vector<QString> result;
	result.reserve(external_files_dirs.size());
	for (const auto& dir : external_files_dirs)
	{
		if (dir.startsWith(primary_storage))
			continue;
		
		auto index = dir.indexOf(QLatin1String("Android/data/"));
		if (index < 0)
			continue;
		
		result.push_back(dir.leftRef(index) + QLatin1String("OOMapper"));
	}
	return result;
}

/**
 * Constructs the cache of known storage locations.
 */
std::shared_ptr<const std::vector<StorageLocation>> buildLocationCache()
{
	std::vector<QString> locations_normal;
	std::vector<QString> locations_application;
	std::vector<QString> locations_readonly;
	
	// API level 1, single primary external storage
	auto primary_storage = QAndroidJniObject::callStaticObjectMethod(
	                           "android/os/Environment",
	                           "getExternalStorageDirectory",
	                           "()Ljava/io/File;",
	                           nullptr).toString();
	
	// Easy: "OOMapper" folder on primary external storage.
	// Write access depends on the WRITE_EXTERNAL_STORAGE permission, which
	// may be revoked by the user since Android 6.0. (Read access depends on the
	// READ_EXTERNAL_STORAGE permission only when write access is not given.)
	const QFileInfo primary_storage_oomapper { primary_storage + QLatin1String("/OOMapper") };
	if (primary_storage_oomapper.exists())
	{
		const auto path = primary_storage_oomapper.filePath();
		if (primary_storage_oomapper.isWritable())
			locations_normal.push_back(path);
		else
			locations_readonly.push_back(path);
		mediaScannerScanFile(path);
	}
	
	// Volatile: Application-specific directories on external storage.
	if (QtAndroid::androidSdkVersion() >= 19)
	{
		// API level 19
		locations_application = Android::getExternalFilesDirs(nullptr);
	}
	
	// Difficult: "OOMapper" folder on secondary external storage.
	// Read access, but (normally) no write access.
	std::vector<QString> secondary_storage_paths;
	const auto env = QProcessEnvironment::systemEnvironment();
	auto env_secondary_storage = env.value(QLatin1String("SECONDARY_STORAGE"));
	if (!env_secondary_storage.isEmpty())
	{
		// Mapper legacy approach, API level < 19
		const auto paths = env_secondary_storage.splitRef(QLatin1Char{';'});
		secondary_storage_paths.reserve(std::size_t(paths.size()));
		for (const auto& path : paths)
			secondary_storage_paths.emplace_back(path + QLatin1String("/OOMapper"));
	}
	else if (!locations_application.empty())
	{
		// API level 19
		secondary_storage_paths = Android::getLegacySecondaryStorage(primary_storage, locations_application);
	}
	for (const auto& path : secondary_storage_paths)
	{
		const QFileInfo secondary_storage { path };
		if (secondary_storage.exists())
		{
			if (secondary_storage.isWritable())
				locations_normal.push_back(path);
			else
				locations_readonly.push_back(path);
			mediaScannerScanFile(path);
		}
	}
	
	auto locations = std::make_shared<std::vector<StorageLocation>>();
	locations->reserve(locations_normal.size()
	                   + locations_application.size()
	                   + locations_readonly.size());
	for (const auto& path : locations_normal)
		locations->emplace_back(path, StorageLocation::HintNormal);
	for (const auto& path : locations_application)
		locations->emplace_back(path, StorageLocation::HintApplication);
	for (const auto& path : locations_readonly)
		locations->emplace_back(path, StorageLocation::HintReadOnly);
	
	return locations;
}


}  // namespace Android

}  // namespace OpenOrienteering

#endif


namespace OpenOrienteering {

// static
std::shared_ptr<const std::vector<StorageLocation>> StorageLocation::knownLocations()
{
#ifdef Q_OS_ANDROID
	auto locations = Android::locations_cache;
	if (!locations)
	{
		locations = Android::buildLocationCache();
		Android::locations_cache = locations;
	}
	Q_ASSERT(locations);
	return locations;
#else
	auto locations = std::make_shared<std::vector<StorageLocation>>();
	auto paths = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
	locations->reserve(std::size_t(paths.size()));
	for (const auto& path : paths)
	{
		locations->emplace_back(path, HintNormal);
	}
	return locations;
#endif
}


void StorageLocation::refresh()
{
#ifdef Q_OS_ANDROID
	Android::locations_cache.reset();
#endif	
}


// static
QString StorageLocation::fileHintTextTemplate(Hint hint)
{
	switch (hint)
	{
	case HintNormal:
		return {};  // No text for a regular location.
		
	case HintApplication:
		return tr("'%1' is located in app storage. The files will be removed when uninstalling the app.");
		
	case HintReadOnly:
		return tr("'%1' is not writable. Changes cannot be saved.");
		
	case HintInvalid:
		return tr("'%1' is not a valid storage location.");
	}
	
	Q_UNREACHABLE();
}


QString OpenOrienteering::StorageLocation::hintText() const
{
	return hint() == HintNormal ? QString{} : fileHintTextTemplate(hint()).arg(path());
}


}  // namespace OpenOrienteering
