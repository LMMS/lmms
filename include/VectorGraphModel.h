/*
 * VectorGraphModel.h - model for vector based graph
 *
 * Copyright (c) 2025 - 2026 szeli1
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
*/

#ifndef LMMS_VECTOR_GRAPH_MODEL_H
#define LMMS_VECTOR_GRAPH_MODEL_H

#include <set>
#include <mutex>

#include "GridModel.h"
#include "JournallingObject.h"

namespace lmms
{

struct VGPoint
{
	enum Type
	{
		bezier, //!< the normal point
		attribute, //!< used to adjust other's attributes
		sine, //!< makes a sine shaped line
		peak, //!< makes a peak filter shaped line
		steps, //!< makes a staircase
		count
	};
	Type type;
};

class LMMS_EXPORT VectorGraphModel : public GridModelTyped<VGPoint, VGPoint>, public JournallingObject
{
Q_OBJECT
public:
	VectorGraphModel(unsigned int length, unsigned int height, unsigned int horizontalSteps, unsigned int verticalSteps,
		size_t bufferSize, Model* parent, QString displayName = QString(), bool defaultConstructed = false);

	void renderAllTo(std::vector<float>& bufferOut);
	void renderChangedPoints();
	//! @param index from where to update the line
	//! @param updatedTo the index until the line was updated
	void renderAfter(size_t index, std::vector<float>& buffer, size_t* updatedTo = nullptr);
	void renderStart(std::vector<float>& buffer);

	//! these automatically update the buffer when dataChanged
	const std::vector<float>& getBuffer();
	std::vector<float>& getBufferRef();
	void setRenderSize(size_t newSize);

	void saveSettings(QDomDocument& doc, QDomElement& element, const QString& name);
	void loadSettings(const QDomElement& element, const QString& name);
	QString nodeName() const override
	{
		return "vectorgraphmodel";
	}

	void clear();
	QString getPointsBase64(float xOffset, float yOffset, const std::set<size_t>* selection = nullptr);
	void addPointsBase64(QString base64String, float xOffset, float yOffset);
protected:
	void dataChangedAt(ssize_t index) override;

	void saveSettings(QDomDocument& doc, QDomElement& element) override;
	void loadSettings(const QDomElement& element) override;
private:
	void processLineTypeBezier(std::vector<float>& samplesOut, size_t startLoc, size_t endLoc,
		float yBefore, float yAfter, float yMid);
	void processLineTypeSine(std::vector<float>& samplesOut, size_t startLoc, size_t endLoc,
		float sineAmp, float sineFreq, float sinePhase, float fadeInStartVal);
	void processLineTypePeak(std::vector<float>& samplesOut, size_t startLoc, size_t endLoc,
		float peakAmp, float peakX, float peakWidth, float fadeInStartVal);
	void processLineTypeSteps(std::vector<float>& samplesIO, size_t startLoc, size_t endLoc,
		float stepHeight, float stepAmp, float yBefore, float fadeInStartVal);
	void processLineTypeLinInterpolate(std::vector<float>& samplesOut, size_t startLoc, size_t endLoc,
		float startY, float endY, bool shouldOverride);
	void processLineTypeFade(std::vector<float>& samplesOut, size_t startLoc, size_t endLoc,
		float fadeInStartVal);

	std::vector<float> m_buffer;
	bool m_allChanged;
	std::set<size_t> m_changedData;
	std::mutex m_setAndBufferMutex;
};

} // namespace lmms

#endif // LMMS_VECTOR_GRAPH_MODEL_H
