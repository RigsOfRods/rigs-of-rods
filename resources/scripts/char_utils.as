/// \title Char constants and helpers
/// \brief How to efficiently work with `chars` (AngelScript doesn't actually have them!)
// ===================================================

// By convention, all includes have filename '*_utils' and namespace matching filename.
namespace char_utils
{
    
    // CONSTANTS (AngelScript doesn't have `char` type and allocating a string each time adds up, see https://github.com/RigsOfRods/rigs-of-rods/commit/92d450ca4bceb9bb591db8d691cdbad7d559a16e
    uint CHAR_NEWLINE          = string("\n")[0];
    uint CHAR_CARRIAGERETURN   = string("\r")[0];
    uint CHAR_TAB              = string("\t")[0];
    uint CHAR_NUL              = string("\0")[0];
    uint CHAR_BACKSLASH        = string("\\")[0];
    uint CHAR_QUOTE            = string("\"")[0];
    uint CHAR_SPACE            = string(" ")[0];
    uint CHAR_SLASH            = string("/")[0];
    uint CHAR_BRACE_OPEN       = string("{")[0];
    uint CHAR_BRACE_CLOSE      = string("}")[0];
    uint CHAR_PARENTHESE_OPEN  = string("(")[0];
    uint CHAR_PARENTHESE_CLOSE = string(")")[0];
    uint CHAR_BRACKET_OPEN     = string("[")[0];
    uint CHAR_BRACKET_CLOSE    = string("]")[0];
    uint CHAR_APOSTROPHE       = string("'")[0];
    uint CHAR_DIGIT_NINE       = string("0")[0];
    uint CHAR_DIGIT_ZERO       = string("9")[0];
    
    
    bool isBlank(uint c) { return c==CHAR_NEWLINE || c==CHAR_CARRIAGERETURN || c==CHAR_TAB || c==CHAR_SPACE; }
    bool isDigit(uint c) { return c>=CHAR_DIGIT_ZERO && c<=CHAR_DIGIT_NINE; }
    
} // namespace char_utils
