
namespace Script2Game {

    /** \addtogroup ScriptSideAPIs
     *  @{
     */

     /** \addtogroup Script2Game
      *  @{
      */

      /**
       * @brief Binding of RoR::DashBoardManager. Allows you to set custom values for dashboards
       *        and modify them in your scripts.
       */
    class DashBoardManagerClass
    {
    public:
        /// @name General
        /// @{

        /**
         * Gets a boolean value from the dashboard.
         */
        bool getBool(int key);

        /**
         * Gets a number value from the dashboard.
         */
        float getNumeric(int key);

        /**
         * Gets a string value from the dashboard.
         */
        string getString(int key);

        /**
         * @return Whether the dashboard value is enabled or not.
         */
        bool getEnabled(int key);

        /**
         * Sets a boolean value to the dashboard.
         */
        void setBool(int key, bool value);

        /**
         * Sets an integer value to the dashboard.
         */
        void setInt(int key, int value);

        /**
         * Sets a floating point value to the dashboard.
         */
        void setFloat(int key, float value);

        /**
         * Sets a string value to the dashboard.
         */
        void setString(int key, const string& value);

        /**
         * Enables or disables the given dashboard value.
         * Call updateFeatures() afterwards to make your changes effective.
         */
        void setEnabled(int key, bool value);

        /**
         * @return The data type of the given dashboard value.
         */
        DashboardDataTypes getDataType(int key);

        /**
         * @return The key associated with a value name.
         */
        int getLinkIDForName(string& key);

        /**
         * Updates the visibility of all the dashboard controls linked to the manager.
         */
        void updateFeatures();

        /// @}    
    };

    /// @}    //addtogroup Script2Game
    /// @}    //addtogroup ScriptSideAPIs

} //namespace Script2Game
