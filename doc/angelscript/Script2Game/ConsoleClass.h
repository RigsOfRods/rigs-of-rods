
namespace Script2Game {

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */    

/**
 * @brief Binding of RoR::Console; provides console variables (cvars), usually defined in RoR.cfg file.
 * @note This object is created automatically as global variable `console`.
 */
class ConsoleClass
{
public:
    /**
     * @brief Add CVar and parse default value if specified
     */
    CVar* cVarCreate(std::string const& name, std::string const& long_name,
        int flags, std::string const& val = std::string());

    /**
     * Parse value by cvar type
     */
    void cVarAssign(CVar* cvar, std::string const& value);

    /**
     * Find cvar by short/long name
     */
    CVar* cVarFind(std::string const& input_name);

    /**
     * Set existing cvar by short/long name. Return the modified cvar (or NULL if not found)
     */
    CVar* cVarSet(std::string const& input_name, std::string const& input_val);

    /**
     * Get cvar by short/long name, or create new one using input as short name.
     */
    CVar* cVarGet(std::string const& input_name, int flags);
};

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} //namespace Script2Game
