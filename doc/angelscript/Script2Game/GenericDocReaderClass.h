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
    // PLEASE maintain the same order as in 'GenericFileFormat.h' and 'GenericFileFormatAngelscript.cpp'
    
    GenericDocReader(GenericDocumentPtr@ d);

    bool moveNext();
    uint getPos();
    bool seekNextLine();
    int countLineArgs();
    bool endOfFile(int offset = 0);
    TokenType tokenType(int offset = 0);

    string getTokString(int offset = 0);
    float getTokFloat(int offset = 0);
    bool gettokBool(int offset = 0);
    string getTokKeyword(int offset = 0);
    string getTokComment(int offset = 0);

    bool isTokString(int offset = 0);
    bool isTokFloat(int offset = 0);
    bool isTokBool(int offset = 0);
    bool isTokKeyword(int offset = 0);
    bool isTokComment(int offset = 0);
};

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} //namespace Script2Game
