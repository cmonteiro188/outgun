/*
 *  timer.cpp
 *
 *  Copyright (C) 2006, 2008, 2010, 2011 - Niko Ritari
 *
 *  This file is part of Outgun.
 *
 *  Outgun is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Outgun is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Outgun; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "incpthread.h"
#include "timer.h"

SystemTimer* g_systemTimer = 0;
TimeCounter g_timeCounter;

void TimeCounter::refresh(double maxExpectedSkip) throw () {
    const double oldValue = value;
    value = g_systemTimer->read() - base;
    if (value < oldValue || value > oldValue + maxExpectedSkip) { // if the clock suddenly goes backwards, or skips forward by an unexpected amount, assume the change is only in the clock, not in real time
        // value = read - base; oldValue = read - newBase -> newBase = read - oldValue = base + value - oldValue
        base += value - oldValue;
        value = oldValue;
    }
}

static bool quickSleepDelay = true;

void quickSleep() throw () {
    if (quickSleepDelay)
        platSleep(2);
    else
        sched_yield();
}

void removeQuickSleepDelay() throw () {
    quickSleepDelay = false;
}
