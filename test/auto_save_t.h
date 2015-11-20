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

#ifndef _OPENORIENTEERING_AUTO_SAVE_T_H
#define _OPENORIENTEERING_AUTO_SAVE_T_H

#include <QtTest/QtTest>

#include "../src/core/auto_save.h"


/**
 * @test A simple document class for AutoSaveTest.
 */
class AutoSaveTestDocument : public AutoSave
{
public:
	/**
	 * @brief Constructs a document with the given expected autoSave() result.
	 */
	AutoSaveTestDocument(AutoSave::AutoSaveResult expected_result);
	
	/**
	 * @brief Simulates auto-saving by returning the current expected_result.
	 * 
	 * If the result as returned by the current invocation is
	 * AutoSave::TemporaryFailure, the expected result (as returned by the next
	 * invocation) will be changed to AutoSave::Success.
	 */
	virtual AutoSave::AutoSaveResult autoSave();
	
	/**
	 * @brief Returns the number of invocations of autoSave().
	 */
	int autoSaveCount() const;
	
private:
	/**
	 * @brief The result to be returned from the next invocation of autoSave().
	 */
	AutoSave::AutoSaveResult expected_result;
	
	/**
	 * @brief The number of invocations of autoSave().
	 */
	int auto_save_count;
};


/**
 * @test Tests the auto-save feature.
 */
class AutoSaveTest : public QObject
{
Q_OBJECT
public:
	/** @brief Constructor */
	explicit AutoSaveTest(QObject* parent = NULL);
	
private slots:
	/** @brief Check the AutoSaveTestDocument implementation of autoSave(). */
	void autoSaveTestDocumentTest();
	
	/** @brief Tests successfull auto-saving. */
	void successTest();
	
	/** @brief Tests temporary failing auto-saving. */
	void temporaryFailureTest();
	
	/** @brief Tests permanent failing auto-saving. */
	void permanentFailureTest();
	
	/** @brief Tests auto-save stopping on normal saving. */
	void autoSaveStopTest();
	
protected:
	/** @brief The auto-save interval, unit: minutes. */
	const double auto_save_interval;
};

#endif
