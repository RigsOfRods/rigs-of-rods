
namespace Script2Game {

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */    

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

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} //namespace Script2Game
