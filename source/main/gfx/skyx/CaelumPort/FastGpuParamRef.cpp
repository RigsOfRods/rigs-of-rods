// This file is part of the Caelum project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution.


#include "CaelumPort/FastGpuParamRef.h"

using namespace Ogre;

namespace CaelumPort
{
    FastGpuParamRef::FastGpuParamRef(Ogre::GpuProgramParametersSharedPtr paramsPtr, const Ogre::String& name)
    {
        this->bind(paramsPtr, name);
    }

    void FastGpuParamRef::bind(
            Ogre::GpuProgramParametersSharedPtr params,
            const Ogre::String& name,
            bool throwIfNotFound/* = false*/)
    {
        assert(!params.isNull());
        #if CAELUM_DEBUG_PARAM_REF
            mParams = params;
        #endif
        const GpuConstantDefinition* def = params->_findNamedConstantDefinition(name, throwIfNotFound);
        if (def) {
            mPhysicalIndex = def->physicalIndex;
            assert(this->isBound());
        } else {
            mPhysicalIndex = InvalidPhysicalIndex;
            assert(!this->isBound());
        }
    }

    void FastGpuParamRef::unbind() {
        #if CAELUM_DEBUG_PARAM_REF
            mParams.setNull();
        #endif
        mPhysicalIndex = InvalidPhysicalIndex;
        assert(!this->isBound());
    }
}
