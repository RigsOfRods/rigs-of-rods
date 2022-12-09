namespace Script2Game {

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */   

enum TokenType
{
    TOKEN_TYPE_NONE,
    TOKEN_TYPE_LINEBREAK,
    TOKEN_TYPE_COMMENT,
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_NUMBER,
    TOKEN_TYPE_BOOL,
    TOKEN_TYPE_KEYWORD
};

/**
 * @brief Binding of RoR::GenericDocReader; Traverses document tokens; See 'demo_script.as' for an example.
 */
class GenericDocReaderClass
{
    GenericDocReader(GenericDocumentPtr@ d);

    bool MoveNext();
    uint GetPos();
    bool SeekNextLine();
    int CountLineArgs();
    bool EndOfFile(int offset = 0);

    TokenType GetTokType(int offset = 0);
    string GetStringData(int offset = 0);
    float GetFloatData(int offset = 0);

    string GetTokString(int offset = 0);
    float GetTokFloat(int offset = 0);
    bool GetTokBool(int offset = 0);
    string GetTokKeyword(int offset = 0);
    string GetTokComment(int offset = 0);

    bool IsTokString(int offset = 0);
    bool IsTokFloat(int offset = 0);
    bool IsTokBool(int offset = 0);
    bool IsTokKeyword(int offset = 0);
    bool IsTokComment(int offset = 0);
};

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} //namespace Script2Game
