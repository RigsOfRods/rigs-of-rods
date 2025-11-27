
namespace AngelOgre { // Dummy namespace, just to distinguish AngelScript from C++

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */  

    /// Enumerates the types of light sources available.
    enum LightTypes
    {
        /// Point light sources give off light equally in all directions, so require only position not direction
        LT_POINT,
        /// Directional lights simulate parallel light beams from a distant source, hence have direction but no position
        LT_DIRECTIONAL,
        /// Spotlights simulate a cone of light from a source so require position and direction, plus extra values for falloff
        LT_SPOTLIGHT
    };

    /** Representation of a dynamic light source in the scene.
        @remarks
            Lights are added to the scene like any other MovableObject. They contain various 
            configurable parameters like type, position, direction and color.
        @par
            Remember that simply creating a light and adding it to a scene does not make a scene 
            brighter; the SceneManager must choose to interpret it. Use the SceneManager::setAmbientLight
            method to set the overall ambient light level. Use the Light methods to configure your 
            individual lights.
    */
    class Light : MovableObject
    {
    public:

        /** Sets the type of light - see LightTypes for more info.
        */
        void setType(LightTypes type);

        /** Returns the light type.
        */
        LightTypes getType() const;

        /** Sets the color of the diffuse light given off by this source.
        @remarks
            Material objects have ambient, diffuse and specular values which indicate how much of 
            each type of light an object reflects. This value denotes the amount and color of this 
            type of light the light exudes into the scene. The actual appearance of objects is a 
            combination of the two.
        @param r Red component (0.0 to 1.0)
        @param g Green component (0.0 to 1.0)
        @param b Blue component (0.0 to 1.0)
        */
        void setDiffuseColour(float r, float g, float b);

        /** Sets the color of the diffuse light given off by this source.
        @param color The color value
        */
        void setDiffuseColour(const color& color);

        /** Returns the color of the diffuse light given off by this light source (see setDiffuseColour for more info).
        */
        const color& getDiffuseColour() const;

        /** Sets the color of the specular light given off by this source.
        @remarks
            As with setDiffuseColour, this property is a modifier to a material's specular component.
        @param r Red component (0.0 to 1.0)
        @param g Green component (0.0 to 1.0)
        @param b Blue component (0.0 to 1.0)
        */
        void setSpecularColour(float r, float g, float b);

        /** Sets the color of the specular light given off by this source.
        @param color The color value
        */
        void setSpecularColour(const color& color);

        /** Returns the color of specular light given off by this light source.
        */
        const color& getSpecularColour() const;

        /** Sets the attenuation parameters for this light source.
        @remarks
            Attenuation means the how the light intensity reduces with distance. The attenuation is calculated as 
            <tt>1 / (constant + linear * distance + quadric * distance^2)</tt>
        @param range
            The absolute upper range of the light in world units. If set to 0, light is attenuated to zero by the 
            quadratic factor. In this case, the light will be visible up to infinity.
        @param constant
            Constant factor, 1.0 means never attenuate, 0.0 is complete attenuation.
        @param linear
            The linear factor, 1 means attenuate evenly over the distance.
        @param quadratic
            The quadratic factor, adds a curvature to the attenuation formula.
        */
        void setAttenuation(float range, float constant, float linear, float quadratic);

        /** Returns the absolute upper range of the light.
        */
        float getAttenuationRange() const;

        /** Returns the constant factor in the attenuation formula.
        */
        float getAttenuationConstant() const;

        /** Returns the linear factor in the attenuation formula.
        */
        float getAttenuationLinear() const;

        /** Returns the quadric factor in the attenuation formula.
        */
        float getAttenuationQuadric() const;

        /** Sets the position of the light.
        @remarks
            Applicable to point lights and spotlights only. This will be overridden if the light is attached to a SceneNode.
        @param x X coordinate
        @param y Y coordinate
        @param z Z coordinate
        */
        void setPosition(float x, float y, float z);

        /** Sets the position of the light.
        @remarks
            Applicable to point lights and spotlights only. This will be overridden if the light is attached to a SceneNode.
        @param pos Vector with the position
        */
        void setPosition(const vector3& pos);

        /** Returns the position of the light.
        @note Applicable to point lights and spotlights only.
        */
        const vector3& getPosition() const;

        /** Sets the direction in which a light points.
        @remarks
            Applicable only to the spotlight and directional light types. This will be overridden if the light is 
            attached to a SceneNode.
        @param x X component
        @param y Y component
        @param z Z component
        */
        void setDirection(float x, float y, float z);

        /** Sets the direction in which a light points.
        @remarks
            Applicable only to the spotlight and directional light types. This will be overridden if the light is 
            attached to a SceneNode.
        @param direction Vector with direction
        */
        void setDirection(const vector3& direction);

        /** Returns the light's direction.
        @note Applicable to directional and spotlights only.
        */
        const vector3& getDirection() const;

        /** Sets the range of a spotlight, i.e. the angle of the inner and outer cones and the rate of falloff between them.
        @param innerAngle
            Angle covered by the bright inner cone. The inner cone is at maximum brightness.
        @param outerAngle
            Angle covered by the outer cone. The light attenuates from the inner cone to the outer cone. 
            Outside the outer cone the light has zero intensity.
        @param falloff
            The rate of falloff between the inner and outer cones. 1.0 means a linear falloff, less means slower 
            falloff, higher means faster falloff. Applicable to spotlights only.
        */
        void setSpotlightRange(const radian& innerAngle, const radian& outerAngle, float falloff = 1.0f);

        /** Returns the angle covered by the spotlights inner cone.
        */
        const radian& getSpotlightInnerAngle() const;

        /** Returns the angle covered by the spotlights outer cone.
        */
        const radian& getSpotlightOuterAngle() const;

        /** Returns the falloff between the inner and outer cones of the spotlight.
        */
        float getSpotlightFalloff() const;

        /** Set a scaling factor to indicate the relative power of a light.
        @remarks
            This factor is only useful in High Dynamic Range (HDR) rendering. You can bind a value of 1.0 (the default) 
            to indicate a light at normal intensity, and another value to indicate a brighter or dimmer light.
        @param power
            The power rating of this light, default is 1.0.
        */
        void setPowerScale(float power);

        /** Returns the scaling factor which indicates the relative power of a light.
        */
        float getPowerScale() const;

        /** Sets whether or not this light should cast shadows.
        @remarks
            This setting simply allows you to turn on/off shadows for a given light. 
            An object will not cast shadows unless the scene supports it in any case 
            (see SceneManager::setShadowTechnique), and also the material which is 
            in use must also have shadow casting enabled.
        @note
            For Light objects, this method refers to whether the light causes shadows itself.
        */
        void setCastShadows(bool enabled);

        /** Returns whether or not this light casts shadows.
        */
        bool getCastShadows() const;

        /** Gets the position of the light including any transform from nodes it is attached to.
        */
        const vector3& getDerivedPosition() const;

        /** Gets the direction of the light including any transform from nodes it is attached to.
        */
        const vector3& getDerivedDirection() const;

        // Inherited from MovableObject

        /** Returns the name of this object. */
        const string& getName() const;

        /** Returns the type name of this object. */
        const string& getMovableType() const;

        /** Returns the node to which this object is attached.
        */
        Node@ getParentNode() const;

        /** Returns the scene node to which this object is attached.
        */
        SceneNode@ getParentSceneNode() const;

        /** Returns true if this object is attached to a SceneNode or TagPoint. */
        bool isAttached() const;

        /** Detaches an object from a parent SceneNode or TagPoint, if attached. */
        void detachFromParent();

        /** Returns true if this object is attached to a SceneNode or TagPoint, 
            and this SceneNode / TagPoint is currently in an active part of the
            scene graph. */
        bool isInScene() const;

        /** Retrieves the radius of the origin-centered bounding sphere 
             for this object.
        */
        float getBoundingRadius() const;

        /** Tells this object whether to be visible or not, if it has a renderable component. 
        */
        void setVisible(bool visible);

        /** Gets this object whether to be visible or not, if it has a renderable component. 
        */
        bool getVisible() const;

        /** Returns whether or not this object is supposed to be visible or not. 
        */
        bool isVisible() const;

        /** Sets the distance at which the object is no longer rendered.
        @param dist Distance beyond which the object will not be rendered.
        */
        void setRenderingDistance(float dist);

        /** Gets the distance at which batches are no longer rendered. */
        float getRenderingDistance() const;

        /** Sets the minimum pixel size an object needs to be in both screen axes in order to be rendered
        @param pixelSize Number of minimum pixels.
        */
        void setRenderingMinPixelSize(float pixelSize);

        /** Returns the minimum pixel size an object needs to be in both screen axes in order to be rendered
        */
        float getRenderingMinPixelSize() const;
    };
    
/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} // namespace AngelOgre (dummy, just to distinguish AngelScript from C++)