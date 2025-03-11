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
    TOKEN_TYPE_FLOAT,
	TOKEN_TYPE_INT,
    TOKEN_TYPE_BOOL,
    TOKEN_TYPE_KEYWORD
};

/**
 * @brief Binding of RoR::GenericDocContext; Traverses document tokens; See 'demo_script.as' for an example.
 */
class GenericDocContextClass
{
    // PLEASE maintain the same order as in 'GenericFileFormat.h' and 'GenericFileFormatAngelscript.cpp'
    
    GenericDocContext(GenericDocumentPtr@ d);

    bool moveNext();
    uint getPos();
    bool seekNextLine();
    int countLineArgs();
    bool endOfFile(int offset = 0);
    TokenType tokenType(int offset = 0);

    string getTokString(int offset = 0);
    float getTokFloat(int offset = 0);
    float getTokInt(int offset = 0);
    bool gettokBool(int offset = 0);
    string getTokKeyword(int offset = 0);
    string getTokComment(int offset = 0);

    bool isTokString(int offset = 0);
    bool isTokFloat(int offset = 0);
    bool isTokInt(int offset = 0);
    bool isTokBool(int offset = 0);
    bool isTokKeyword(int offset = 0);
    bool isTokComment(int offset = 0);
    bool isTokLineBreak(int offset = 0);
    
    // Editing functions:

    void appendTokens(int count); //!< Appends a series of `TokenType::NONE` and sets Pos at the first one added; use `setTok*` functions to fill them.
    bool insertToken(int offset = 0); //!< Inserts `TokenType::NONE`; @return false if offset is beyond EOF
    bool eraseToken(int offset = 0); //!< @return false if offset is beyond EOF
    
    void appendTokString(const string&in str);
    void appendTokFloat(float val);
    void appendTokInt(float val);
    void appendTokBool(bool val);
    void appendTokKeyword(const string&in str);
    void appendTokComment(const string&in str);
    void appendTokLineBreak();    

    bool setTokString(int offset, const string&in str);
    bool setTokFloat(int offset, float val);
    bool setTokInt(int offset, int val);
    bool setTokBool(int offset, bool val);
    bool setTokKeyword(int offset, const string&in str);
    bool setTokComment(int offset, const string&in str);
    bool setTokLineBreak(int offset);
};

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} //namespace Script2Game
