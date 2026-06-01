/*
 * TracyProfiling.h - Tracy profiling for LMMS
 *
 * Copyright (c) 2026 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#ifndef LMMS_TRACY_PROFILING_H
#define LMMS_TRACY_PROFILING_H

#include "lmmsconfig.h"

#ifdef LMMS_DEBUG_TRACY

#ifndef TRACY_ENABLE
#define TRACY_ENABLE
#endif
#include <tracy/Tracy.hpp>
#include <tracy/TracyC.h>

#else

// Define no-op Tracy macros (copied from tracy/Tracy.hpp)

#define TracyNoop

#define ZoneNamed(x,y)
#define ZoneNamedN(x,y,z)
#define ZoneNamedC(x,y,z)
#define ZoneNamedNC(x,y,z,w)

#define ZoneTransient(x,y)
#define ZoneTransientN(x,y,z)

#define ZoneScoped
#define ZoneScopedN(x)
#define ZoneScopedC(x)
#define ZoneScopedNC(x,y)

#define ZoneText(x,y)
#define ZoneTextV(x,y,z)
#define ZoneTextF(x,...)
#define ZoneTextVF(x,y,...)
#define ZoneName(x,y)
#define ZoneNameV(x,y,z)
#define ZoneNameF(x,...)
#define ZoneNameVF(x,y,...)
#define ZoneColor(x)
#define ZoneColorV(x,y)
#define ZoneValue(x)
#define ZoneValueV(x,y)
#define ZoneIsActive false
#define ZoneIsActiveV(x) false

#define FrameMark
#define FrameMarkNamed(x)
#define FrameMarkStart(x)
#define FrameMarkEnd(x)

#define FrameImage(x,y,z,w,a)

#define TracyLockable( type, varname ) type varname
#define TracyLockableN( type, varname, desc ) type varname
#define TracySharedLockable( type, varname ) type varname
#define TracySharedLockableN( type, varname, desc ) type varname
#define LockableBase( type ) type
#define SharedLockableBase( type ) type
#define LockMark(x) (void)x
#define LockableName(x,y,z)

#define TracyPlot(x,y)
#define TracyPlotConfig(x,y,z,w,a)

#define TracyMessage(x,y)
#define TracyMessageL(x)
#define TracyMessageC(x,y,z)
#define TracyMessageLC(x,y)
#define TracyAppInfo(x,y)

#define TracyAlloc(x,y)
#define TracyFree(x)
#define TracyMemoryDiscard(x)
#define TracySecureAlloc(x,y)
#define TracySecureFree(x)
#define TracySecureMemoryDiscard(x)

#define TracyAllocN(x,y,z)
#define TracyFreeN(x,y)
#define TracySecureAllocN(x,y,z)
#define TracySecureFreeN(x,y)

#define ZoneNamedS(x,y,z)
#define ZoneNamedNS(x,y,z,w)
#define ZoneNamedCS(x,y,z,w)
#define ZoneNamedNCS(x,y,z,w,a)

#define ZoneTransientS(x,y,z)
#define ZoneTransientNS(x,y,z,w)

#define ZoneScopedS(x)
#define ZoneScopedNS(x,y)
#define ZoneScopedCS(x,y)
#define ZoneScopedNCS(x,y,z)

#define TracyAllocS(x,y,z)
#define TracyFreeS(x,y)
#define TracyMemoryDiscardS(x,y)
#define TracySecureAllocS(x,y,z)
#define TracySecureFreeS(x,y)
#define TracySecureMemoryDiscardS(x,y)

#define TracyAllocNS(x,y,z,w)
#define TracyFreeNS(x,y,z)
#define TracySecureAllocNS(x,y,z,w)
#define TracySecureFreeNS(x,y,z)

#define TracyMessageS(x,y,z)
#define TracyMessageLS(x,y)
#define TracyMessageCS(x,y,z,w)
#define TracyMessageLCS(x,y,z)

#define TracySourceCallbackRegister(x,y)
#define TracyParameterRegister(x,y)
#define TracyParameterSetup(x,y,z,w)
#define TracyIsConnected false
#define TracyIsStarted false
#define TracySetProgramName(x)

#define TracyFiberEnter(x)
#define TracyFiberEnterHint(x,y)
#define TracyFiberLeave

// Define no-op Tracy C API macros (copied from tracy/TracyC.h)

typedef const void* TracyCZoneCtx;

typedef const void* TracyCLockCtx;

#define TracyCZone(c,x)
#define TracyCZoneN(c,x,y)
#define TracyCZoneC(c,x,y)
#define TracyCZoneNC(c,x,y,z)
#define TracyCZoneEnd(c)
#define TracyCZoneText(c,x,y)
#define TracyCZoneName(c,x,y)
#define TracyCZoneColor(c,x)
#define TracyCZoneValue(c,x)

#define TracyCAlloc(x,y)
#define TracyCFree(x)
#define TracyCMemoryDiscard(x)
#define TracyCSecureAlloc(x,y)
#define TracyCSecureFree(x)
#define TracyCSecureMemoryDiscard(x)

#define TracyCAllocN(x,y,z)
#define TracyCFreeN(x,y)
#define TracyCSecureAllocN(x,y,z)
#define TracyCSecureFreeN(x,y)

#define TracyCFrameMark
#define TracyCFrameMarkNamed(x)
#define TracyCFrameMarkStart(x)
#define TracyCFrameMarkEnd(x)
#define TracyCFrameImage(x,y,z,w,a)

#define TracyCPlot(x,y)
#define TracyCPlotF(x,y)
#define TracyCPlotI(x,y)
#define TracyCPlotConfig(x,y,z,w,a)

#define TracyCMessage(x,y)
#define TracyCMessageL(x)
#define TracyCMessageC(x,y,z)
#define TracyCMessageLC(x,y)
#define TracyCAppInfo(x,y)

#define TracyCZoneS(x,y,z)
#define TracyCZoneNS(x,y,z,w)
#define TracyCZoneCS(x,y,z,w)
#define TracyCZoneNCS(x,y,z,w,a)

#define TracyCAllocS(x,y,z)
#define TracyCFreeS(x,y)
#define TracyCMemoryDiscardS(x,y)
#define TracyCSecureAllocS(x,y,z)
#define TracyCSecureFreeS(x,y)
#define TracyCSecureMemoryDiscardS(x,y)

#define TracyCAllocNS(x,y,z,w)
#define TracyCFreeNS(x,y,z)
#define TracyCSecureAllocNS(x,y,z,w)
#define TracyCSecureFreeNS(x,y,z)

#define TracyCMessageS(x,y,z)
#define TracyCMessageLS(x,y)
#define TracyCMessageCS(x,y,z,w)
#define TracyCMessageLCS(x,y,z)

#define TracyCLockCtx(l)
#define TracyCLockAnnounce(l)
#define TracyCLockTerminate(l)
#define TracyCLockBeforeLock(l)
#define TracyCLockAfterLock(l)
#define TracyCLockAfterUnlock(l)
#define TracyCLockAfterTryLock(l,x)
#define TracyCLockMark(l)
#define TracyCLockCustomName(l,x,y)

#define TracyCIsConnected 0
#define TracyCIsStarted 0

#define TracyCBeginSamplingProfiling() 0
#define TracyCEndSamplingProfiling()

#ifdef TRACY_FIBERS
#  define TracyCFiberEnter(fiber)
#  define TracyCFiberLeave
#endif

#endif

#endif // LMMS_TRACY_PROFILING_H
