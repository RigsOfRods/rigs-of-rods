// This file is part of the Caelum project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution.

#ifndef CAELUMPORT__CAELUM_EXCEPTIONS_H
#define CAELUMPORT__CAELUM_EXCEPTIONS_H

#include "CaelumPort/CaelumPrerequisites.h"

namespace CaelumPort
{
    /** Exception class for unsupported features.
     *  This is frequently thrown if a certain required material does not load;
     *  most likely because the hardware does not support the required shaders.
     */
    class  UnsupportedException : public Ogre::Exception
    {
    public:
        /// Constructor.
        UnsupportedException
        (
                int number,
                const Ogre::String &description,
                const Ogre::String &source,
                const char *file,
                long line
        ):
                Ogre::Exception (number, description, source, "UnsupportedException", file, line)
        {
        }
    };

#define CAELUMPORT_THROW_UNSUPPORTED_EXCEPTION(desc, src) \
        throw UnsupportedException(-1, (desc), (src), __FILE__, __LINE__);

}

#endif // CAELUM__CAELUM_EXCEPTIONS_H
