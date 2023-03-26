
namespace AngelOgre { // Dummy namespace, just to distinguish AngelScript from C++

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */  

    /**
    * Represents a single named skeletal animation track present in the mesh.
    */
    class  AnimationState 
    {
    public:

        /// Gets the name of the animation to which this state applies
        const string& getAnimationName() const;

        /// Gets the time position for this animation
        float getTimePosition(void) const;

        /// Sets the time position for this animation
        void setTimePosition(float timePos);

        /// Gets the total length of this animation (may be shorter than whole animation)
        float getLength() const;

        /// Sets the total length of this animation (may be shorter than whole animation)
        void setLength(float len);

        /// Gets the weight (influence) of this animation
        float getWeight(void) const;

        /// Sets the weight (influence) of this animation
        void setWeight(float weight);

        /** Modifies the time position, adjusting for animation length
        @param offset The amount of time, in seconds, to extend the animation.
        @remarks
            This method loops at the edges if animation looping is enabled.
        */
        void addTime(float offset);

        /// Returns true if the animation has reached the end and is not looping
        bool hasEnded(void) const;

        /// Returns true if this animation is currently enabled
        bool getEnabled(void) const;

        /// Sets whether this animation is enabled
        void setEnabled(bool enabled);

        /** Sets whether or not an animation loops at the start and end of
            the animation if the time continues to be altered.
        */
        void setLoop(bool loop);

        /// Gets whether or not this animation loops            
        bool getLoop(void) const;

        /// Get the parent animation state set
        AnimationStateSet* getParent(void) const;

        /** @brief Create a new blend mask with the given number of entries
        *
        * In addition to assigning a single weight value to a skeletal animation,
        * it may be desirable to assign animation weights per bone using a 'blend mask'.
        *
        * @param blendMaskSizeHint 
        *   The number of bones of the skeleton owning this AnimationState.
        * @param initialWeight
        *   The value all the blend mask entries will be initialised with (negative to skip initialisation)
        */
        void createBlendMask(uint64 blendMaskSizeHint, float initialWeight = 1.0f);

        /// Destroy the currently set blend mask
        void destroyBlendMask();

        /// Return whether there is currently a valid blend mask set
        bool hasBlendMask() const;

        /// Set the weight for the bone identified by the given handle
        void setBlendMaskEntry(uint64 boneHandle, float weight);

        /// Get the weight for the bone identified by the given handle
        float getBlendMaskEntry(uint64 boneHandle) const;
    };
    
/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} // namespace AngelOgre (dummy, just to distinguish AngelScript from C++)
  