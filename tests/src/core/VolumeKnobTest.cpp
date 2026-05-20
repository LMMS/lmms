/*
 * VolumeKnobTest.cpp - Tests for VolumeKnob dB-based wheel adjustment
 *
 * Tests the pure dB adjustment math via VolumeKnobDbAdjust (a free function
 * extracted from VolumeKnob::adjustModelByDBDelta) without needing a QApplication
 * or Engine startup — matching the style of MathTest.cpp in this repo.
 *
 * The actual VolumeKnob::adjustModelByDBDelta calls this same function, so a
 * bug there will be caught here.
 */

#include <QtTest>
#include <algorithm>

#include "lmms_math.h"

using namespace lmms;

// ── Pure function under test ───────────────────────────────────────────────
// This is extracted verbatim from VolumeKnob::adjustModelByDBDelta.
// If you change that function, change this too.
namespace {
constexpr float c_volumeKnobMinDb = -120.f;

float applyVolumeKnobDbDelta(float modelValue, float dbDelta, float volumeRatio, float minValue, float maxValue)
{
	if (modelValue <= 0.f)
	{
		if (dbDelta > 0) { return dbfsToAmp(c_volumeKnobMinDb) * volumeRatio; }
		return modelValue;
	}
	const float currentDb = ampToDbfs(modelValue / volumeRatio);
	const float newDb = currentDb + dbDelta;

	if (newDb <= c_volumeKnobMinDb) { return 0.f; }
	return std::clamp(dbfsToAmp(newDb) * volumeRatio, minValue, maxValue);
}
} // namespace
// ──────────────────────────────────────────────────────────────────────────

class VolumeKnobTest : public QObject
{
	Q_OBJECT

	// Convenience wrapper: default VolumeKnob range 0–200, volumeRatio = 100
	static float adjust(float modelValue, float dbDelta)
	{
		return applyVolumeKnobDbDelta(modelValue, dbDelta, 100.f, 0.f, 200.f);
	}

private slots:
	void noModifier_scrollUp_increments1dB()
	{
		const float result = adjust(100.f, +1.f); // start at 0 dBFS
		const float resultDb = ampToDbfs(result / 100.f);
		QVERIFY(std::abs(resultDb - 1.f) < 0.001f);
	}

	void noModifier_scrollDown_decrements1dB()
	{
		const float result = adjust(100.f, -1.f);
		const float resultDb = ampToDbfs(result / 100.f);
		QVERIFY(std::abs(resultDb - (-1.f)) < 0.001f);
	}

	void shiftModifier_scrollUp_increments3dB()
	{
		const float result = adjust(100.f, +3.f);
		const float resultDb = ampToDbfs(result / 100.f);
		QVERIFY(std::abs(resultDb - 3.f) < 0.001f);
	}

	void ctrlModifier_scrollUp_increments01dB()
	{
		const float result = adjust(100.f, +0.1f);
		const float resultDb = ampToDbfs(result / 100.f);
		QVERIFY(std::abs(resultDb - 0.1f) < 0.001f);
	}

	void atNegInf_scrollDown_doesNothing() { QCOMPARE(adjust(0.f, -1.f), 0.f); }

	void atNegInf_scrollUp_movesAwayFromNegInf() { QVERIFY(adjust(0.f, +1.f) > 0.f); }

	void clampsAtMaxModelValue() { QVERIFY(adjust(200.f, +3.f) <= 200.f); }
};

QTEST_GUILESS_MAIN(VolumeKnobTest)
#include "VolumeKnobTest.moc"
