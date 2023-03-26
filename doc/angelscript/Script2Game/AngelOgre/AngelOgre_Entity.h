
namespace AngelOgre { // Dummy namespace, just to distinguish AngelScript from C++

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */  

    /** Defines an instance of a discrete, movable object based on a Mesh.
    @par
        Entity and SubEntity classes are never created directly. Use the
        createEntity method of the SceneManager (passing a model name) to
        create one.
    @par
        Entities are included in the scene by associating them with a
        SceneNode, using the attachEntity method. See the SceneNode class
        for full information.
    @note
        No functions were declared virtual to improve performance.
    */
    class  Entity: public MovableObject
    {

    public:
        /** Default destructor.
        */
        ~Entity();

        /** Gets the Mesh that this Entity is based on.
        */
   //     const MeshPtr& getMesh(void) const;

        /** Sets the material to use for the whole of this entity.
        @remarks
            This is a shortcut method to set all the materials for all
            subentities of this entity. Only use this method is you want to
            set the same material for all subentities or if you know there
            is only one. Otherwise call getSubEntity() and call the same
            method on the individual SubEntity.
        */
        void setMaterialName( const string& name, const string& groupName = "OgreAutodetect" );

        /** @copydoc MovableObject::getBoundingBox
        */
   //     const AxisAlignedBox& getBoundingBox(void) const;

        /// Merge all the child object Bounds a return it.
   //     AxisAlignedBox getChildObjectsBoundingBox(void) const;

        /** returns "Entity" */
        const string& getMovableType(void) const;

        /** For entities based on animated meshes, gets the AnimationState object for a single animation.
        @remarks
            You animate an entity by updating the animation state objects. Each of these represents the
            current state of each animation available to the entity. The AnimationState objects are
            initialised from the Mesh object.
        */
        AnimationState@ getAnimationState(const string& name);
        
        /** For entities based on animated meshes, gets the AnimationState objects for all animations.
        @return
            In case the entity is animated, this functions returns the pointer to a AnimationStateSet
            containing all animations of the entries. If the entity is not animated, it returns 0.
        @remarks
            You animate an entity by updating the animation state objects. Each of these represents the
            current state of each animation available to the entity. The AnimationState objects are
            initialised from the Mesh object.
        */
        AnimationStateSet@ getAllAnimationStates(void);

        /** Tells the Entity whether or not it should display it's skeleton, if it has one.
        */
        void setDisplaySkeleton(bool display);

        /** Returns whether or not the entity is currently displaying its skeleton.
        */
        bool getDisplaySkeleton(void) const;

        /** Returns the number of manual levels of detail that this entity supports.
        @remarks
            This number never includes the original entity, it is difference
            with Mesh::getNumLodLevels.
        */
        uint64 getNumManualLodLevels(void) const;

        /** Returns the current LOD used to render
        */
        uint16 getCurrentLodIndex();

        /** Gets a pointer to the entity representing the numbered manual level of detail.
        @remarks
            The zero-based index never includes the original entity, unlike
            Mesh::getLodLevel.
        */
        Entity* getManualLodLevel(uint64 index) const;

        /** Sets a level-of-detail bias for the mesh detail of this entity.
        @remarks
            Level of detail reduction is normally applied automatically based on the Mesh
            settings. However, it is possible to influence this behaviour for this entity
            by adjusting the LOD bias. This 'nudges' the mesh level of detail used for this
            entity up or down depending on your requirements. You might want to use this
            if there was a particularly important entity in your scene which you wanted to
            detail better than the others, such as a player model.
        @par
            There are three parameters to this method; the first is a factor to apply; it
            defaults to 1.0 (no change), by increasing this to say 2.0, this model would
            take twice as long to reduce in detail, whilst at 0.5 this entity would use lower
            detail versions twice as quickly. The other 2 parameters are hard limits which
            let you set the maximum and minimum level-of-detail version to use, after all
            other calculations have been made. This lets you say that this entity should
            never be simplified, or that it can only use LODs below a certain level even
            when right next to the camera.
        @param factor
            Proportional factor to apply to the distance at which LOD is changed.
            Higher values increase the distance at which higher LODs are displayed (2.0 is
            twice the normal distance, 0.5 is half).
        @param maxDetailIndex
            The index of the maximum LOD this entity is allowed to use (lower
            indexes are higher detail: index 0 is the original full detail model).
        @param minDetailIndex
            The index of the minimum LOD this entity is allowed to use (higher
            indexes are lower detail). Use something like 99 if you want unlimited LODs (the actual
            LOD will be limited by the number in the Mesh).
        */
        void setMeshLodBias(float factor, uint16 maxDetailIndex = 0, uint16 minDetailIndex = 99);

        /** Sets a level-of-detail bias for the material detail of this entity.
        @remarks
            Level of detail reduction is normally applied automatically based on the Material
            settings. However, it is possible to influence this behaviour for this entity
            by adjusting the LOD bias. This 'nudges' the material level of detail used for this
            entity up or down depending on your requirements. You might want to use this
            if there was a particularly important entity in your scene which you wanted to
            detail better than the others, such as a player model.
        @par
            There are three parameters to this method; the first is a factor to apply; it
            defaults to 1.0 (no change), by increasing this to say 2.0, this entity would
            take twice as long to use a lower detail material, whilst at 0.5 this entity
            would use lower detail versions twice as quickly. The other 2 parameters are
            hard limits which let you set the maximum and minimum level-of-detail index
            to use, after all other calculations have been made. This lets you say that
            this entity should never be simplified, or that it can only use LODs below
            a certain level even when right next to the camera.
        @param factor
            Proportional factor to apply to the distance at which LOD is changed.
            Higher values increase the distance at which higher LODs are displayed (2.0 is
            twice the normal distance, 0.5 is half).
        @param maxDetailIndex
            The index of the maximum LOD this entity is allowed to use (lower
            indexes are higher detail: index 0 is the original full detail model).
        @param minDetailIndex
            The index of the minimum LOD this entity is allowed to use (higher
            indexes are lower detail. Use something like 99 if you want unlimited LODs (the actual
            LOD will be limited by the number of LOD indexes used in the Material).
        */
        void setMaterialLodBias(float factor, uint16 maxDetailIndex = 0, uint16 minDetailIndex = 99);
        
        /** Returns whether or not this entity is skeletally animated. */
        bool hasSkeleton(void) const;
    };
    
/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} // namespace AngelOgre (dummy, just to distinguish AngelScript from C++)
