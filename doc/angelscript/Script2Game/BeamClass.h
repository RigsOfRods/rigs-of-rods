
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
	* Is node marked as wheel rim? Note some wheel models use only tire nodes. See https://docs.rigsofrods.org/vehicle-creation/fileformat-truck/#wheels
	* @param the number of the node (counts from 0, see `getNodeCount()`)
    * @return True if the node is a wheel rim.
	*/	
	vector3 isNodeWheelRim(int nodeNumber);     
    
   /**
	* Is node marked as wheel tire? Note some wheel models use only tire nodes. See https://docs.rigsofrods.org/vehicle-creation/fileformat-truck/#wheels
	* @param the number of the node (counts from 0, see `getNodeCount()`)
    * @return True if this is a tire node.
	*/	
	vector3 isNodeWheelTire(int nodeNumber);         
    
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
    
    //! @}
    
    /// @name Physics editing
    /// @{
    // PLEASE maintain the same ordering as in 'Actor.h' and 'scripting/bindings/ActorAngelscript.cpp'  
    
	/**
	 * Scales the truck.
	 */
	void scaleTruck(float ratio);
	
	/**
	 * Sets the mass of the truck.
	 */
	void setMass(float m);
	
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
    
    //! @}
    
    /// @name Subsystems
    /// @{
    // PLEASE maintain the same ordering as in 'Actor.h' and 'scripting/bindings/ActorAngelscript.cpp'      

	
	/**
	 * Retrieve the waypoint AI object.
	 */
	VehicleAiClass @getVehicleAI();
    

    /**
    * Retrieve engine/transmission simulator.
    */
    EngineSimClassPtr @getEngineSim();
	
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

