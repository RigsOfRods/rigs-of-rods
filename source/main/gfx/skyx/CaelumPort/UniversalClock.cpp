// This file is part of the Caelum project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution.

#include "CaelumPort/UniversalClock.h"
#include "CaelumPort/Astronomy.h"

namespace CaelumPort
{
    const Caelum::LongReal UniversalClock::SECONDS_PER_DAY = 86400.0;

    UniversalClock::UniversalClock () {
        setJulianDay (Astronomy::J2000);        
        setTimeScale (1.0);
    }

    void UniversalClock::setJulianDay (Caelum::LongReal value) {
        mJulianDayBase = value;
        mCurrentTime = 0;
        mLastUpdateTime = 0;
    }

    void UniversalClock::setGregorianDateTime(
            int year, int month, int day,
            int hour, int minute, double second)
    {
        ScopedHighPrecissionFloatSwitch precissionSwitch;
        setJulianDay(Astronomy::getJulianDayFromGregorianDateTime(year, month, day, hour, minute, second));
    }

    LongReal UniversalClock::getJulianDay () const
    {
        ScopedHighPrecissionFloatSwitch precissionSwitch;
        Caelum::LongReal res = mJulianDayBase + mCurrentTime / SECONDS_PER_DAY;
        return res;
    }

    LongReal UniversalClock::getJulianDayDifference () const {
        ScopedHighPrecissionFloatSwitch precissionSwitch;
        return (mCurrentTime - mLastUpdateTime) / SECONDS_PER_DAY;
    }

    LongReal UniversalClock::getJulianSecond () const {
        ScopedHighPrecissionFloatSwitch precissionSwitch;
        LongReal res = mJulianDayBase * SECONDS_PER_DAY + mCurrentTime;
        return res;
    }

    LongReal UniversalClock::getJulianSecondDifference () const {
        ScopedHighPrecissionFloatSwitch precissionSwitch;
        return mCurrentTime - mLastUpdateTime;
    }

    void UniversalClock::setTimeScale (const Ogre::Real scale) {
        mTimeScale = scale;
    }

    Ogre::Real UniversalClock::getTimeScale () const {
        return mTimeScale;
    }

    void UniversalClock::update (const Ogre::Real time) {
        mLastUpdateTime = mCurrentTime;
        mCurrentTime += time * mTimeScale;
    }
}

