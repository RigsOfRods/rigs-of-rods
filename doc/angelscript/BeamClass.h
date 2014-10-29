class BeamClass
{
public:
	/**
	 * Scales the truck.
	 */
	void scaleTruck(float ratio);
	
	/**
	 * Gets the name of the truck.
	 */
	string getTruckName();
	
	/**
	 * Resets the truck.
	 * @param reset_position true if you want to keep the truck at the current position.
	 *        (otherwise, it will be moved to its origin position)
	 */
	void reset(bool keep_position);
	
	/**
	 * Sets the detail level of the truck.
	 * @param v v=0: full detail, v=1: no beams
	 */
	void setDetailLevel(int v);
	
	/**
	 * Shows the skeleton of the truck.
	 */
	void showSkeleton(bool meshes, bool newMode);
	
	/**
	 * Hides the skeleton of the truck.
	 */
	void hideSkeleton(bool newMode);
	
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
	 * Toggles the beacons.
	 */
	void beaconsToggle();
	
	/**
	 * Enables or disables replay mode.
	 */
	void setReplayMode(bool rm);
	
	/**
	 * Resets the auto-pilot.
	 */
	void resetAutopilot();
	
	/**
	 * Toggles the custom particles.
	 */
	void toggleCustomParticles();
	
	/**
	 * Gets the default deformation.
	 */
	float getDefaultDeformation();
	
	/**
	 * Gets the total amount of nodes of the truck.
	 */
	int getNodeCount();
	
	/**
	 * Gets the total mass of the truck.
	 * @param withLocked if true, the mass of everything locked to the truck will be added to the mass of the truck.
	 */
	float getTotalMass(bool withLocked);
	
	/**
	 * Gets the total amount of nodes of the wheels of the truck.
	 */
	int getWheelNodeCount();
	
	/**
	 * Recalculates the mass of the truck.
	 */
	void recalc_masses();
	
	/**
	 * Sets the mass of the truck.
	 */
	void setMass(float m);
	
	/**
	 * Returns true if the brake light is enabled.
	 */
	bool getBrakeLightVisible();
	
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
	 * Sets the blinking type.
	 * @see blinktype
	 */
	void setBlinkType(int blink);
	
	/**
	 * Gets the blinking type.
	 * @see blinktype
	 */
	int getBlinkType();
	
	/**
	 * Gets the custom particles mode.
	 */
	bool getCustomParticleMode();
	
	/**
	 * Returns the index number of the lowest node.
	 */
	int getLowestNode();
	
	/**
	 * Sets the mesh visibility.
	 */
	bool setMeshVisibility(bool visible);

	/**
	 * Returns true if the reverse lights are enabled.
	 */
	bool getReverseLightVisible();
	
	/**
	 * Returns the angle in which the truck is heading.
	 */
	float getHeadingDirectionAngle();
	
	/**
	 * Returns true if a hook of this truck is locked.
	 */
	bool isLocked();
	
	/**
	 * Gets the current wheel speed of the vehicle.
	 */
	float getWheelSpeed();
	
	/**
	 * Gets the G-forces that this truck is currently experiencing.
	 * @return a vector3 representing the G-forces
	 */
	vector3 getGForces();
}
