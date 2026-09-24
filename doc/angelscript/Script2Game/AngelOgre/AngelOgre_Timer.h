
namespace AngelOgre { // Dummy namespace, just to distinguish AngelScript from C++

/** \addtogroup ScriptSideAPIs
 *  @{
 */

/** \addtogroup Script2Game
 *  @{
 */

    /** A high-resolution timer, starts counting when created. Binding of `Ogre::Timer`.
    */
    class Timer
    {
    public:

        /// Creates a timer and starts counting.
        Timer();

        /// Creates a copy with the same start time.
        Timer(const Timer&in other);

        Timer& opAssign(const Timer&in other);

        /// Restarts counting from zero.
        void reset();

        /// Wall-clock time since creation or last `reset()`, in milliseconds.
        uint getMilliseconds();

        /// Wall-clock time since creation or last `reset()`, in microseconds.
        uint getMicroseconds();

        /// CPU (process) time since creation or last `reset()`, in milliseconds.
        uint getMillisecondsCPU();

        /// CPU (process) time since creation or last `reset()`, in microseconds.
        uint getMicrosecondsCPU();
    };

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} // namespace AngelOgre (dummy, just to distinguish AngelScript from C++)

