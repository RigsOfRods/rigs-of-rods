
namespace Script2Game {

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */    
 
enum autoswitch
{
    REAR,
    NEUTRAL,
    DRIVE,
    TWO,
    ONE,
    MANUALMODE
};

/**
 * @brief Binding of RoR::EngineSim; A land vehicle engine + transmission
 * @note Obtain the object using `BeamClass::getEngine()`.
 */
class EngineSimClass
{
public:

    /// @name Definition; keyword 'engine'
    /// @{
    float          getMinRPM() const; //!< Shift down RPM ('engine' attr #1)
    float          getMaxRPM() const; //!< Shift up RPM ('engine' attr #2)
    float          getEngineTorque() const; //!< Torque in N/m ('engine' attr #3)
    float          getDiffRatio() const; //!< Global gear ratio ('engine' attr #4)
    float          getGearRatio(int pos); //!< -1=R, 0=N, 1... ('engine' attrs #5[R],#6[N],#7[1]...)
    int            getNumGears() const;
    int            getNumGearsRanges() const;
    /// @}

    /// @name Definition, Keyword 'engoption'
    /// @{
    float          getEngineInertia() const; //!< ('engoption' attr #1)
    char           getEngineType() const; //!< 't' = truck (default), 'c' = car, 'e' = electric car ('engoption' attr #2)
    bool           isElectric() const;
    bool           hasAir() const;
    bool           hasTurbo() const;
    float          getClutchForce() const; //!< ('engoption' attr #3)
    float          getShiftTime() const; //!< Time (in seconds) that it takes to shift ('engoption' attr #4)
    float          getClutchTime() const; //!< Time (in seconds) the clutch takes to apply ('engoption' attr #5)
    float          getPostShiftTime() const; //!< Time (in seconds) until full torque is transferred ('engoption' attr #6)
    float          getStallRMP() const; //!< ('engoption' attr #7)
    float          getIdleRPM() const; //!< ('engoption' attr #8)
    float          getMaxIdleMixture() const; //!< Maximum throttle to maintain the idle RPM ('engoption' attr #9)
    float          getMinIdleMixture() const; //!< Minimum throttle to maintain the idle RPM ('engoption' attr #10)
    float          getBrakingTorque() const;
    /// @}
    
    /// @name General state getters
    /// @{
    float          getAcc();
    float          getClutch();
    float          getCrankFactor();
    float          getRPM();
    float          getSmoke();
    float          getTorque();
    float          getTurboPSI();
    SimGearboxMode getAutoMode();
    int            getGear();
    int            getGearRange();
    bool           isRunning();
    bool           hasContact(); //!< Ignition
    float          getEngineTorque();
    float          getInputShaftRPM();
    float          getDriveRatio();
    float          getEnginePower();
    float          getEnginePower(float rpm);
    float          getTurboPower();
    float          getIdleMixture();
    float          getPrimeMixture();
    autoswitch     getAutoShift();
    float          getAccToHoldRPM(); //!< estimate required throttle input to hold the current rpm
    /// @}

    /// @name Shifting diagnostic
    /// @{
    float          getPostShiftClock();
    float          getShiftClock();
    bool           isPostShifting();
    bool           isShifting();
    int            getShifTargetGear();
    float          getAutoShiftBehavior();
    int            getUpshiftDelayCounter();
    int            getKickdownDelayCounter();
    /// @}

    /// @name General state changes
    /// @{
    void           pushNetworkState(float engine_rpm, float acc, float clutch, int gear, bool running, bool contact, char auto_mode, char auto_select = -1);
    void           setAcc(float val);
    void           autoSetAcc(float val);
    void           setClutch(float clutch);
    void           setRPM(float rpm);
    void           setWheelSpin(float rpm);
    void           setAutoMode(SimGearboxMode mode);
    void           setPrime(bool p);
    void           setHydroPump(float work);
    void           setManualClutch(float val);
    void           setTCaseRatio(float ratio);   //!< Set current transfer case gear (reduction) ratio
    void           toggleContact();              //!< Ignition
    void           offStart();                   //!< Quick start of vehicle engine.
    void           startEngine();                //!< Quick engine start. Plays sounds.
    void           stopEngine();                 //!< stall engine
    /// @}

    /// @name Shifting
    /// @{
    void           toggleAutoMode();        
    void           autoShiftDown();
    void           autoShiftSet(autoswitch mode);
    void           autoShiftUp();
    void           setGear(int v);               //!< low level gear changing (bypasses shifting logic)
    void           setGearRange(int v);          //!< low level gear changing (bypasses shifting logic)
    void           shift(int val);               //!< Changes gear by a relative offset. Plays sounds.
    void           shiftTo(int val);             //!< Changes gear to given value. Plays sounds.
    /// @}

};

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} //namespace Script2Game
