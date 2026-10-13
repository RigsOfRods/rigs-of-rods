
namespace AngelOgre { // Dummy namespace, just to distinguish AngelScript from C++

/** \addtogroup ScriptSideAPIs
 *  @{
 */

/** \addtogroup Script2Game
 *  @{
 */

    /** Geometry built by the script, vertex by vertex. Binding of `Ogre::ManualObject`.
        @remarks
            Create one with `SceneManager::createManualObject()` and attach it to a `SceneNode`.
            Each section is defined by `begin()`, followed by vertices (`position()` first, then optionally
            `normal()`, `textureCoord()`, `colour()`), optionally `index()`es, and finally `end()`.
        @par
            It is a `MovableObject` - the handle converts implicitly, and `MovableObject` handles can be cast back.
    */
    class ManualObject
    {
    public:
        /** Starts defining a new section.
        @param materialName Material to use for this section.
        @param opType Type of primitives, see `RenderOperation`.
        @param rg Resource group of the material.
        */
        void begin(const string&in materialName, RenderOperation opType, const string&in rg);

        /// Starts re-defining an existing section (the next one), for dynamic geometry.
        void beginUpdate();

        /// Adds a vertex at the given position; call the other vertex attribute methods after this.
        void position(const vector3&in pos);

        /// Sets the normal of the current vertex.
        void normal(const vector3&in norm);

        /// Sets a 2D texture coordinate of the current vertex.
        void textureCoord(float u, float v);

        /// Sets a 2D texture coordinate of the current vertex.
        void textureCoord(const vector2&in uv);

        /// Sets a 3D texture coordinate of the current vertex.
        void textureCoord(const vector3&in uvw);

        /// Sets the colour of the current vertex.
        void colour(const color&in col);

        /// Adds a vertex index; if you don't add any, vertices are used in order.
        void index(uint32 idx);

        /// Finishes the current section.
        void end();

        /// Number of vertices in the section being defined.
        uint getCurrentVertexCount();

        /** Number of indices in the section being defined.
        @warning Currently broken - the binding is declared to return `void`.
        */
        void getCurrentIndexCount();

        /** Converts the geometry to a mesh, which can then be used to create entities.
        @param name Name of the new mesh.
        @param group Resource group of the new mesh.
        */
        MeshPtr convertToMesh(const string&in name, const string&in group = 'General');
    };

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} // namespace AngelOgre (dummy, just to distinguish AngelScript from C++)

