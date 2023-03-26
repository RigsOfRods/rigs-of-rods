namespace AngelOgre { // Dummy namespace, just to distinguish AngelScript from C++

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */  

    /** Class representing a node in the scene graph.
        @remarks
            A SceneNode is a type of Node which is used to organise objects in a scene.
            It has the same hierarchical transformation properties of the generic Node class,
            but also adds the ability to attach world objects to the node, and stores hierarchical
            bounding volumes of the nodes in the tree.
            Child nodes are contained within the bounds of the parent, and so on down the
            tree, allowing for fast culling.
    */
    class  SceneNode : public Node
    {

    public:


        /** Adds an instance of a scene object to this node.
        @remarks
            Scene objects can include Entity objects, Camera objects, Light objects, 
            ParticleSystem objects etc. Anything that subclasses from MovableObject.
        */
         void attachObject(MovableObject@ obj);

        /** Retrieves a pointer to an attached object.
        @remarks Retrieves by object name, see alternate version to retrieve by index.
        */
        MovableObject@ getAttachedObject(const string& name);

        /** Detaches the indexed object from this scene node.
        @remarks
            Detaches by index, see the alternate version to detach by name. Object indexes
            may change as other objects are added / removed.
        */
         MovableObject@ detachObject(uint16 index);

        /** Detaches an object by pointer. */
         void detachObject(MovableObject@ obj);

        /** Detaches the named object from this node and returns a pointer to it. */
         MovableObject@ detachObject(const string& name);

        /** Detaches all objects attached to this node.
        */
         void detachAllObjects(void);

        /** Determines whether this node is in the scene graph, i.e.
            whether it's ultimate ancestor is the root scene node.
        */
        bool isInSceneGraph(void) const;         

        /** The MovableObjects attached to this node
         *
         * This is a much faster way to go through <B>all</B> the objects attached to the node than
         * using getAttachedObject.
         */
        MovableObjectArray@ getAttachedObjects();

        /** Gets the creator of this scene node. 
        @remarks
            This method returns the SceneManager which created this node.
            This can be useful for destroying this node.
        */
        SceneManager@ getCreator(void) const;

        /** This method removes and destroys the named child and all of its children.
        @remarks
            Unlike removeChild, which removes a single named child from this
            node but does not destroy it, this method destroys the child
            and all of it's children. 
        @par
            Use this if you wish to recursively destroy a node as well as 
            detaching it from it's parent. Note that any objects attached to
            the nodes will be detached but will not themselves be destroyed.
        */
        void removeAndDestroyChild(const string& name);

        /// @overload
        void removeAndDestroyChild(uint16 index);

        /// @overload
        void removeAndDestroyChild(SceneNode@ child);


        /** Removes and destroys all children of this node.
        @remarks
            Use this to destroy all child nodes of this node and remove
            them from the scene graph. Note that all objects attached to this
            node will be detached but will not be destroyed.
        */
        void removeAndDestroyAllChildren(void);

        /** Allows the showing of the node's bounding box.
        @remarks
            Use this to show or hide the bounding box of the node.
        */
        void showBoundingBox(bool bShow);

        /** Allows the overriding of the node's bounding box
            over the SceneManager's bounding box setting.
        @remarks
            Use this to override the bounding box setting of the node.
        */
        void hideBoundingBox(bool bHide);

        /** This allows scene managers to determine if the node's bounding box
            should be added to the rendering queue.
        @remarks
            Scene Managers that implement their own _findVisibleObjects will have to 
            check this flag and then use _addBoundingBoxToQueue to add the bounding box
            wireframe.
        */
        bool getShowBoundingBox() const;

        /** Creates an unnamed new SceneNode as a child of this node.
        @param
            translate Initial translation offset of child relative to parent
        @param
            rotate Initial rotation relative to parent
        */
        SceneNode@ createChildSceneNode(
            const vector3& translate = vector3(0.f, 0.f, 0.f), 
            const Quaternion& rotate = quaternion() );

        /** Creates a new named SceneNode as a child of this node.
        @remarks
            This creates a child node with a given name, which allows you to look the node up from 
            the parent which holds this collection of nodes.
            @param
                translate Initial translation offset of child relative to parent
            @param
                rotate Initial rotation relative to parent
        */
        SceneNode@ createChildSceneNode(const string& name, const vector3& translate = vector3(0.f, 0.f, 0.f), const Quaternion& rotate = quaternion());

       

        /** Tells the node whether to yaw around it's own local Y axis or a fixed axis of choice.
        @remarks
        This method allows you to change the yaw behaviour of the node - by default, it
        yaws around it's own local Y axis when told to yaw with TS_LOCAL, this makes it
        yaw around a fixed axis. 
        You only floatly need this when you're using auto tracking (see setAutoTracking,
        because when you're manually rotating a node you can specify the TransformSpace
        in which you wish to work anyway.
        @param
        useFixed If true, the axis passed in the second parameter will always be the yaw axis no
        matter what the node orientation. If false, the node returns to it's default behaviour.
        @param
        fixedAxis The axis to use if the first parameter is true.
        */
        void setFixedYawAxis( bool useFixed, const vector3& fixedAxis = vector3(0.f, 1.f, 0.f) );

        /** Rotate the node around the Y-axis.
        */
        void yaw(const Radian& angle, TransformSpace relativeTo = TS_LOCAL);
        /** Sets the node's direction vector ie it's local -z.
        @remarks
        Note that the 'up' vector for the orientation will automatically be 
        recalculated based on the current 'up' vector (i.e. the roll will 
        remain the same). If you need more control, use setOrientation.
        @param vec The components of the direction vector
        @param relativeTo The space in which this direction vector is expressed
        @param localDirectionVector The vector which normally describes the natural
        direction of the node, usually -Z
        */
        void setDirection(const vector3& vec, TransformSpace relativeTo = TS_LOCAL,
            const vector3& localDirectionVector = vector3(0.f, 0.f, -1.f));
            
        /** Points the local -Z direction of this node at a point in space.
        @param targetPoint A vector specifying the look at point.
        @param relativeTo The space in which the point resides
        @param localDirectionVector The vector which normally describes the natural
        direction of the node, usually -Z
        */
        void lookAt( const vector3& targetPoint, TransformSpace relativeTo,
            const vector3& localDirectionVector = vector3(0.f, 0.f, -1.f));
            
        /** Enables / disables automatic tracking of another SceneNode.
        @remarks
        If you enable auto-tracking, this SceneNode will automatically rotate to
        point it's -Z at the target SceneNode every frame, no matter how 
        it or the other SceneNode move. Note that by default the -Z points at the 
        origin of the target SceneNode, if you want to tweak this, provide a 
        vector in the 'offset' parameter and the target point will be adjusted.
        @param enabled If true, tracking will be enabled and the next 
        parameter cannot be null. If false tracking will be disabled and the 
        current orientation will be maintained.
        @param target Pointer to the SceneNode to track. Make sure you don't
        delete this SceneNode before turning off tracking (e.g. SceneManager::clearScene will
        delete it so be careful of this). Can be null if and only if the enabled param is false.
        @param localDirectionVector The local vector considered to be the usual 'direction'
        of the node; normally the local -Z but can be another direction.
        @param offset If supplied, this is the target point in local space of the target node
        instead of the origin of the target node. Good for fine tuning the look at point.
        */
        void setAutoTracking(bool enabled, SceneNode@ const target = 0,
            const vector3& localDirectionVector = vector3(0.f, 0.f, -1.f),
            const vector3& offset = vector3(0.f, 0.f, 0.f));
            
        /** Get the auto tracking target for this node, if any. */
        SceneNode@ getAutoTrackTarget(void);
        
        /** Get the auto tracking offset for this node, if the node is auto tracking. */
        const vector3& getAutoTrackOffset(void);
        
        /** Get the auto tracking local direction for this node, if it is auto tracking. */
        const vector3& getAutoTrackLocalDirection(void);
        
        /** Gets the parent of this SceneNode. */
        SceneNode@ getParentSceneNode(void);
        
        /** Makes all objects attached to this node become visible / invisible.
        @remarks    
            This is a shortcut to calling setVisible() on the objects attached
            to this node, and optionally to all objects attached to child
            nodes. 
        @param visible Whether the objects are to be made visible or invisible
        @param cascade If true, this setting cascades into child nodes too.
        */
        void setVisible(bool visible, bool cascade = true);
        
        /** Inverts the visibility of all objects attached to this node.
        @remarks    
        This is a shortcut to calling setVisible(!isVisible()) on the objects attached
        to this node, and optionally to all objects attached to child
        nodes. 
        @param cascade If true, this setting cascades into child nodes too.
        */
        void flipVisibility(bool cascade = true);

        /** Tells all objects attached to this node whether to display their
            debug information or not.
        @remarks    
            This is a shortcut to calling setDebugDisplayEnabled() on the objects attached
            to this node, and optionally to all objects attached to child
            nodes. 
        @param enabled Whether the objects are to display debug info or not
        @param cascade If true, this setting cascades into child nodes too.
        */
        void setDebugDisplayEnabled(bool enabled, bool cascade = true);

    };
    
/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs
    
} // namespace AngelOgre (dummy, just to distinguish AngelScript from C++)


