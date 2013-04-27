

/**
 * @brief A class that gives you access to the RoR.cfg file.
 * @note The settings object is already created for you, you don't need to create it yourself.
 *       E.g.: you can get a setting using settings.getSetting("Nickname");
 */
class SettingsClass
{
public:
	/**
	 * Gets a setting from the RoR.cfg file.
	 * @param key the name of the setting
	 * @return The current value of the setting
	 */
	string getSetting(string key);

	/**
	 * Sets a setting in the RoR.cfg file.
	 * @param key the name of the setting
	 * @param value The new value for the setting
	 */
	void setSetting(string key, string value);
};
