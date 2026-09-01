// ============================================================
/// \title AngelScript Tokenizer
/// \brief Compact lexer: identifiers, keywords, numbers, strings, comments, operators/punctuation.
// ============================================================

//#region Token defs
enum TokType {
    TT_EOF, TT_IDENT, TT_KEYWORD, TT_INT, TT_FLOAT, TT_LINEBREAK,
    TT_STRING, TT_COMMENT, TT_PUNCT, TT_UNKNOWN
}

string TokTypeName(TokType t) {
    switch (t) {
        case TT_IDENT:   return "IDENT";
        case TT_KEYWORD: return "KEYWORD";
        case TT_INT:     return "INT";
        case TT_FLOAT:   return "FLOAT";
        case TT_STRING:  return "STRING";
        case TT_COMMENT: return "COMMENT";
        case TT_PUNCT:   return "PUNCT";
        case TT_EOF:     return "EOF";
        default:         return "UNKNOWN";
    }
}

class Token {
    TokType   type;
    string    text;
    uint      pos;
    uint      line;
    Token(TokType t, const string &in s, uint p, uint l) {
        type = t; text = s; pos = p; line = l;
    }
}
//#endregion


// #region the tokenizer
class ASTokenizer {
    
    string src;
    uint   i, len, lineNo;
    array<Token@> toks;
    
    // -- char helpers (byte-wise, ASCII) --------------------
string ch(uint k) { return k < len ? src.substr(k, 1) : ""; }
bool isDigit(const string &in c) { return c >= "0" && c <= "9"; }
bool isHex(const string &in c)   { return isDigit(c) || (c>="a"&&c<="f") || (c>="A"&&c<="F"); }
bool isAlpha(const string &in c) { return (c>="a"&&c<="z") || (c>="A"&&c<="Z") || c=="_"; }
bool isAlnum(const string &in c) { return isAlpha(c) || isDigit(c); }
    
    // -- keyword table ---------------------------------------
    array<string> kw = {
        "and","abstract","auto","bool","break","case","cast","class","const",
        "continue","default","do","double","else","enum","explicit","external",
        "false","final","float","for","from","funcdef","function","get","if",
        "import","in","inout","int","int8","int16","int32","int64","interface",
        "is","mixin","namespace","not","null","or","out","override","private",
        "property","protected","return","set","shared","super","switch","this",
        "true","typedef","uint","uint8","uint16","uint32","uint64","void",
        "while","xor"
    };
    bool isKeyword(const string &in s) {
        for (uint k = 0; k < kw.length(); k++) if (kw[k] == s) return true;
        return false;
    }
    
    // -- multi-char punctuation, longest first ---------------
    array<string> ops = {
        ">>>=","**=","<<=",">>=",">>>","&&","||","^^","==","!=","<=",">=",
        "++","--","+=","-=","*=","/=","%=","&=","|=","^=","<<",">>","::","..",
        "**","@","!","~","&","|","^","%","*","/","+","-","<",">","=","?",":",
    ";",",",".","(",")","{","}","[","]"
    };
    string matchOp() {
        for (uint k = 0; k < ops.length(); k++) {
            uint L = ops[k].length();
            if (i + L <= len && src.substr(i, L) == ops[k]) return ops[k];
        }
        return "";
    }
    
void add(TokType t, const string &in s, uint p) { toks.insertLast(Token(t, s, p, lineNo)); }
    
    // -- main entry -------------------------------------------
    array<Token@>@ Tokenize(const string &in code) {
        src = code; len = src.length(); i = 0; lineNo = 1; toks.resize(0);
        
        while (i < len) {
            string c = ch(i);
            
            // linebreak
            if (c == "\n") {
                add(TT_LINEBREAK, "", 0);
            lineNo++; i++; continue; }
            
            // whitespace
        if (c == " " || c == "\t" || c == "\r") { i++; continue; }
            
            // line comment
            if (c == "/" && ch(i+1) == "/") {
                uint p = i;
                while (i < len && ch(i) != "\n") i++;
                add(TT_COMMENT, src.substr(p, i - p), p);
                continue;
            }
            
            // block comment
            if (c == "/" && ch(i+1) == "*") {
                uint p = i; i += 2;
                while (i < len && !(ch(i) == "*" && ch(i+1) == "/")) {
                    if (ch(i) == "\n") lineNo++;
                    i++;
                }
                i = i + 2 <= len ? i + 2 : len;
                add(TT_COMMENT, src.substr(p, i - p), p);
                continue;
            }
            
            // string literal: '...' "..." or heredoc '''...'''
            if (c == "\"" || c == "'") {
                uint p = i;
                string q = c;
                bool heredoc = (ch(i+1) == q && ch(i+2) == q);
                if (heredoc) {
                    i += 3;
                    while (i < len && !(ch(i)==q && ch(i+1)==q && ch(i+2)==q)) i++;
                    i = i + 3 <= len ? i + 3 : len;
                } else {
                    i++;
                    while (i < len && ch(i) != q) {
                        if (ch(i) == "\\") i++;   // skip escaped char
                        i++;
                    }
                    if (i < len) i++;             // closing quote
                }
                add(TT_STRING, src.substr(p, i - p), p);
                continue;
            }
            
            // number: int / hex / oct / bin / float
            if (isDigit(c) || (c == "." && isDigit(ch(i+1)))) {
                uint p = i;
                if (c == "0" && (ch(i+1) == "x" || ch(i+1) == "X")) {
                    i += 2; while (isHex(ch(i))) i++;
                } else if (c == "0" && (ch(i+1) == "b" || ch(i+1) == "B")) {
                    i += 2; while (ch(i) == "0" || ch(i) == "1") i++;
                } else if (c == "0" && (ch(i+1) == "o" || ch(i+1) == "O")) {
                    i += 2; while (ch(i) >= "0" && ch(i) <= "7") i++;
                } else {
                    bool isFloat = false;
                    while (isDigit(ch(i))) i++;
                if (ch(i) == ".") { isFloat = true; i++; while (isDigit(ch(i))) i++; }
                    if (ch(i) == "e" || ch(i) == "E") {
                        isFloat = true; i++;
                        if (ch(i) == "+" || ch(i) == "-") i++;
                        while (isDigit(ch(i))) i++;
                    }
                if (ch(i) == "f" || ch(i) == "F") { isFloat = true; i++; }
                    add(isFloat ? TT_FLOAT : TT_INT, src.substr(p, i - p), p);
                    continue;
                }
                add(TT_INT, src.substr(p, i - p), p);
                continue;
            }
            
            // identifier / keyword
            if (isAlpha(c)) {
                uint p = i;
                while (isAlnum(ch(i))) i++;
                string word = src.substr(p, i - p);
                add(isKeyword(word) ? TT_KEYWORD : TT_IDENT, word, p);
                continue;
            }
            
            // operator / punctuation
            string op = matchOp();
        if (op != "") { add(TT_PUNCT, op, i); i += op.length(); continue; }
            
            // unknown byte, skip
            add(TT_UNKNOWN, c, i);
            i++;
        }
        
        add(TT_EOF, "", i);
        return @toks;
    }
}

//#endregion


// #region Example data

string sample = 
"""
namespace foo {
    float x = 3.14e2f;
    string s = "hi \"there\"";
    // compute factorial
    int fact(int n) {
        if (n <= 1) return 1;
        return n * fact(n - 1);
    }
} // namespace foo
""";


array<Token@> tokens; // tokenization done in `main()`


//#endregion


// #region Rendering tokens with auto-indent

color TokColor(TokType t) {
    switch (t) {
        case TT_KEYWORD: return color(0.36f, 0.55f, 0.95f, 1.0f); // blue
        case TT_IDENT:   return color(0.85f, 0.85f, 0.85f, 1.0f); // light gray
        case TT_INT:
        case TT_FLOAT:   return color(0.70f, 0.55f, 0.95f, 1.0f); // purple
        case TT_STRING:  return color(0.90f, 0.55f, 0.30f, 1.0f); // orange
        case TT_COMMENT: return color(0.45f, 0.60f, 0.40f, 1.0f); // green
        case TT_PUNCT:   return color(0.80f, 0.80f, 0.50f, 1.0f); // yellow-gray
        default:         return color(0.60f, 0.15f, 0.15f, 1.0f); // red (unknown)
    }
}



void DrawTokens(array<Token@>@ tokens) {
    int indentLevel=0;
    bool firstOnLine=true;
    for (uint i = 0; i < tokens.length(); i++) {
        Token@ t = tokens[i];
        if (t.type == TT_EOF) break;
        
        if (t.type == TT_PUNCT) { 
        if (t.text=="{") indentLevel++; else if (t.text=="}") indentLevel--; 
        }
        
        if (t.type == TT_LINEBREAK) 
        {
            firstOnLine=true;
            continue;
        }
        
        
        if (firstOnLine) { 
        for(int n=0;n<indentLevel; n++) { ImGui::Text("    "); ImGui::SameLine(0.f,0.f);} firstOnLine=false;
        }
        else ImGui::SameLine(0.f, 0.f);
        
        color col = TokColor(t.type);
        
        // only comments and (potentially multi-line) strings need
        // to be split on embedded newlines; everything else is
        // guaranteed single-line by the tokenizer.
        if (t.type == TT_COMMENT || t.type == TT_STRING) {
            uint p = 0, L = t.text.length();
            for (uint k = 0; k <= L; k++) {
                if (k == L || t.text[k] == "\n"[0]) {
                    string seg = t.text.substr(p, k - p);
                    if (seg.length() > 0) {
                        ImGui::TextColored(col, seg);
                        
                    }
                    p = k + 1;
                }
            }
            continue;
        }
        
        ImGui::TextColored(col, t.text);
        
    }
}

//#endregion


// #region game glue

// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler;


void main()
{
    // Uncomment to close window without asking.
    //closeBtnHandler.cfgCloseImmediatelly = true;
    ASTokenizer tz;
    tokens = tz.Tokenize(sample);
}

// `frameStep()` runs every frame; `dt` is delta time in seconds.
void frameStep(float dt)
{
    // Begin drawing window
    if (ImGui::Begin("Lexer script", closeBtnHandler.windowOpen, 0))
    {
        // Draw the "Terminate this script?" prompt on the top (if not disabled by config).
        closeBtnHandler.draw();
        
        ImGui::Text(sample);
        ImGui::Separator();
        
        DrawTokens(tokens);
        
        // End drawing window
        ImGui::End();
    }
}

//#endregion
