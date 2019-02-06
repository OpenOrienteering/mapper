/*
 *    Copyright 2014, 2017 Kai Pastor
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

#ifndef OPENORIENTEERING_AUTO_SAVE_T_H
#define OPENORIENTEERING_AUTO_SAVE_T_H

#include <QObject>

#include "core/autosave.h"

using namespace OpenOrienteering;


/**
 * @test A simple document class for AutosaveTest.
 */
class AutosaveTestDocument : public Autosave
{
public:
	/**
	 * @brief Constructs a document with the given expected autosave() result.
	 */
	AutosaveTestDocument(Autosave::AutosaveResult expected_result);
	
	~AutosaveTestDocument() override = default;
	
	/**
	 * @brief Simulates autosaving by returning the current expected_result.
	 * 
	 * If the result as returned by the current invocation is
	 * Autosave::TemporaryFailure, the expected result (as returned by the next
	 * invocation) will be changed to Autosave::Success.
	 */
	Autosave::AutosaveResult autosave() override;
	
	/**
	 * @brief Returns the number of invocations of autosave().
	 */
	int autosaveCount() const;
	
private:
	/**
	 * @brief The result to be returned from the next invocation of autosave().
	 */
	Autosave::AutosaveResult expected_result;
	
	/**
	 * @brief The number of invocations of autosave().
	 */
	int autosave_count;
};


/**
 * @test Tests the autosave feature.
 */
class AutosaveTest : public QObject
{
Q_OBJECT
public:
	/** @brief Constructor */
	explicit AutosaveTest(QObject* parent = nullptr);
	
private slots:
	/** @brief Check the AutosaveTestDocument implementation of autosave(). */
	void autosaveTestDocumentTest();
	
	/** @brief Tests successful autosaving. */
	void successTest();
	
	/** @brief Tests temporary failing autosaving. */
	void temporaryFailureTest();
	
	/** @brief Tests permanent failing autosaving. */
	void permanentFailureTest();
	
	/** @brief Tests autosave stopping on normal saving. */
	void autosaveStopTest();
	
protected:
	/** @brief The autosave interval, unit: minutes. */
	const double autosave_interval;
};

#endif
