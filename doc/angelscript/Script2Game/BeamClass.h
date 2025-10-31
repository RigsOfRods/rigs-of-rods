
namespace Script2Game {

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */    

/**
 * @brief Binding of RoR::Actor; a softbody-physics gameplay object, can be anything from soda can to space shuttle.
 */
class BeamClass
{
public:

    /// @name Physics state
    /// @{
    // PLEASE maintain the same ordering as in 'Actor.h' and 'scripting/bindings/ActorAngelscript.cpp'
    
    TruckState getTruckState();
    
    /**
    * Get vehicle absolute position
    */
    vector3 getPosition();
    
    /**
    * @deprecated Alias of `getPosition()`
    */
    vector3 getVehiclePosition();
    
    /**
     * Returns the angle in which the truck is heading.
     */  
    float getRotation();
    
    /**
     * Returns the angle in which the truck is heading.
     */
    float getHeadingDirectionAngle();
    
    /**
    * Orientation on all axes packed to single quaternion. Use `getYaw()`, `getPitch()` and `getRoll()` for individual rotations in radians.
    */
    quaternion getOrientation()
    
    /**
    * Meters per second.
    */
    float getSpeed();
    
    /**
     * Gets the G-forces that this truck is currently experiencing.
     * @return a vector3 representing the G-forces
     */
    vector3 getGForces();

    /**
     * Gets the total mass of the truck.
     * @param withLocked if true, the mass of everything locked to the truck will be added to the mass of the truck.
     */
    float getTotalMass(bool withLocked);

    /**
     * @return The current dry mass of the truck, in kilograms.
     */
    float getDryMass();

    /**
     * @return The current load mass of the truck, in kilograms.
     */
    float getLoadedMass();

    /**
     * @return The initial dry mass of the truck, in kilograms
     */
    float getInitialDryMass();

    /**
     * @return The initial load mass of the truck, in kilograms
     */
    float getInitialLoadedMass();

    /**
     * Gets the total amount of nodes of the truck.
     */
    int getNodeCount();
    
    /**
     * Returns the position of the node
     * @param the number of the node (counts from 0, see `getNodeCount()`)
     * @return vector3 of the world position for the node
     */
    vector3 getNodePosition(int nodeNumber); 
    
    /**
     * Returns the initial mass of the node
     * @param nodeNumber The number of the node (counts from 0, see `getNodeCount()`)
     * @return The node's initial mass, in kilograms
     */	
    float getNodeInitialMass(int nodeNumber);

    /**
     * Returns the current mass of the node
     * @param nodeNumber The number of the node (counts from 0, see `getNodeCount()`)
     * @return The node's current mass, in kilograms
     */
    float getNodeMass(int nodeNumber);

    /**
     * Returns the velocity vector of the node.
     * @param nodeNumber The number of the node (counts from 0, see `getNodeCount()`)
     * @return The node's current velocity vector
     */
    vector3 getNodeVelocity(int nodeNumber);

    /**
     * Returns the sum of all the forces applied to the node.
     * @param nodeNumber The number of the node (counts from 0, see `getNodeCount()`)
     * @return The node's current force vector.
     */
    vector3 getNodeForces(int nodeNumber);

    /**
     * Returns the mass options of the node.
     * @param nodeNumber The number of the node (counts from 0, see `getNodeCount()`)
     * @param loaded Reference parameter. Indicates whether the node bears a part of the cargo load.
     * @param overrideMass Reference parameter. Indicates whether the node mass can be overriden.
     * @return The node's mass options (using the reference parameters).
     */
    void getNodeMassOptions(int nodeNumber, bool& loaded, bool& overrideMass);

    /**
     * Is node marked as wheel rim? Note some wheel models use only tire nodes. See https://docs.rigsofrods.org/vehicle-creation/fileformat-truck/#wheels
     * @param the number of the node (counts from 0, see `getNodeCount()`)
     * @return True if the node is a wheel rim.
     */	
    bool isNodeWheelRim(int nodeNumber);     
    
    /**
     * Is node marked as wheel tire? Note some wheel models use only tire nodes. See https://docs.rigsofrods.org/vehicle-creation/fileformat-truck/#wheels
     * @param the number of the node (counts from 0, see `getNodeCount()`)
     * @return True if this is a tire node.
     */	
    bool isNodeWheelTire(int nodeNumber);
    
    /**
     * Gets the total amount of nodes of the wheels of the truck.
     */
    int getWheelNodeCount();    
    
    /**
     * Gets the current wheel speed of the vehicle.
     */
    float getWheelSpeed();   

    /**
     * Resets the truck.
     * @param reset_position true if you want to keep the truck at the current position.
     *        (otherwise, it will be moved to its origin position)
     */
    void reset(bool keep_position);

    /**
     * @return The air brake (speed brake) level for aircraft, from 0 (no braking) to 5 (maximum braking).
     */
    float getAirbrakeIntensity();

    /**
     * @return The flaps level for aircraft, from 0 (flaps up) to 5 (flaps fully down).
     */
    int getAircraftFlaps();
    
    //! @}
    
    /// @name Physics editing
    /// @{
    // PLEASE maintain the same ordering as in 'Actor.h' and 'scripting/bindings/ActorAngelscript.cpp'  
    
    /**
     * Scales the truck.
     */
    void scaleTruck(float ratio);
    
    /**
     * Sets the dry mass of the truck.
     * @note Use `recalculateNodeMasses()` to make your changes effective.
     */
    void setMass(float m);

    /**
     * @brief Sets the load mass of the truck.
     * @note Use `recalculateNodeMasses()` to make your changes effective.
     */
    void setLoadedMass(float loadMass);

    /**
     * @brief Overrides the node's mass.
     * @note If the node bears a part of the load mass (is marked with the `l` option) and its mass can't be overriden, this function has no effect.
     * @note Use `recalculateNodeMasses()` to make your changes effective.
     * @warning This function might break the N/B if you set the wrong mass. Always set masses adequate to the truck you're working on!
     */
    void setNodeMass(int nodeNumber, float mass);

    /**
     * Sets the mass options of the node.
     * @param nodeNumber The number of the node (counts from 0, see `getNodeCount()`)
     * @param loaded Allows the node to bear a part of the cargo load.
     * @param overrideMass Allows the node mass to be overriden.
     * @note Use `recalculateNodeMasses()` to make your changes effective.
     */
    void setNodeMassOptions(int nodeNumber, bool loaded, bool overrideMass);

    /**
     * Allows advanced users to set physics settings directly, including some not accessible from rig-def file format.
     * HAZARDOUS - values may not be checked; Pay attention to 'safe values' at each attribute description.
     */
    void setSimAttribute(ActorSimAttr attr, float val);

    float getSimAttribute(ActorSimAttr attr);

    /**
     * @brief Recalculates the masses of all the nodes.
     * @warning Do not call this function repeatedly! Otherwise, performance might be degraded.
     */
    void recalculateNodeMasses();

    /**
     * @brief Sets the air braking level for aircraft, from 0 (no braking) to 5 (maximum braking).
     */
    void setAirbrakeIntensity(float level);

    /**
     * @brief Sets the flaps level for aircraft, from 0 (flaps up) to 5 (flaps fully down).
     */
    void setAircraftFlaps(int level);
    
    //! @}
    
    /// @name User interaction
    /// @{
    // PLEASE maintain the same ordering as in 'Actor.h' and 'scripting/bindings/ActorAngelscript.cpp'  
    
    /**
     * Toggles the parking brake.
     */
    void parkingbrakeToggle();
    
    /**
     * Toggles the tracktion control.
     */
    void tractioncontrolToggle();
    
    /**
     * Toggles the anti-lock brakes.
     */
    void antilockbrakeToggle();
    
    /**
     * Toggles the custom particles.
     */
    void toggleCustomParticles();

    /**
     * Gets the custom particles mode.
     */
    bool getCustomParticleMode();
    
    /**
     * Returns true if a hook of this truck is locked.
     */
    bool isLocked();      
    
    /**
    * Sets a forced cinecam for this actor; This disables camera hotkeys.
    * @param cinecamId Cinecams are numbered from 0, use `getNumCinecams()`; -1 means no cinecam.
    * @param flags Flags are reserved for future use.
    */
    void setForcedCinecam(int cinecamId, int flags);
    
    /**
    * Resets the effect of `setForcedCinecam()`;
    */
    void clearForcedCinecam();
    
    /**
    * Reports the values submitted by `setForcedCinecam()`;
    * @param cinecamId Cinecams are numbered from 0, use `getNumCinecams()`; -1 means no cinecam.
    * @param flags Flags are reserved for future use.
    * @return True if a forced camera was set, False if not.
    */ 
    bool getForcedCinecam(int& inout, int& inout);
    
    /**
    * Reports number of installed cinecams.
    */
    int getNumCinecams() const;
    
    
    //! @}
    
    /// @name Subsystems
    /// @{
    // PLEASE maintain the same ordering as in 'Actor.h' and 'scripting/bindings/ActorAngelscript.cpp'      


    /**
    * Retrieve dashboard manager.
    */
    DashboardManagerClassPtr @getDashboardManager();
    
    /**
     * Retrieve the waypoint AI object.
     */
    VehicleAiClass @getVehicleAI();

    /**
    * Retrieve engine/transmission simulator.
    */
    EngineClassPtr @getEngine();

    /**
    * @return The amount of aircraft engines defined for this actor.
    */
    int getAircraftEngineCount();

    /**
    * @return The aircraft engine at the specified index, or `null` if the engine does not exist.
    */
    AircraftEngineClass @getAircraftEngine(int index);

    /**
    * @return The turbojet engine at the specified index, or `null` if the engine is not a turbojet or doesn't exist.
    */
    TurbojetClass @getTurbojet(int index);

    /**
    * @return The propeller engine at the specified index, or `null` if the engine is not a propeller or doesn't exist.
    */
    TurbopropClass @getTurboprop(int index);

    /**
    * Retrieves the autopilot system object, or `null` if unavailable.
    */
    AutopilotClass @getAutopilot();

    /**
    * @return The amount of screwprops defined for this actor.
    */
    int getScrewpropCount();

    /**
    * @return The screwprop at the specified index, or `null` if the screwprop doesn't exist.
    */
    ScrewpropClass @getScrewprop(int index);

    //! @}
    
    /// @name Vehicle lights
    /// @{
    // PLEASE maintain the same order as in 'Actor.h' and 'scripting/bindings/ActorAngelscript.cpp'
    
    /**
     * Gets the blinking type.
     * @see blinktype
     */
    int getBlinkType();    
    
    /**
     * Sets the blinking type.
     * @see blinktype
     */
    void setBlinkType(int blink);    
    
    /**
     * Returns true if the custom light with the number number is enabled.
     */
    bool getCustomLightVisible(int number);
    
    /**
     * Enables or disables the custom light.
     */
    void setCustomLightVisible(int number, bool visible);  

    /**
     * Gets the mode of the beacon.
     */
    bool getBeaconMode();    
    
    /**
     * Toggles the beacons.
     */
    void beaconsToggle();    
    
    /**
     * Returns true if the brake light is enabled.
     */
    bool getBrakeLightVisible();   

    /**
     * Returns true if the reverse lights are enabled.
     */
    bool getReverseLightVisible();

    /**
     * Counts flares using the given custom light group number (1-10).
     */
    int countCustomLights(int);
    
    /**
     * Counts flares using the given type.
     */
    int countFlaresByType(FlareType);
    
    //! @}
    
    /// @name Organizational
    /// @{
    // PLEASE maintain the same order as in 'Actor.h' and 'scripting/bindings/ActorAngelscript.cpp'

    /**
     * Gets the designated name of the truck.
     */
    string getTruckName();
    
    /**
     * Gets the name of the truck definition file.
     */
    string getTruckFileName();    
    
    /**
     * Gets the name of the OGRE resource group where the truck definition file lives. Useful for loading using `GenericDocumentClass`.
     */
    string getTruckFileResourceGroup();      

    /**
     * Gets the name of the loaded section for a truck.
     */
    string getSectionConfig();
    
    /**
     * Gets the unique Actor Instance ID; The same value as provided by `Game2Script::eventCallbackEx()`.
     */
    int getInstanceId();    
    
    //! @}    
}

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} //namespace Script2Game

