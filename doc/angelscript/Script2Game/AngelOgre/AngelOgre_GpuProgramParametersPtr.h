
namespace AngelOgre { // Dummy namespace, just to distinguish AngelScript from C++

/** \addtogroup ScriptSideAPIs
 *  @{
 */

/** \addtogroup Script2Game
 *  @{
 */

    /** A shared pointer to a set of shader parameters (uniforms). Binding of `Ogre::GpuProgramParametersSharedPtr`.
        @remarks
            Obtain one with `Pass::getVertexProgramParameters()`, `Pass::getFragmentProgramParameters()` etc.
        @par
            Errors (e.g. unknown constant name) are forwarded as `SE_GENERIC_EXCEPTION_CAUGHT` script events.
    */
    class GpuProgramParametersPtr
    {
    public:

        /// Creates an empty (null) pointer.
        GpuProgramParametersPtr();

        /// Creates a copy pointing to the same parameters.
        GpuProgramParametersPtr(const GpuProgramParametersPtr&in other);

        GpuProgramParametersPtr& opAssign(const GpuProgramParametersPtr&in other);

        /// Returns true if the pointer doesn't point to anything.
        bool isNull();

        /// @name Set constant by index
        /// The index is the constant register (a 4-float slot); arrays are written to consecutive memory starting there.
        /// @{
        void setConstant(uint index, float val);
        void setConstant(uint index, const vector3& val);
        void setConstant(uint index, const vector2& val);
        void setConstant(uint index, const color& val);
        void setConstant(uint index, const array<float>@ vals);
        void setConstant(uint index, const array<vector3>@ vals);
        void setConstant(uint index, const array<vector2>@ vals);
        void setConstant(uint index, const array<color>@ vals);
        /// @}

        /// @name Set constant by name
        /// The name is the uniform name as declared in the shader.
        /// @{
        void setNamedConstant(const string&in name, float val);
        void setNamedConstant(const string&in name, const vector3& val);
        void setNamedConstant(const string&in name, const vector2& val);
        void setNamedConstant(const string&in name, const color& val);
        void setNamedConstant(const string&in name, const array<float>@ vals);
        void setNamedConstant(const string&in name, const array<vector3>@ vals);
        void setNamedConstant(const string&in name, const array<vector2>@ vals);
        void setNamedConstant(const string&in name, const array<color>@ vals);
        /// @}

        /// Experimental: returns names of all named constants (uniforms) of the program.
        array<string>@ __getNamedConstants();
    };

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} // namespace AngelOgre (dummy, just to distinguish AngelScript from C++)

