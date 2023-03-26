 
namespace AngelOgre { // Dummy namespace, just to distinguish AngelScript from C++

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */  

    /**
    * A manager for skeletal animation tracks in the entity.
    */
    class  AnimationStateSet 
    {
    public:


        /** Create a new AnimationState instance. 
        @param animName The name of the animation
        @param timePos Starting time position
        @param length Length of the animation to play
        @param weight Weight to apply the animation with 
        @param enabled Whether the animation is enabled
        */
        AnimationState@ createAnimationState(const string& animName,  
            float timePos, float length, float weight = 1.0, bool enabled = false);
            
        /// Get an animation state by the name of the animation
        AnimationState@ getAnimationState(const string& name) const;
        
        /// Tests if state for the named animation is present
        bool hasAnimationState(const string& name) const;
        
        /// Remove animation state with the given name
        void removeAnimationState(const string& name);
        
        /// Remove all animation states
        void removeAllAnimationStates(void);

        /** Get all the animation states in this set.
        * @return a readonly array view object.
        */
        AnimationStateArray@ getAnimationStates();

    };
    
/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs
    
} // namespace AngelOgre (dummy, just to distinguish AngelScript from C++)
    