
namespace Script2Game {

    /** \addtogroup ScriptSideAPIs
     *  @{
     */

     /** \addtogroup Script2Game
      *  @{
      */

      /**
       * @brief Binding of RoR::DashBoardManager. Allows you to modify custom dashboard inputs in AngelScript.
       */
    class DashBoardManagerClass
    {
    public:
        /// @name General
        /// @{

        /**
         * Gets the boolean value of the given dashboard link ID.
         */
        bool getBool(int key);

        /**
         * Gets the numeric value of the given dashboard link ID.
         */
        float getNumeric(int key);

        /**
         * Gets the string value of the given dashboard link ID.
         */
        string getString(int key);

        /**
         * @return Whether the dashboard input is enabled or not.
         */
        bool getEnabled(int key);

        /**
         * Sets a boolean value to the given dashboard link ID.
         */
        void setBool(int key, bool value);

        /**
         * Sets an integer value to the given dashboard link ID.
         */
        void setInt(int key, int value);

        /**
         * Sets a floating point to the given dashboard link ID.
         */
        void setFloat(int key, float value);

        /**
         * Sets a string value to the given dashboard link ID.
         */
        void setString(int key, const string& value);

        /**
         * Enables or disables the given dashboard input.
         * Call updateFeatures() afterwards to make your changes effective.
         */
        void setEnabled(int key, bool value);

        /**
         * @return The data type of the given dashboard input.
         */
        DashboardDataTypes getDataType(int key);

        /**
         * @return The link ID associated to the dashboard input name.
         */
        int getLinkIDForName(string& key);

        /**
         * Updates the visibility of all the dashboard controls linked to the manager, according to
         * the enabled/disabled status of their dashboard input link.
         */
        void updateFeatures();

        /// @}    
    };

    /// @}    //addtogroup Script2Game
    /// @}    //addtogroup ScriptSideAPIs

} //namespace Script2Game
