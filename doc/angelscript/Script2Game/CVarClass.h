
/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */    

namespace Script2Game {
    
/**
 * @brief Types and special attributes of cvars.
 * @note Default cvar type is string - To create a string cvar, enter '0' as flags.
 */
enum CVarFlags
{
    CVAR_TYPE_BOOL    = BITMASK(1),
    CVAR_TYPE_INT     = BITMASK(2),
    CVAR_TYPE_FLOAT   = BITMASK(3),
    CVAR_ARCHIVE      = BITMASK(4),    //!< Will be written to RoR.cfg
    CVAR_NO_LOG       = BITMASK(5)     //!< Will not be written to RoR.log
};

/**
 * @brief Binding of RoR::CVar; A console variable, usually defined in RoR.cfg but also created by users or scripts.
 */
class CVarClass
{
public:

    /**
     * @brief Get the value converted to string, works with any CVAR_TYPE.
     */
    std::string const&      getStr() const   { return m_value_str; }
    
    float                   getFloat() const { return m_value_num; }
    
    int                     getInt() const   { return (int)m_value_num; }
    
    bool                    getBool() const  { return (bool)m_value_num; }
    
    std::string const&      getName() const  { return m_name; }
};

} //namespace Script2Game

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs
