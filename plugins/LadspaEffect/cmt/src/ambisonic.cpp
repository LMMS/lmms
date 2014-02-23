/* ambisonic.cpp

   Computer Music Toolkit - a library of LADSPA plugins. Copyright (C)
   2000-2002 Richard W.E. Furse. The author may be contacted at
   richard@muse.demon.co.uk.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public Licence as
   published by the Free Software Foundation; either version 2 of the
   Licence, or (at your option) any later version.

   This library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307, USA. */

/*****************************************************************************/

/* This module provides simple plugins handling B-Format and
   FMH-Format audio. Ambisonics is a mathematical technique designed
   to capture the sound field around point. See
   http://www.muse.demon.co.uk/3daudio.html. "Ambisonics" is a
   registered trademark of Nimbus Communications International
   although. An exteremly `vanilla' approach is taken to encoding
   distance, using inverse square, but no filtering or delay. */

/*****************************************************************************/

#include <math.h>
#include <stdlib.h>
#include <string.h>

/*****************************************************************************/

#include "cmt.h"

/*****************************************************************************/

#define ENC_INPUT 0
#define ENC_IN_X  1
#define ENC_IN_Y  2
#define ENC_IN_Z  3
#define ENC_OUT_W 4
#define ENC_OUT_X 5
#define ENC_OUT_Y 6
#define ENC_OUT_Z 7

#define ENC_OUT_R 8
#define ENC_OUT_S 9
#define ENC_OUT_T 10
#define ENC_OUT_U 11
#define ENC_OUT_V 12

/*****************************************************************************/

/** This plugin encodes a signal to B-Format depending on where it is
    located in a virtual space. */
class BFormatEncoder : public CMT_PluginInstance {
public:
  BFormatEncoder(const LADSPA_Descriptor *,
		 unsigned long lSampleRate)
    : CMT_PluginInstance(8) {
  }
  friend void runBFormatEncoder(LADSPA_Handle Instance,
				unsigned long SampleCount);
};

/** This plugin encodes a signal to FMH-Format depending on where it
    is located in a virtual space. */
class FMHFormatEncoder : public CMT_PluginInstance {
public:
  FMHFormatEncoder(const LADSPA_Descriptor *,
		 unsigned long lSampleRate)
    : CMT_PluginInstance(13) {
  }
  friend void runFMHFormatEncoder(LADSPA_Handle Instance,
				  unsigned long SampleCount);
};

/*****************************************************************************/

#define F2B_IN_W  0
#define F2B_IN_X  1
#define F2B_IN_Y  2
#define F2B_IN_Z  3
#define F2B_IN_R  4
#define F2B_IN_S  5
#define F2B_IN_T  6
#define F2B_IN_U  7
#define F2B_IN_V  8
#define F2B_OUT_W 9
#define F2B_OUT_X 10
#define F2B_OUT_Y 11
#define F2B_OUT_Z 12

/** This plugin coverts FMH-Format to B-Format. This is a trivial
    operation that can also be achieved simply by discarding RSTUV
    channels. */
class FMHToB : public CMT_PluginInstance {
public:
  FMHToB(const LADSPA_Descriptor *,
	 unsigned long lSampleRate)
    : CMT_PluginInstance(13) {
  }
  friend void runFMHToB(LADSPA_Handle Instance,
			unsigned long SampleCount);
};

/*****************************************************************************/

#define DECST_IN_W  0
#define DECST_IN_X  1
#define DECST_IN_Y  2
#define DECST_IN_Z  3
#define DECST_OUT_L 4
#define DECST_OUT_R 5

/** This plugin decodes B-Format to produce a stereo speaker feed. */
class BFormatToStereo : public CMT_PluginInstance {
public:
  BFormatToStereo(const LADSPA_Descriptor *,
		  unsigned long lSampleRate)
    : CMT_PluginInstance(6) {
  }
  friend void runBFormatToStereo(LADSPA_Handle Instance,
				 unsigned long SampleCount);
};

/*****************************************************************************/

#define DECQ_IN_W   0
#define DECQ_IN_X   1
#define DECQ_IN_Y   2
#define DECQ_IN_Z   3
#define DECQ_OUT_FL 4
#define DECQ_OUT_FR 5
#define DECQ_OUT_BL 6
#define DECQ_OUT_BR 7

/** This plugin decodes B-Format to produce a quad (square) speaker feed. */
class BFormatToQuad : public CMT_PluginInstance {
public:
  BFormatToQuad(const LADSPA_Descriptor *,
		unsigned long lSampleRate)
    : CMT_PluginInstance(8) {
  }
  friend void runBFormatToQuad(LADSPA_Handle Instance,
			       unsigned long SampleCount);
};

/*****************************************************************************/

#define DECC_IN_W    0
#define DECC_IN_X    1
#define DECC_IN_Y    2
#define DECC_IN_Z    3
#define DECC_OUT_BFL 4
#define DECC_OUT_BFR 5
#define DECC_OUT_BBL 6
#define DECC_OUT_BBR 7
#define DECC_OUT_TFL 8
#define DECC_OUT_TFR 9
#define DECC_OUT_TBL 10
#define DECC_OUT_TBR 11

/** This plugin decodes B-Format to produce a speaker feed for eight
    speakers arranged at the corners of a cube. */
class BFormatToCube : public CMT_PluginInstance {
public:
  BFormatToCube(const LADSPA_Descriptor *,
		unsigned long lSampleRate)
    : CMT_PluginInstance(12) {
  }
  friend void runBFormatToCube(LADSPA_Handle Instance,
			       unsigned long SampleCount);
};

/*****************************************************************************/

#define DECO_IN_W     0
#define DECO_IN_X     1
#define DECO_IN_Y     2
#define DECO_IN_Z     3
#define DECO_IN_R     4
#define DECO_IN_S     5
#define DECO_IN_T     6
#define DECO_IN_U     7
#define DECO_IN_V     8
#define DECO_OUT_FFL  9
#define DECO_OUT_FFR  10
#define DECO_OUT_FRR  11
#define DECO_OUT_BRR  12
#define DECO_OUT_BBR  13
#define DECO_OUT_BBL  14
#define DECO_OUT_BLL  15
#define DECO_OUT_FLL  16

/** This plugin decodes FMH-Format to produce a speaker feed for eight
    speakers arranged at the corners of an octagon. */
class FMHFormatToOct : public CMT_PluginInstance {
public:
  FMHFormatToOct(const LADSPA_Descriptor *,
	       unsigned long lSampleRate)
    : CMT_PluginInstance(17) {
  }
  friend void runFMHFormatToOct(LADSPA_Handle Instance,
				unsigned long SampleCount);
};

/*****************************************************************************/

#define BFROT_ANGLE    0
#define BFROT_IN_W     1
#define BFROT_IN_X     2
#define BFROT_IN_Y     3
#define BFROT_IN_Z     4
#define BFROT_OUT_W    5
#define BFROT_OUT_X    6
#define BFROT_OUT_Y    7
#define BFROT_OUT_Z    8

/** This plugin rotates an B-Format soundfield around the Z-axis. */
class BFormatRotation : public CMT_PluginInstance {
public:
  BFormatRotation(const LADSPA_Descriptor *,
		  unsigned long lSampleRate)
    : CMT_PluginInstance(9) {
  }
  friend void runBFormatRotation(LADSPA_Handle Instance,
				 unsigned long SampleCount);
};

/*****************************************************************************/

#define FMHROT_ANGLE    0

#define FMHROT_IN_W     1
#define FMHROT_IN_X     2
#define FMHROT_IN_Y     3
#define FMHROT_IN_Z     4
#define FMHROT_IN_R     5
#define FMHROT_IN_S     6
#define FMHROT_IN_T     7
#define FMHROT_IN_U     8
#define FMHROT_IN_V     9

#define FMHROT_OUT_W    10
#define FMHROT_OUT_X    11
#define FMHROT_OUT_Y    12
#define FMHROT_OUT_Z    13
#define FMHROT_OUT_R    14
#define FMHROT_OUT_S    15
#define FMHROT_OUT_T    16
#define FMHROT_OUT_U    17
#define FMHROT_OUT_V    18

/** This plugin rotates an FMH-Format soundfield around the Z-axis. */
class FMHFormatRotation : public CMT_PluginInstance {
public:
  FMHFormatRotation(const LADSPA_Descriptor *,
		    unsigned long lSampleRate)
    : CMT_PluginInstance(19) {
  }
  friend void runFMHFormatRotation(LADSPA_Handle Instance,
				   unsigned long SampleCount);
};

/*****************************************************************************/

void 
runBFormatEncoder(LADSPA_Handle Instance,
		  unsigned long SampleCount) {

  BFormatEncoder * poProcessor = (BFormatEncoder *)Instance;
  
  LADSPA_Data * pfInput = poProcessor->m_ppfPorts[ENC_INPUT];
  LADSPA_Data * pfOutW  = poProcessor->m_ppfPorts[ENC_OUT_W];
  LADSPA_Data * pfOutX  = poProcessor->m_ppfPorts[ENC_OUT_X];
  LADSPA_Data * pfOutY  = poProcessor->m_ppfPorts[ENC_OUT_Y];
  LADSPA_Data * pfOutZ  = poProcessor->m_ppfPorts[ENC_OUT_Z];

  LADSPA_Data fX = *(poProcessor->m_ppfPorts[ENC_IN_X]);
  LADSPA_Data fY = *(poProcessor->m_ppfPorts[ENC_IN_Y]);
  LADSPA_Data fZ = *(poProcessor->m_ppfPorts[ENC_IN_Z]);
  LADSPA_Data fDistanceSquared = fX * fX + fY * fY + fZ * fZ;
  const LADSPA_Data fWScalar = 0.707107;
  LADSPA_Data fXScalar, fYScalar, fZScalar;
  if (fDistanceSquared > 1e-10) {
    LADSPA_Data fOneOverDistanceSquared = 1 / fDistanceSquared;
    fXScalar = fX * fOneOverDistanceSquared;
    fYScalar = fY * fOneOverDistanceSquared;
    fZScalar = fZ * fOneOverDistanceSquared;
  }
  else {
    /* Avoid division by zero issues. */
    fXScalar = fYScalar = fZScalar = 0;
  }

  for (unsigned long lSampleIndex = 0; 
       lSampleIndex < SampleCount; 
       lSampleIndex++) {
    LADSPA_Data fInput = *(pfInput++);
    *(pfOutW++) = fWScalar * fInput;
    *(pfOutX++) = fXScalar * fInput;
    *(pfOutY++) = fYScalar * fInput;
    *(pfOutZ++) = fZScalar * fInput;
  }
}

/*****************************************************************************/

void 
runFMHFormatEncoder(LADSPA_Handle Instance,
		    unsigned long SampleCount) {

  FMHFormatEncoder * poProcessor = (FMHFormatEncoder *)Instance;
  
  LADSPA_Data * pfInput = poProcessor->m_ppfPorts[ENC_INPUT];
  LADSPA_Data * pfOutW  = poProcessor->m_ppfPorts[ENC_OUT_W];
  LADSPA_Data * pfOutX  = poProcessor->m_ppfPorts[ENC_OUT_X];
  LADSPA_Data * pfOutY  = poProcessor->m_ppfPorts[ENC_OUT_Y];
  LADSPA_Data * pfOutZ  = poProcessor->m_ppfPorts[ENC_OUT_Z];
  LADSPA_Data * pfOutR  = poProcessor->m_ppfPorts[ENC_OUT_R];
  LADSPA_Data * pfOutS  = poProcessor->m_ppfPorts[ENC_OUT_S];
  LADSPA_Data * pfOutT  = poProcessor->m_ppfPorts[ENC_OUT_T];
  LADSPA_Data * pfOutU  = poProcessor->m_ppfPorts[ENC_OUT_U];
  LADSPA_Data * pfOutV  = poProcessor->m_ppfPorts[ENC_OUT_V];

  LADSPA_Data fX = *(poProcessor->m_ppfPorts[ENC_IN_X]);
  LADSPA_Data fY = *(poProcessor->m_ppfPorts[ENC_IN_Y]);
  LADSPA_Data fZ = *(poProcessor->m_ppfPorts[ENC_IN_Z]);
  LADSPA_Data fDistanceSquared = fX * fX + fY * fY + fZ * fZ;
  const LADSPA_Data fWScalar = 0.707107;
  LADSPA_Data fXScalar, fYScalar, fZScalar;
  LADSPA_Data fRScalar, fSScalar, fTScalar;
  LADSPA_Data fUScalar, fVScalar;
  if (fDistanceSquared > 1e-10) {
    LADSPA_Data fOneOverDistanceSquared 
      = 1 / fDistanceSquared;
    LADSPA_Data fOneOverDistanceCubed 
      = LADSPA_Data(pow(fDistanceSquared, -1.5));
    fXScalar = fX * fOneOverDistanceSquared;
    fYScalar = fY * fOneOverDistanceSquared;
    fZScalar = fZ * fOneOverDistanceSquared;
    fRScalar = ((fZ * fZ) * fOneOverDistanceSquared - 0.5) 
      * sqrt(fOneOverDistanceSquared);
    fSScalar = 2 * (fZ * fX) * fOneOverDistanceCubed;
    fTScalar = 2 * (fY * fX) * fOneOverDistanceCubed;
    fUScalar = (fX * fX - fY * fY) * fOneOverDistanceCubed;
    fVScalar = 2 * (fX * fY) * fOneOverDistanceCubed;
  }
  else {
    /* Avoid division by zero issues. */
    fXScalar = fYScalar = fZScalar 
      = fRScalar = fSScalar = fTScalar
      = fUScalar = fVScalar = 0;
  }

  for (unsigned long lSampleIndex = 0; 
       lSampleIndex < SampleCount; 
       lSampleIndex++) {
    LADSPA_Data fInput = *(pfInput++);
    *(pfOutW++) = fWScalar * fInput;
    *(pfOutX++) = fXScalar * fInput;
    *(pfOutY++) = fYScalar * fInput;
    *(pfOutZ++) = fZScalar * fInput;
    *(pfOutR++) = fRScalar * fInput;
    *(pfOutS++) = fSScalar * fInput;
    *(pfOutT++) = fTScalar * fInput;
    *(pfOutU++) = fUScalar * fInput;
    *(pfOutV++) = fVScalar * fInput;
  }
}

/*****************************************************************************/

void 
runFMHToB(LADSPA_Handle Instance,
	  unsigned long SampleCount) {

  FMHToB * poProcessor = (FMHToB *)Instance;
  
  LADSPA_Data * pfInW  = poProcessor->m_ppfPorts[F2B_IN_W];
  LADSPA_Data * pfInX  = poProcessor->m_ppfPorts[F2B_IN_X];
  LADSPA_Data * pfInY  = poProcessor->m_ppfPorts[F2B_IN_Y];
  LADSPA_Data * pfInZ  = poProcessor->m_ppfPorts[F2B_IN_Z];
  LADSPA_Data * pfOutW = poProcessor->m_ppfPorts[F2B_OUT_W];
  LADSPA_Data * pfOutX = poProcessor->m_ppfPorts[F2B_OUT_X];
  LADSPA_Data * pfOutY = poProcessor->m_ppfPorts[F2B_OUT_Y];
  LADSPA_Data * pfOutZ = poProcessor->m_ppfPorts[F2B_OUT_Z];

  int iSize = sizeof(LADSPA_Data) * SampleCount;
  memcpy(pfOutW, pfInW, iSize);
  memcpy(pfOutX, pfInX, iSize);
  memcpy(pfOutY, pfInY, iSize);
  memcpy(pfOutZ, pfInZ, iSize);
}

/*****************************************************************************/

void
runBFormatToStereo(LADSPA_Handle Instance,
		   unsigned long SampleCount) {

  BFormatToStereo * poProcessor = (BFormatToStereo *)Instance;
  
  LADSPA_Data * pfInW  = poProcessor->m_ppfPorts[DECST_IN_W];
  LADSPA_Data * pfInY  = poProcessor->m_ppfPorts[DECST_IN_Y];

  LADSPA_Data * pfOutL = poProcessor->m_ppfPorts[DECST_OUT_L];
  LADSPA_Data * pfOutR = poProcessor->m_ppfPorts[DECST_OUT_R];

  for (unsigned long lSampleIndex = 0; 
       lSampleIndex < SampleCount; 
       lSampleIndex++) {
    LADSPA_Data fA = 0.707107 * *(pfInW++);
    LADSPA_Data fB = 0.5 * *(pfInY++);
    *(pfOutL++) = fA + fB;
    *(pfOutR++) = fA - fB;
  }

}

/*****************************************************************************/

void
runBFormatToQuad(LADSPA_Handle Instance,
		 unsigned long SampleCount) {

  BFormatToQuad * poProcessor = (BFormatToQuad *)Instance;
  
  LADSPA_Data * pfInW  = poProcessor->m_ppfPorts[DECQ_IN_W];
  LADSPA_Data * pfInX  = poProcessor->m_ppfPorts[DECQ_IN_X];
  LADSPA_Data * pfInY  = poProcessor->m_ppfPorts[DECQ_IN_Y];

  LADSPA_Data * pfOutFL = poProcessor->m_ppfPorts[DECQ_OUT_FL];
  LADSPA_Data * pfOutFR = poProcessor->m_ppfPorts[DECQ_OUT_FR];
  LADSPA_Data * pfOutBL = poProcessor->m_ppfPorts[DECQ_OUT_BL];
  LADSPA_Data * pfOutBR = poProcessor->m_ppfPorts[DECQ_OUT_BR];

  for (unsigned long lSampleIndex = 0; 
       lSampleIndex < SampleCount; 
       lSampleIndex++) {
    LADSPA_Data fW = 0.353553 * *(pfInW++);
    LADSPA_Data fX = 0.243361 * *(pfInX++);
    LADSPA_Data fY = 0.243361 * *(pfInY++);
    LADSPA_Data fV = 0.096383 * *(pfInY++);
    *(pfOutFL++) = fW + fX + fY + fV;
    *(pfOutFR++) = fW + fX - fY - fV;
    *(pfOutBL++) = fW - fX + fY + fV;
    *(pfOutBR++) = fW - fX - fY - fV;
  }

}

/*****************************************************************************/

void
runBFormatToCube(LADSPA_Handle Instance,
		 unsigned long SampleCount) {

  BFormatToCube * poProcessor = (BFormatToCube *)Instance;
  
  LADSPA_Data * pfInW  = poProcessor->m_ppfPorts[DECC_IN_W];
  LADSPA_Data * pfInX  = poProcessor->m_ppfPorts[DECC_IN_X];
  LADSPA_Data * pfInY  = poProcessor->m_ppfPorts[DECC_IN_Y];
  LADSPA_Data * pfInZ  = poProcessor->m_ppfPorts[DECC_IN_Z];

  LADSPA_Data * pfOutBFL = poProcessor->m_ppfPorts[DECC_OUT_BFL];
  LADSPA_Data * pfOutBFR = poProcessor->m_ppfPorts[DECC_OUT_BFR];
  LADSPA_Data * pfOutBBL = poProcessor->m_ppfPorts[DECC_OUT_BBL];
  LADSPA_Data * pfOutBBR = poProcessor->m_ppfPorts[DECC_OUT_BBR];
  LADSPA_Data * pfOutTFL = poProcessor->m_ppfPorts[DECC_OUT_BFL];
  LADSPA_Data * pfOutTFR = poProcessor->m_ppfPorts[DECC_OUT_BFR];
  LADSPA_Data * pfOutTBL = poProcessor->m_ppfPorts[DECC_OUT_BBL];
  LADSPA_Data * pfOutTBR = poProcessor->m_ppfPorts[DECC_OUT_BBR];

  for (unsigned long lSampleIndex = 0; 
       lSampleIndex < SampleCount; 
       lSampleIndex++) {
    LADSPA_Data fW = 0.176777 * *(pfInW++);
    LADSPA_Data fX = 0.113996 * *(pfInX++);
    LADSPA_Data fY = 0.113996 * *(pfInY++);
    LADSPA_Data fZ = 0.113996 * *(pfInZ++);
    LADSPA_Data fS = 0.036859 * *(pfInX++);
    LADSPA_Data fT = 0.036859 * *(pfInY++);
    LADSPA_Data fV = 0.036859 * *(pfInZ++);
    *(pfOutBFL++) = fW + fX + fY - fZ + fV - fT - fS;
    *(pfOutBFR++) = fW + fX - fY - fZ - fV + fT - fS;
    *(pfOutBBL++) = fW - fX + fY - fZ + fV + fT + fS;
    *(pfOutBBR++) = fW - fX - fY - fZ - fV - fT + fS;
    *(pfOutTFL++) = fW + fX + fY + fZ + fV + fT + fS;
    *(pfOutTFR++) = fW + fX - fY + fZ - fV - fT + fS;
    *(pfOutTBL++) = fW - fX + fY + fZ + fV - fT - fS;
    *(pfOutTBR++) = fW - fX - fY + fZ - fV + fT - fS;
  }

}

/*****************************************************************************/

void
runFMHFormatToOct(LADSPA_Handle Instance,
		  unsigned long SampleCount) {

  FMHFormatToOct * poProcessor = (FMHFormatToOct *)Instance;
  
  LADSPA_Data * pfInW  = poProcessor->m_ppfPorts[DECO_IN_W];
  LADSPA_Data * pfInX  = poProcessor->m_ppfPorts[DECO_IN_X];
  LADSPA_Data * pfInY  = poProcessor->m_ppfPorts[DECO_IN_Y];
  LADSPA_Data * pfInU  = poProcessor->m_ppfPorts[DECO_IN_U];
  LADSPA_Data * pfInV  = poProcessor->m_ppfPorts[DECO_IN_V];

  LADSPA_Data * pfOutFFL = poProcessor->m_ppfPorts[DECO_OUT_FFL];
  LADSPA_Data * pfOutFFR = poProcessor->m_ppfPorts[DECO_OUT_FFR];
  LADSPA_Data * pfOutFRR = poProcessor->m_ppfPorts[DECO_OUT_FRR];
  LADSPA_Data * pfOutBRR = poProcessor->m_ppfPorts[DECO_OUT_BRR];
  LADSPA_Data * pfOutBBR = poProcessor->m_ppfPorts[DECO_OUT_BBR];
  LADSPA_Data * pfOutBBL = poProcessor->m_ppfPorts[DECO_OUT_BBL];
  LADSPA_Data * pfOutBLL = poProcessor->m_ppfPorts[DECO_OUT_BLL];
  LADSPA_Data * pfOutFLL = poProcessor->m_ppfPorts[DECO_OUT_FLL];

  for (unsigned long lSampleIndex = 0; 
       lSampleIndex < SampleCount; 
       lSampleIndex++) {
    LADSPA_Data fW  = 0.176777 * *(pfInW++);
    LADSPA_Data fX1 = 0.065888 * *pfInX;
    LADSPA_Data fX2 = 0.159068 * *(pfInX++);
    LADSPA_Data fY1 = 0.065888 * *pfInY;
    LADSPA_Data fY2 = 0.159068 * *(pfInY++);
    LADSPA_Data fU  = 0.034175 * *(pfInU++);
    LADSPA_Data fV  = 0.034175 * *(pfInV++);
    *(pfOutFFL++) = fW + fX2 + fY1 + fU + fV;
    *(pfOutFFR++) = fW + fX2 - fY1 + fU - fV;
    *(pfOutFRR++) = fW + fX1 - fY2 - fU - fV;
    *(pfOutBRR++) = fW - fX1 + fY2 - fU + fV;
    *(pfOutBBR++) = fW - fX2 + fY1 + fU + fV;
    *(pfOutBBL++) = fW - fX2 - fY1 + fU - fV;
    *(pfOutBLL++) = fW - fX1 - fY2 - fU - fV;
    *(pfOutFLL++) = fW + fX1 + fY2 - fU + fV;
  }

}

/*****************************************************************************/

void
runBFormatRotation(LADSPA_Handle Instance,
		   unsigned long SampleCount) {

  BFormatRotation * poProcessor = (BFormatRotation *)Instance;

  /* Work in radians. */
  LADSPA_Data fAngle 
    = LADSPA_Data(M_PI / 180.0) * *(poProcessor->m_ppfPorts[FMHROT_ANGLE]);
  LADSPA_Data fSin = sin(fAngle);
  LADSPA_Data fCos = cos(fAngle);

  LADSPA_Data * pfInW  = poProcessor->m_ppfPorts[BFROT_IN_W];
  LADSPA_Data * pfInX  = poProcessor->m_ppfPorts[BFROT_IN_X];
  LADSPA_Data * pfInY  = poProcessor->m_ppfPorts[BFROT_IN_Y];
  LADSPA_Data * pfInZ  = poProcessor->m_ppfPorts[BFROT_IN_Z];

  LADSPA_Data * pfOutW  = poProcessor->m_ppfPorts[BFROT_OUT_W];
  LADSPA_Data * pfOutX  = poProcessor->m_ppfPorts[BFROT_OUT_X];
  LADSPA_Data * pfOutY  = poProcessor->m_ppfPorts[BFROT_OUT_Y];
  LADSPA_Data * pfOutZ  = poProcessor->m_ppfPorts[BFROT_OUT_Z];

  int iSize = sizeof(LADSPA_Data) * SampleCount;
  memcpy(pfOutW, pfInW, iSize);
  memcpy(pfOutZ, pfInZ, iSize);

  for (unsigned long lSampleIndex = 0; 
       lSampleIndex < SampleCount; 
       lSampleIndex++) {
    float fInX = *(pfInX++);
    float fInY = *(pfInY++);
    *(pfOutX++) = fCos * fInX - fSin  * fInY;
    *(pfOutY++) = fSin * fInX + fCos  * fInY;
  }
}

/*****************************************************************************/

void
runFMHFormatRotation(LADSPA_Handle Instance,
		     unsigned long SampleCount) {

  FMHFormatRotation * poProcessor = (FMHFormatRotation *)Instance;

  /* Work in radians. */
  LADSPA_Data fAngle 
    = LADSPA_Data(M_PI / 180.0) * *(poProcessor->m_ppfPorts[FMHROT_ANGLE]);
  LADSPA_Data fSin = sin(fAngle);
  LADSPA_Data fCos = cos(fAngle);
  LADSPA_Data fSin2 = sin(fAngle * 2);
  LADSPA_Data fCos2 = cos(fAngle * 2);

  LADSPA_Data * pfInW  = poProcessor->m_ppfPorts[FMHROT_IN_W];
  LADSPA_Data * pfInX  = poProcessor->m_ppfPorts[FMHROT_IN_X];
  LADSPA_Data * pfInY  = poProcessor->m_ppfPorts[FMHROT_IN_Y];
  LADSPA_Data * pfInZ  = poProcessor->m_ppfPorts[FMHROT_IN_Z];
  LADSPA_Data * pfInR  = poProcessor->m_ppfPorts[FMHROT_IN_R];
  LADSPA_Data * pfInS  = poProcessor->m_ppfPorts[FMHROT_IN_S];
  LADSPA_Data * pfInT  = poProcessor->m_ppfPorts[FMHROT_IN_T];
  LADSPA_Data * pfInU  = poProcessor->m_ppfPorts[FMHROT_IN_U];
  LADSPA_Data * pfInV  = poProcessor->m_ppfPorts[FMHROT_IN_V];

  LADSPA_Data * pfOutW  = poProcessor->m_ppfPorts[FMHROT_OUT_W];
  LADSPA_Data * pfOutX  = poProcessor->m_ppfPorts[FMHROT_OUT_X];
  LADSPA_Data * pfOutY  = poProcessor->m_ppfPorts[FMHROT_OUT_Y];
  LADSPA_Data * pfOutZ  = poProcessor->m_ppfPorts[FMHROT_OUT_Z];
  LADSPA_Data * pfOutR  = poProcessor->m_ppfPorts[FMHROT_OUT_R];
  LADSPA_Data * pfOutS  = poProcessor->m_ppfPorts[FMHROT_OUT_S];
  LADSPA_Data * pfOutT  = poProcessor->m_ppfPorts[FMHROT_OUT_T];
  LADSPA_Data * pfOutU  = poProcessor->m_ppfPorts[FMHROT_OUT_U];
  LADSPA_Data * pfOutV  = poProcessor->m_ppfPorts[FMHROT_OUT_V];

  int iSize = sizeof(LADSPA_Data) * SampleCount;
  memcpy(pfOutW, pfInW, iSize);
  memcpy(pfOutZ, pfInZ, iSize);
  memcpy(pfOutR, pfInR, iSize);

  for (unsigned long lSampleIndex = 0; 
       lSampleIndex < SampleCount; 
       lSampleIndex++) {

    float fInX = *(pfInX++);
    float fInY = *(pfInY++);
    float fInS = *(pfInS++);
    float fInT = *(pfInT++);
    float fInU = *(pfInU++);
    float fInV = *(pfInV++);

    *(pfOutX++) = fCos  * fInX - fSin  * fInY;
    *(pfOutY++) = fSin  * fInX + fCos  * fInY;    
    *(pfOutS++) = fCos  * fInS - fSin  * fInT;
    *(pfOutT++) = fSin  * fInS + fCos  * fInT;
    *(pfOutU++) = fCos2 * fInU - fSin2 * fInV;
    *(pfOutV++) = fSin2 * fInU + fCos2 * fInV;
  }
}

/*****************************************************************************/

void
initialise_ambisonic() {
  
  CMT_Descriptor * psDescriptor;

  psDescriptor = new CMT_Descriptor
    (1087,
     "encode_bformat",
     LADSPA_PROPERTY_HARD_RT_CAPABLE,
     "Ambisonic Encoder (B-Format)",
     CMT_MAKER("Richard W.E. Furse"),
     CMT_COPYRIGHT("2000-2002", "Richard W.E. Furse"),
     NULL,
     CMT_Instantiate<BFormatEncoder>,
     NULL,
     runBFormatEncoder,
     NULL,
     NULL,
     NULL);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Sound Source X Coordinate",
     LADSPA_HINT_DEFAULT_1);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Sound Source Y Coordinate",
     LADSPA_HINT_DEFAULT_0);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Sound Source Z Coordinate",
     LADSPA_HINT_DEFAULT_0);
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (W)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (X)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (Y)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (Z)");
  registerNewPluginDescriptor(psDescriptor);

  psDescriptor = new CMT_Descriptor
    (1088,
     "encode_fmh",
     LADSPA_PROPERTY_HARD_RT_CAPABLE,
     "Ambisonic Encoder (FMH-Format)",
     CMT_MAKER("Richard W.E. Furse"),
     CMT_COPYRIGHT("2000-2002", "Richard W.E. Furse"),
     NULL,
     CMT_Instantiate<FMHFormatEncoder>,
     NULL,
     runFMHFormatEncoder,
     NULL,
     NULL,
     NULL);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Sound Source X Coordinate",
     LADSPA_HINT_DEFAULT_1);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Sound Source Y Coordinate",
     LADSPA_HINT_DEFAULT_0);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Sound Source Z Coordinate",
     LADSPA_HINT_DEFAULT_0);
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (W)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (X)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (Y)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (Z)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (R)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (S)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (T)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (U)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (V)");
  registerNewPluginDescriptor(psDescriptor);

  psDescriptor = new CMT_Descriptor
    (1089,
     "fmh2bf",
     LADSPA_PROPERTY_HARD_RT_CAPABLE,
     "FMH-Format to B-Format (Discards RSTUV Channels)",
     CMT_MAKER("Richard W.E. Furse"),
     CMT_COPYRIGHT("2000", "Richard W.E. Furse"),
     NULL,
     CMT_Instantiate<FMHToB>,
     NULL,
     runFMHToB,
     NULL,
     NULL,
     NULL);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (W)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (X)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (Y)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (Z)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (R)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (S)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (T)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (U)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (V)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (W)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (X)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (Y)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (Z)");
  registerNewPluginDescriptor(psDescriptor);

  psDescriptor = new CMT_Descriptor
    (1090,
     "bf2stereo",
     LADSPA_PROPERTY_HARD_RT_CAPABLE,
     "Ambisonic Decoder (B-Format to Stereo)",
     CMT_MAKER("Richard W.E. Furse"),
     CMT_COPYRIGHT("2000", "Richard W.E. Furse"),
     NULL,
     CMT_Instantiate<BFormatToStereo>,
     NULL,
     runBFormatToStereo,
     NULL,
     NULL,
     NULL);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (W)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (X)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (Y)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (Z)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (Left)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (Right)");
  registerNewPluginDescriptor(psDescriptor);

  psDescriptor = new CMT_Descriptor
    (1091,
     "bf2quad",
     LADSPA_PROPERTY_HARD_RT_CAPABLE,
     "Ambisonic Decoder (B-Format to Quad)",
     CMT_MAKER("Richard W.E. Furse"),
     CMT_COPYRIGHT("2000", "Richard W.E. Furse"),
     NULL,
     CMT_Instantiate<BFormatToQuad>,
     NULL,
     runBFormatToQuad,
     NULL,
     NULL,
     NULL);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (W)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (X)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (Y)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (Z)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (Front Left)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (Front Right)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (Back Left)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (Back Right)");
  registerNewPluginDescriptor(psDescriptor);

  psDescriptor = new CMT_Descriptor
    (1092,
     "bf2cube",
     LADSPA_PROPERTY_HARD_RT_CAPABLE,
     "Ambisonic Decoder (B-Format to Cube)",
     CMT_MAKER("Richard W.E. Furse"),
     CMT_COPYRIGHT("2000", "Richard W.E. Furse"),
     NULL,
     CMT_Instantiate<BFormatToCube>,
     NULL,
     runBFormatToCube,
     NULL,
     NULL,
     NULL);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (W)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (X)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (Y)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (Z)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (Base Front Left)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (Base Front Right)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (Base Back Left)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (Base Back Right)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (Top Front Left)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (Top Front Right)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (Top Back Left)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (Top Back Right)");
  registerNewPluginDescriptor(psDescriptor);

  psDescriptor = new CMT_Descriptor
    (1093,
     "fmh2oct",
     LADSPA_PROPERTY_HARD_RT_CAPABLE,
     "Ambisonic Decoder (FMH-Format to Octagon)",
     CMT_MAKER("Richard W.E. Furse"),
     CMT_COPYRIGHT("2000", "Richard W.E. Furse"),
     NULL,
     CMT_Instantiate<FMHFormatToOct>,
     NULL,
     runFMHFormatToOct,
     NULL,
     NULL,
     NULL);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (W)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (X)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (Y)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (Z)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (R)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (S)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (T)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (U)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (V)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (Front Front Left)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (Front Front Right)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (Front Right Right)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (Back Right Right)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (Back Back Right)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (Back Back Left)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (Back Left Left)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (Front Left Left)");
  registerNewPluginDescriptor(psDescriptor);

  psDescriptor = new CMT_Descriptor
    (1094,
     "bf_rotate_z",
     LADSPA_PROPERTY_HARD_RT_CAPABLE,
     "Ambisonic Rotation (B-Format, Horizontal)",
     CMT_MAKER("Richard W.E. Furse"),
     CMT_COPYRIGHT("2000-2002", "Richard W.E. Furse"),
     NULL,
     CMT_Instantiate<BFormatRotation>,
     NULL,
     runBFormatRotation,
     NULL,
     NULL,
     NULL);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Angle of Rotation (Degrees Anticlockwise)",
     (LADSPA_HINT_BOUNDED_BELOW
      | LADSPA_HINT_BOUNDED_ABOVE
      | LADSPA_HINT_DEFAULT_HIGH),
     -180,
     180);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (W)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (X)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (Y)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (Z)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (W)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (X)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (Y)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (Z)");
  registerNewPluginDescriptor(psDescriptor);

  psDescriptor = new CMT_Descriptor
    (1095,
     "fmh_rotate_z",
     LADSPA_PROPERTY_HARD_RT_CAPABLE,
     "Ambisonic Rotation (FMH-Format, Horizontal)",
     CMT_MAKER("Richard W.E. Furse"),
     CMT_COPYRIGHT("2000-2002", "Richard W.E. Furse"),
     NULL,
     CMT_Instantiate<FMHFormatRotation>,
     NULL,
     runFMHFormatRotation,
     NULL,
     NULL,
     NULL);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Angle of Rotation (Degrees Anticlockwise)",
     (LADSPA_HINT_BOUNDED_BELOW 
      | LADSPA_HINT_BOUNDED_ABOVE
      | LADSPA_HINT_DEFAULT_HIGH),
     -180,
     180);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (W)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (X)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (Y)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (Z)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (R)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (S)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (T)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (U)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (V)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (W)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (X)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (Y)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (Z)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (R)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (S)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (T)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (U)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (V)");
  registerNewPluginDescriptor(psDescriptor);

}

/*****************************************************************************/

/* EOF */
