
namespace AngelOgre { // Dummy namespace, just to distinguish AngelScript from C++

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */  

    /** Abstract class defining a movable object in a scene.
    */
    class  MovableObject
    {
    public:

        /** Returns the name of this object. */
        const string& getName(void) const;

        /** Returns the type name of this object. */
        const string& getMovableType(void) const;

        /** Returns the node to which this object is attached.
        @remarks
            A MovableObject may be attached to either a SceneNode or to a TagPoint, 
            the latter case if it's attached to a bone on an animated entity. 
            Both are Node subclasses so this method will return either.
        */
        Node* getParentNode(void) const;

        /** Returns the scene node to which this object is attached.
        @remarks
            A MovableObject may be attached to either a SceneNode or to a TagPoint, 
            the latter case if it's attached to a bone on an animated entity. 
            This method will return the scene node of the parent entity 
            if the latter is true.
        */
        SceneNode@ getParentSceneNode(void) const;

        /// Gets whether the parent node is a TagPoint (or a SceneNode)
        bool isParentTagPoint() const;

        /** Returns true if this object is attached to a SceneNode or TagPoint. */
        bool isAttached(void) const;

        /** Detaches an object from a parent SceneNode or TagPoint, if attached. */
        void detachFromParent(void);

        /** Returns true if this object is attached to a SceneNode or TagPoint, 
            and this SceneNode / TagPoint is currently in an active part of the
            scene graph. */
        bool isInScene(void) const;

        /** Retrieves the local axis-aligned bounding box for this object.
            @remarks
                This bounding box is in local coordinates.
        */
  //      const AxisAlignedBox& getBoundingBox(void) const = 0;

        /** Retrieves the radius of the origin-centered bounding sphere 
             for this object.
        */
        float getBoundingRadius(void) const;

        /** Tells this object whether to be visible or not, if it has a renderable component. 
        */
        void setVisible(bool visible);

        /** Gets this object whether to be visible or not, if it has a renderable component. 
        @remarks
            Returns the value set by MovableObject::setVisible only.
        */
        bool getVisible(void) const;

        /** Returns whether or not this object is supposed to be visible or not. 
        @remarks
            Takes into account both upper rendering distance and visible flag.
        */
        bool isVisible(void) const;

        /** Sets the distance at which the object is no longer rendered.
        @note Camera::setUseRenderingDistance() needs to be called for this parameter to be used.
        @param dist Distance beyond which the object will not be rendered 
            (the default is 0, which means objects are always rendered).
        */
        void setRenderingDistance(float dist);

        /** Gets the distance at which batches are no longer rendered. */
        float getRenderingDistance(void) const { return mUpperDistance; }

        /** Sets the minimum pixel size an object needs to be in both screen axes in order to be rendered
        @note Camera::setUseMinPixelSize() needs to be called for this parameter to be used.
        @param pixelSize Number of minimum pixels
            (the default is 0, which means objects are always rendered).
        */
        void setRenderingMinPixelSize(float pixelSize);

        /** Returns the minimum pixel size an object needs to be in both screen axes in order to be rendered
        */
        float getRenderingMinPixelSize() const;

        /** Return an instance of user objects binding associated with this class.
        You can use it to associate one or more custom objects with this class instance.
        @see UserObjectBindings::setUserAny.        
        */
  //      UserObjectBindings& getUserObjectBindings() { return mUserObjectBindings; }

        /** Return an instance of user objects binding associated with this class.
        You can use it to associate one or more custom objects with this class instance.
        @see UserObjectBindings::setUserAny.        
        */
  //      const UserObjectBindings& getUserObjectBindings() const { return mUserObjectBindings; }


        /** Sets whether or not this object will cast shadows.
        @remarks
        This setting simply allows you to turn on/off shadows for a given object.
        An object will not cast shadows unless the scene supports it in any case
        (see SceneManager::setShadowTechnique), and also the material which is
        in use must also have shadow casting enabled. By default all entities cast
        shadows. If, however, for some reason you wish to disable this for a single 
        object then you can do so using this method.
        @note This method normally refers to objects which block the light, but
        since Light is also a subclass of MovableObject, in that context it means
        whether the light causes shadows itself.
        */
        void setCastShadows(bool enabled);
        
        /** Returns whether shadow casting is enabled for this object. */
        bool getCastShadows(void) const;

        /** Sets whether or not the debug display of this object is enabled.
        @remarks
            Some objects aren't visible themselves but it can be useful to display
            a debug representation of them. Or, objects may have an additional 
            debug display on top of their regular display. This option enables / 
            disables that debug display. Objects that are not visible never display
            debug geometry regardless of this setting.
        */
        void setDebugDisplayEnabled(bool enabled);
        
        /// Gets whether debug display of this object is enabled. 
        bool isDebugDisplayEnabled(void) const;
    };
    
/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} // namespace AngelOgre (dummy, just to distinguish AngelScript from C++)
    