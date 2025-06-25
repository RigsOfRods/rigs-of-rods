
  // =================================================== //
  // THIS IS NOT A C++ HEADER! Only a dummy for Doxygen. //
  // =================================================== //


/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Server
 *  @{
 */   

namespace Script2Server {
    
/**
* A global `server` object.
*/
class ServerScriptClass
{
public:

    /// @name Server state
    /// @{
    
        /** 
        */
        void Log(const string &in);
        
        /**
        */
        int getNumClients();    
        
        /**
        */
        int getTime();

        /**
        */
        int getStartTime();    
    
    /// @} 
    
    /// @name User management
    /// @{

        /**
        * @param type Script2Server::serverSayType
        */
        void say(const string &in, int uid, int type);

        /**
        */
        void kick(int kuid, const string &in);

        /**
        */
        void ban(int buid, const string &in);

        /**
        */
        bool unban(int buid);

        /**
        * Runs an AngelScript code snippet on a client.!
        */
        int cmd(int uid, string cmd);

        /**
        */
        string getUserName(int uid);

        /**
        */
        void setUserName(int uid, const string &in);

        /**
        */
        string getUserAuth(int uid);

        /**
        */
        int getUserAuthRaw(int uid);

        /**
        */
        void setUserAuthRaw(int uid, int);

        /**
        */
        int getUserColourNum(int uid);

        /**
        */
        void setUserColourNum(int uid, int);

        /**
        */
        void broadcastUserInfo(int);

        /**
        */
        string getUserToken(int uid);

        /**
        */
        string getUserVersion(int uid);

        /**
        */
        string getUserIPAddress(int uid);
    
    /// @} 
    
    /// @name Script management
    /// @{

        /**
        */
        void setCallback(const string &in, const string &in, ?&in);

        /**
        */
        void deleteCallback(const string &in, const string &in, ?&in);

        /**
        */
        void throwException(const string &in);
    
    /// @} 
    
    /// @name Config file values
    /// @{
        
        /**
        */
        uint get_maxClients();

        /**
        */
        string get_serverName();

        /**
        */
        string get_IPAddr();

        /**
        */
        uint get_listenPort();

        /**
        * @returns Script2Server::ServerType
        */
        int get_serverMode();

        /**
        */
        string get_owner();

        /**
        */
        string get_website();

        /**
        */
        string get_ircServ();

        /**
        */
        string get_voipServ();     

        /**
        */
        string getServerTerrain();        

    /// @}    

    /// @name Version info
    /// @{
    
        /** Returns build date, for example "Feb 13 2023"
        */
        string get_version();

        /** Returns `ANGELSCRIPT_VERSION_STRING`, for example "2.29.2"
        */
        string get_asVersion();

        /** Returns `RORNET_VERSION`, for example "RoRnet_2.44"
        */
        string get_protocolVersion();

    /// @}

    /// @name Utils
    /// @{
    
        /**
        */
        int rangeRandomInt(int, int);
        
        /**
         * Launches a background task, use [`curlStatus` callback](@ref Server2Script::curlStatus) to monitor progress and receive result.
         * @param displayname The "correlation ID" - the label passed to the callback to identify the transfer.
         */
        void curlRequestAsync(string url, string displayname);    
    
    /// @}
    
};

} // namespace Script2Server

/// @}    //addtogroup Script2Server
/// @}    //addtogroup ScriptSideAPIs
