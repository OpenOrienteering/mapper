/*
 *    Copyright 2014 Kai Pastor
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

#include "auto_save_t.h"

#include "../src/settings.h"


//### AutoSaveTestDocument ###

AutoSaveTestDocument::AutoSaveTestDocument(AutoSave::AutoSaveResult expected_result)
: expected_result(expected_result),
  auto_save_count(0)
{
	// nothing
}

AutoSave::AutoSaveResult AutoSaveTestDocument::autoSave()
{
	++auto_save_count;
	
	AutoSave::AutoSaveResult result = expected_result;
	if (result == AutoSave::TemporaryFailure)
		expected_result = AutoSave::Success; // success on next call
	
	return result;
}

int AutoSaveTestDocument::autoSaveCount() const
{
	return auto_save_count;
}


//### AutoSaveTest ###

AutoSaveTest::AutoSaveTest(QObject* parent)
: QObject(parent),
  auto_save_interval(0.02) // 0.02 min = 1.2 s
{
	// Set distinct application name in order to use distinct settings.
	// AutoSave objects must not be constructed before this!
	QCoreApplication::setApplicationName("Tests");
}

void AutoSaveTest::autoSaveTestDocumentTest()
{
	{
		AutoSaveTestDocument doc(AutoSave::Success);
		QCOMPARE(doc.autoSaveCount(), 0);
		QCOMPARE(doc.autoSave(), AutoSave::Success);
		QCOMPARE(doc.autoSaveCount(), 1);
	}
	
	{
		AutoSaveTestDocument doc(AutoSave::TemporaryFailure);
		QCOMPARE(doc.autoSave(), AutoSave::TemporaryFailure);
		QCOMPARE(doc.autoSave(), AutoSave::Success);
		QCOMPARE(doc.autoSave(), AutoSave::Success);
		QCOMPARE(doc.autoSaveCount(), 3);
	}
	
	{
		AutoSaveTestDocument doc(AutoSave::PermanentFailure);
		QCOMPARE(doc.autoSave(), AutoSave::PermanentFailure);
		QCOMPARE(doc.autoSave(), AutoSave::PermanentFailure);
		QCOMPARE(doc.autoSaveCount(), 2);
	}
}

void AutoSaveTest::successTest()
{
	AutoSaveTestDocument doc(AutoSave::Success);
	
	// Enable and trigger autosave
	Settings::getInstance().setSetting(Settings::General_AutoSaveInterval, auto_save_interval);
	doc.setAutoSaveNeeded(true);
	
	// Verify that autosave does not trigger too early
	QThread::msleep(auto_save_interval * 50000); 
	QCoreApplication::processEvents();
	QCOMPARE(doc.autoSaveCount(), 0);
	
	// Verify that autosave does trigger exactly once before too late
	QThread::msleep(auto_save_interval * 20000);
	QCoreApplication::processEvents();
	QCOMPARE(doc.autoSaveCount(), 1);
	
	// Verify that autosave does not trigger again too early
	QThread::msleep(auto_save_interval * 50000); 
	QCoreApplication::processEvents();
	QCOMPARE(doc.autoSaveCount(), 1);
	
	// Verify that autosave does trigger again once before too late
	QThread::msleep(auto_save_interval * 20000);
	QCoreApplication::processEvents();
	QCOMPARE(doc.autoSaveCount(), 2);
}

void AutoSaveTest::temporaryFailureTest()
{
	AutoSaveTestDocument doc(AutoSave::TemporaryFailure);
	
	// Enable and trigger autosave
	Settings::getInstance().setSetting(Settings::General_AutoSaveInterval, auto_save_interval);
	doc.setAutoSaveNeeded(true);
	
	// Verify that autosave does not trigger too early
	QThread::msleep(auto_save_interval * 50000); 
	QCoreApplication::processEvents();
	QCOMPARE(doc.autoSaveCount(), 0);
	
	// Verify that autosave does trigger exactly once before too late
	QThread::msleep(auto_save_interval * 20000);
	QCoreApplication::processEvents();
	QCOMPARE(doc.autoSaveCount(), 1);
	
	// Verify that autosave does trigger once more quickly
	QThread::msleep(10000);
	QCoreApplication::processEvents();
	QCOMPARE(doc.autoSaveCount(), 2);
	
	// Verify that autosave does not trigger again too early
	QThread::msleep(auto_save_interval * 50000); 
	QCoreApplication::processEvents();
	QCOMPARE(doc.autoSaveCount(), 2);
	
	// Verify that autosave does trigger again once before too late
	QThread::msleep(auto_save_interval * 20000);
	QCoreApplication::processEvents();
	QCOMPARE(doc.autoSaveCount(), 3);
}

void AutoSaveTest::permanentFailureTest()
{
	AutoSaveTestDocument doc(AutoSave::PermanentFailure);
	
	// Enable and trigger autosave
	Settings::getInstance().setSetting(Settings::General_AutoSaveInterval, auto_save_interval);
	doc.setAutoSaveNeeded(true);
	
	// Verify that autosave does not trigger too early
	QThread::msleep(auto_save_interval * 50000); 
	QCoreApplication::processEvents();
	QCOMPARE(doc.autoSaveCount(), 0);
	
	// Verify that autosave does trigger exactly once before too late
	QThread::msleep(auto_save_interval * 20000);
	QCoreApplication::processEvents();
	QCOMPARE(doc.autoSaveCount(), 1);
	
	// Verify that autosave does not trigger again too early
	QThread::msleep(auto_save_interval * 50000); 
	QCoreApplication::processEvents();
	QCOMPARE(doc.autoSaveCount(), 1);
	
	// Verify that autosave does trigger again once before too late
	QThread::msleep(auto_save_interval * 20000);
	QCoreApplication::processEvents();
	QCOMPARE(doc.autoSaveCount(), 2);
}

void AutoSaveTest::autoSaveStopTest()
{
	{
		AutoSaveTestDocument doc(AutoSave::Success);
		
		// Enable and trigger autosave
		Settings::getInstance().setSetting(Settings::General_AutoSaveInterval, auto_save_interval);
		doc.setAutoSaveNeeded(true);
		
		// Sleep some time
		QThread::msleep(auto_save_interval * 20000); 
		QCoreApplication::processEvents();
		QCOMPARE(doc.autoSaveCount(), 0);
		
		// Verify that autosave does not trigger after setAutoSaveNeeded(false)
		doc.setAutoSaveNeeded(false);
		QThread::msleep(auto_save_interval * 70000); 
		QCoreApplication::processEvents();
		QCOMPARE(doc.autoSaveCount(), 0);
	}
	
	{
		AutoSaveTestDocument doc(AutoSave::TemporaryFailure);
		
		// Test stopping auto-save in temporary failure mode
		doc.setAutoSaveNeeded(true);
		QThread::msleep(auto_save_interval * 70000); 
		QCoreApplication::processEvents();
		QCOMPARE(doc.autoSaveCount(), 1);
		
		doc.setAutoSaveNeeded(false);
		QThread::msleep(auto_save_interval * 20000); 
		QCoreApplication::processEvents();
		QCOMPARE(doc.autoSaveCount(), 1);
	}
	
	{
		AutoSaveTestDocument doc(AutoSave::PermanentFailure);
		
		// Test stopping auto-save in permament failure mode
		doc.setAutoSaveNeeded(true);
		QThread::msleep(auto_save_interval * 70000); 
		QCoreApplication::processEvents();
		QCOMPARE(doc.autoSaveCount(), 1);
		
		doc.setAutoSaveNeeded(false);
		QThread::msleep(auto_save_interval * 70000); 
		QCoreApplication::processEvents();
		QCOMPARE(doc.autoSaveCount(), 1);
	}
}

QTEST_MAIN(AutoSaveTest)
