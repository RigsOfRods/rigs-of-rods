
namespace AngelOgre { // Dummy namespace, just to distinguish AngelScript from C++

/** \addtogroup ScriptSideAPIs
 *  @{
 */

/** \addtogroup Script2Game
 *  @{
 */

    /** A part of a mesh which uses a single material. Binding of `Ogre::SubMesh`.
        @remarks
            Obtain one with `MeshPtr::getSubMeshes()`, `MeshPtr::createSubMesh()` or `SubEntity::getSubMesh()`.
        @par
            Methods prefixed with `__` are experimental helpers for reading the geometry;
            they may change in future versions. On failure they log to Angelscript.log and return null.
    */
    class SubMesh
    {
    public:

        /// Name of the material used by this submesh.
        const string& getMaterialName();

        /** Sets the material used by this submesh.
        @param name Material name.
        @param rg Resource group of the material.
        */
        void setMaterialName(const string&in name, const string&in rg);

        /// Returns positions of all vertices.
        array<vector3>@ __getVertexPositions();

        /** Returns texture coordinates of all vertices.
        @param index Which texture coordinate set to read (0 = first).
        */
        array<vector2>@ __getVertexTexcoords(uint index);

        /// Returns the format of the index buffer - use it to pick `__getIndexBuffer16bit()` or `__getIndexBuffer32bit()`.
        IndexType __getIndexType();

        /// Returns the index buffer; only works if `__getIndexType()` is `IT_16BIT`.
        array<uint16>@ __getIndexBuffer16bit();

        /// Returns the index buffer; only works if `__getIndexType()` is `IT_32BIT`.
        array<uint>@ __getIndexBuffer32bit();
    };

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} // namespace AngelOgre (dummy, just to distinguish AngelScript from C++)

