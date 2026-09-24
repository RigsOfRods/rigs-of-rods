
namespace AngelOgre { // Dummy namespace, just to distinguish AngelScript from C++

/** \addtogroup ScriptSideAPIs
 *  @{
 */

/** \addtogroup Script2Game
 *  @{
 */

    /** A single rendering pass of a `Technique`. Binding of `Ogre::Pass`.
        @remarks
            Obtain one with `Technique::getPasses()` or `Technique::createPass()`.
        @par
            The `get*ProgramParameters()` methods return a null pointer if the pass has no shader program of that type;
            the error is forwarded as `SE_GENERIC_EXCEPTION_CAUGHT` script event.
    */
    class Pass
    {
    public:
        /// Name of the pass (may be empty).
        const string& getName() const;

        /// Returns a read-only list of the pass's texture units.
        TextureUnitStateArray@ getTextureUnitStates();

        /// Removes the texture unit at the given index.
        void removeTextureUnitState(uint16 index);

        /// @name Shader program parameters
        /// @{
        GpuProgramParametersPtr getVertexProgramParameters();
        GpuProgramParametersPtr getFragmentProgramParameters();
        GpuProgramParametersPtr getGeometryProgramParameters();
        GpuProgramParametersPtr getTessellationHullProgramParameters();
        GpuProgramParametersPtr getTessellationDomainProgramParameters();
        GpuProgramParametersPtr getComputeProgramParameters();

        void setVertexProgramParameters(GpuProgramParametersPtr params);
        void setFragmentProgramParameters(GpuProgramParametersPtr params);
        void setGeometryProgramParameters(GpuProgramParametersPtr params);
        void setTessellationHullProgramParameters(GpuProgramParametersPtr params);
        void setTessellationDomainProgramParameters(GpuProgramParametersPtr params);
        void setComputeProgramParameters(GpuProgramParametersPtr params);
        /// @}
    };

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} // namespace AngelOgre (dummy, just to distinguish AngelScript from C++)

