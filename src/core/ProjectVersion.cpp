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


namespace lmms
{


ProjectVersion::ProjectVersion(QString version, CompareType c) :
	m_version(version),
	m_compareType(c)
{
	// Version numbers may have build data, prefixed with a '+',
	// but this mustn't affect version precedence in comparisons
	QString metadataStripped = version.split("+").first();
	// They must have an obligatory initial segment, and may have
	// optional identifiers prefaced by a '-'. Both parts affect precedence
	QString obligatorySegment = metadataStripped.section('-', 0, 0);
	QString prereleaseSegment = metadataStripped.section('-', 1);

	// The obligatory segment consists of three identifiers: MAJOR.MINOR.PATCH
	QStringList mainVersion = obligatorySegment.split(".");
	// HACK: Pad invalid versions in order to prevent crashes
	while (mainVersion.size() < 3){ mainVersion.append("0"); }
	m_major = mainVersion.at(0).toInt();
	m_minor = mainVersion.at(1).toInt();
	m_patch = mainVersion.at(2).toInt();

	// Any # of optional pre-release identifiers may follow, separated by '.'s
	if (!prereleaseSegment.isEmpty()){ m_labels = prereleaseSegment.split("."); }

	// HACK: Handle old (1.2.2 and earlier), non-standard versions of the form
	// MAJOR.MINOR.PATCH.COMMITS, used for non-release builds from source.
	if (mainVersion.size() >= 4 && m_major <= 1 && m_minor <= 2 && m_patch <= 2){
		// Drop the standard version identifiers. erase(a, b) removes [a,b)
		mainVersion.erase(mainVersion.begin(), mainVersion.begin() + 3);
		// Prepend the remaining identifiers as prerelease versions
		m_labels = mainVersion + m_labels;
		// Bump the patch version. x.y.z-a < x.y.z, but we want x.y.z.a > x.y.z
		m_patch += 1;
	}
}




ProjectVersion::ProjectVersion(const char* version, CompareType c) : ProjectVersion(QString(version), c)
{
}




//! @param c Determines the number of identifiers to check when comparing
int ProjectVersion::compare(const ProjectVersion & a, const ProjectVersion & b, CompareType c)
{
	// How many identifiers to compare before we consider the versions equal
	const int limit = static_cast<int>(c);

	// Use the value of limit to zero out identifiers we don't care about
	int aMaj = 0, bMaj = 0, aMin = 0, bMin = 0, aPat = 0, bPat = 0;
	if (limit >= 1){ aMaj = a.getMajor(); bMaj = b.getMajor(); }
	if (limit >= 2){ aMin = a.getMinor(); bMin = b.getMinor(); }
	if (limit >= 3){ aPat = a.getPatch(); bPat = b.getPatch(); }

	// Then we can compare as if we care about every identifier
	if(aMaj != bMaj){ return aMaj - bMaj; }
	if(aMin != bMin){ return aMin - bMin; }
	if(aPat != bPat){ return aPat - bPat; }

	// Decide how many optional identifiers we care about
	const int maxLabels = std::max(0, limit - 3);
	const auto aLabels = a.getLabels().mid(0, maxLabels);
	const auto bLabels = b.getLabels().mid(0, maxLabels);

	// We can only compare identifiers if both versions have them
	const int commonLabels = std::min(aLabels.size(), bLabels.size());
	// If one version has optional labels and the other doesn't,
	// the one without them is bigger
	if (commonLabels == 0){ return bLabels.size() - aLabels.size(); }

	// Otherwise, compare as many labels as we can
	for (int i = 0; i < commonLabels; i++){
		const QString& labelA = aLabels.at(i);
		const QString& labelB = bLabels.at(i);
		// If both labels are the same, skip
		if (labelA == labelB){ continue; }
		// Numeric and non-numeric identifiers compare differently
		bool aIsNumeric = false, bIsNumeric = false;
		const int numA = labelA.toInt(&aIsNumeric);
		const int numB = labelB.toInt(&bIsNumeric);
		// toInt reads '-x' as a negative number, semver says it's non-numeric
		aIsNumeric &= !labelA.startsWith("-");
		bIsNumeric &= !labelB.startsWith("-");
		// If only one identifier is numeric, that one is smaller
		if (aIsNumeric != bIsNumeric){ return aIsNumeric ? -1 : 1; }
		// If both are numeric, compare as numbers
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


} // namespace lmms