/*
 * ProjectVersion.cpp - compare versions in import upgrades
 *
 * Copyright (c) 2007 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2015 Tres Finocchiaro <tres.finocchiaro/at/gmail.com>
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


#include "ProjectVersion.h"




ProjectVersion::ProjectVersion(QString version, CompareType c) :
	m_version(version),
	m_compareType(c)
{
	// Version numbers may have build data, prefixed with a '+',
	// but this mustn't affect version precedence in comparisons
	QString metadataStripped = version.split("+").first();
	// They must have an obligatory initial segement, and may have
	// optional identifiers prefaced by a '-'. Both parts affect precedence
	QStringList mainAndLabels = metadataStripped.split("-");

	// The obligatory segment consists of three identifiers: MAJOR.MINOR.PATCH
	QStringList mainVersion = mainAndLabels.first().split(".");
	m_major = mainVersion.at(0).toInt();
	m_minor = mainVersion.at(1).toInt();
	m_patch = mainVersion.at(2).toInt();

	// Any # of optional pre-release identifiers may follow, separated by '.'s
	if (mainAndLabels.size() >= 2){ m_labels = mainAndLabels.at(1).split("."); }
}




ProjectVersion::ProjectVersion(const char* version, CompareType c) : ProjectVersion(QString(version), c)
{
}




//! @param c The number of identifiers to check when comparing
int ProjectVersion::compare(const ProjectVersion & a, const ProjectVersion & b, int c)
{
	// Use the value of c to zero out identifiers we don't care about
	int aMaj = 0, bMaj = 0, aMin = 0, bMin = 0, aPat = 0, bPat = 0;
	if (c >= 1){ aMaj = a.getMajor(); bMaj = b.getMajor(); }
	if (c >= 2){ aMin = a.getMinor(); bMin = b.getMinor(); }
	if (c >= 3){ aPat = a.getPatch(); bPat = b.getPatch(); }

	// Then compare as if we care about every identifiers
	if(aMaj != bMaj){ return aMaj - bMaj; }
	if(aMin != bMin){ return aMin - bMin; }
	if(aPat != bPat){ return aPat - bPat; }

	// Decide how many optional identifiers we care about
	int numLabels = qMax(0, c - 3);
	auto aLabels = a.getLabels().mid(0, numLabels);
	auto bLabels = b.getLabels().mid(0, numLabels);

	// We can only compare identifiers if both versions have them
	int commonLabels = qMin(aLabels.size(), bLabels.size());
	// If one version has optional labels and the other doesn't,
	// the one without them is bigger
	if (commonLabels == 0){ return bLabels.size() - aLabels.size(); }

	// Otherwise, compare as many labels as we can
	for (int i = 0; i < commonLabels; i++){
		QString labelA = aLabels.at(i);
		QString labelB = bLabels.at(i);
		// If both labels are the same, skip
		if (labelA == labelB){ continue; }
		// Else if both labels are numeric, compare them numerically
		bool aIsNumeric = false, bIsNumeric = false;
		int numA = labelA.toInt(&aIsNumeric);
		int numB = labelB.toInt(&bIsNumeric);
		if (aIsNumeric && bIsNumeric){ return numA - numB; }
		// Otherwise, compare lexically
		return labelA.compare(labelB);
	}

	// If everything else matches, the version with more labels is bigger
	return aLabels.size() - bLabels.size();
}




int ProjectVersion::compare(ProjectVersion v1, ProjectVersion v2)
{
	return compare(v1, v2, std::min(v1.getCompareType(), v2.getCompareType()));
}
