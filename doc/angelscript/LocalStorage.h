/**
 * @brief A class that allows your script to store data persistently.
 * The data will be stored on the hard disk of the user. The data will stay available until the user clears his cache.
 * Your data will be saved automatically if the object is destroyed or when you call the save() method.
 * To get your data, you'll have to use the correct get function, as that function will have to convert the data from a string to the correct datatype.
 * @warning Try to use only 1 of these objects per file at the same time. Otherwise, the changes of one object may get overridden by the changes of the other object.
 */
class LocalStorage
{	
public:
	/**
	 * The constructor for the class.
	 * This will return a reference to the newly created object.
	 * @param filename The name of the file that should be used to save the data.
	 * @param sectionname Which section should be used by default.
	 * @return A reference to the new object.
	 */
	LocalStorage(string filename, const string sectionname = "common");
	
	/**
	 * A getter for string data.
	 * @param key
	 *      The key, associated with the data you want to get. @n
	 *      You can use a point ('.') to use another section than the default, as specified when constructing the object. @n
	 *      E.g.: string myValue = get("mySection.myKey"); will get the value associated with the key 'myKey' in section 'mySection'.
	 * @return the value associated with the key or an empty string if the key did not exist.
	 */
	string get(string key);
	
	/**
	 * @see get
	 */
	string getString(string key);
	
	/**
	 * @see setString
	 */
	void set(string key, const string value);
	
	/**
	 * This sets a key.
	 * @param key \see get
	 * @param value The string that should be stored.
	 */
	void setString(string key, const string value);

	float getFloat(string key);
	void set(string key, float value);
	void setFloat(string key, float value);

	vector3 getVector3(string key);
	void set(string key, const vector3 value);
	void setVector3(string key, const vector3 value);

	radian getRadian(string key);
	void set(string key, const radian value);
	void setRadian(string key, const radian value);

	degree getDegree(string key);
	void set(string key, const degree value);
	void setDegree(string key, const degree value);

	quaternion getQuaternion(string key);
	void set(string key, const quaternion value);
	void setQuaternion(string key, const quaternion value);

	bool getBool(string key);
	void set(string key, const bool value);
	void setBool(string key, const bool value);

	int getInt(string key);
	int getInteger(string key);
	void set(string key, int);
	void setInt(string key, int);
	void setInteger(string key, int);

	/**
	 * This saves the data to a file.
	 * The data will also be auto-saved when the object is destroyed.
	 */
	void save();
	
	/**
	 * This reloads the data from the saved file, overwriting your changes.
	 */
	bool reload();

	/**
	 * Check if certain key exists.
	 * @param key \see get
	 * @return true if the key exists.
	 */
	bool exists(string key) const;
	
	/**
	 * deletes a key and its associated value.
	 * @param key \see get
	 */
	void delete(string key);
	
	/**
	 * Changes the default section.
	 * @param section the new section.
	 */
	void changeSection(const string section);
};
