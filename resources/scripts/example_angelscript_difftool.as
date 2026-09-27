// DIFF TEST
// ===================================================

// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler;

#include "char_utils.as"

//#region game glue

void main()
{
    // Uncomment to close window without asking.
    //closeBtnHandler.cfgCloseImmediatelly = true;
}

// `frameStep()` runs every frame; `dt` is delta time in seconds.
void frameStep(float dt)
{
    // Begin drawing window
    if (ImGui::Begin("Diff test", closeBtnHandler.windowOpen, 0))
    {
        // Draw the "Terminate this script?" prompt on the top (if not disabled by config).
        closeBtnHandler.draw();
        
        drawDiff(diffLines);
        
        // End drawing window
        ImGui::End();
    }
}

//#endregion (game glue)

//#region Test data



// TEST CODE (using 'heredoc' syntax; see https://www.angelcode.com/angelscript/sdk/docs/manual/doc_datatypes_strings.html)
const string TEST_CODE_A =
"""
// \brief Parsing test code

string msg = "Hello"
+ "Rigs of Rods!";

game.log( msg);
// Begin drawing window
if (ImGui::Begin("Diff test", closeBtnHandler.windowOpen, 0))
{
    // Draw the "Terminate this script?" prompt on the top (if not disabled by config).
    closeBtnHandler.draw();
    
    drawDiff(diffLines);
    
    // End drawing window
    ImGui::End();
}
""";
const string TEST_CODE_B =
"""
// \brief Parsing test code

string msg = "Hello"
+ " World!";

game.log( msg);

// Begin drawing window
if (ImGui::Begin("Diff test", closeBtnHandler.windowOpen, 0))
{
    // Draw the "Terminate this script?" prompt on the top (if not disabled by config).
    closeBtnHandler.draw();
    
    drawBiff(diffLines);
    
    // End drawing window
    ImGui::End();
}
""";

//#endregion

//#region Data preprocessing

array<string> splitLines(const string &in text)
{
    
    
    array<string> lines;
    string line;
    uint c;
    
    for (uint i = 0; i < text.length(); i++)
    {
        c = text[i];
        
        if (c == char_utils::CHAR_NEWLINE)
        {
            lines.insertLast(line);
            line = "";
        }
        else if (c != char_utils::CHAR_CARRIAGERETURN)
        {
            line += text.substr(i, 1);
        }
    }
    
    lines.insertLast(line);
    return lines;
}

array<string> aLines = splitLines(TEST_CODE_A);
array<string> bLines = splitLines(TEST_CODE_B);



//#endregion 

// #region LCS algorithm
// The Longest Common Subsequence (LCS) algorithm is the standard backbone of diff tools. It’s easy to implement and good enough for text files.

enum DiffType { Same, Added, Removed };

class DiffLine
{
    DiffType type;
    string text;
}

int maxInt(int a, int b)
{
    return (a > b) ? a : b;
}

array<array<int>> buildLCSTable(const array<string> &in A, const array<string> &in B)
{
    uint n = A.length();
    uint m = B.length();
    
    array<array<int>> dp(n + 1);
    for (uint i = 0; i <= n; i++)
    dp[i].resize(m + 1);
    
    for (uint i = 1; i <= n; i++)
    {
        for (uint j = 1; j <= m; j++)
        {
            if (A[i - 1] == B[j - 1])
            dp[i][j] = dp[i - 1][j - 1] + 1;
            else
            dp[i][j] = maxInt(dp[i - 1][j], dp[i][j - 1]);
        }
    }
    return dp;
}

// Backtrack to produce diff
array<DiffLine> buildDiff(
const array<string> &in A,
const array<string> &in B)
{
    array<array<int>> dp = buildLCSTable(A, B);
    array<DiffLine> result;
    
    int i = int(A.length());
    int j = int(B.length());
    
    while (i > 0 || j > 0)
    {
        if (i > 0 && j > 0 && A[i - 1] == B[j - 1])
        {
            DiffLine d;
            d.type = Same;
            d.text = A[i - 1];
            result.insertLast(d);
            i--; j--;
        }
        else if (j > 0 && (i == 0 || dp[i][j - 1] >= dp[i - 1][j]))
        {
            DiffLine d;
            d.type = Added;
            d.text = B[j - 1];
            result.insertLast(d);
            j--;
        }
        else
        {
            DiffLine d;
            d.type = Removed;
            d.text = A[i - 1];
            result.insertLast(d);
            i--;
        }
    }
    
    result.reverse();
    return result;
}

array<DiffLine> diffLines = buildDiff(aLines, bLines);

// #endregion

// #region Rendering

color COLOR_SAME (1, 1, 1, 1);
color COLOR_ADDED (0.6f, 1.0f, 0.6f, 1);
color COLOR_REMOVED (1.0f, 0.6f, 0.6f, 1);

color COLOR_LINENUM_SAME    (0.6f, 0.6f, 0.6f, 1);
color COLOR_LINENUM_ADDED   (0.4f, 0.9f, 0.4f, 1);
color COLOR_LINENUM_REMOVED (0.9f, 0.4f, 0.4f, 1);


class SimpleClipper
{
    int displayStart;
    int displayEnd;
}

SimpleClipper computeClipper(int itemCount, float itemHeight)
{
    SimpleClipper c;
    
    float scrollY     = ImGui::GetScrollY();
    float windowH     = ImGui::GetWindowHeight();
    
    int first = int(scrollY / itemHeight);
    int last  = int((scrollY + windowH) / itemHeight) + 1;
    
    if (first < 0) first = 0;
    if (last > itemCount) last = itemCount;
    
    c.displayStart = first;
    c.displayEnd   = last;
    return c;
}



void drawDiff(const array<DiffLine> &in diff)
{
    ImGui::BeginChild("DiffView");
    
    float lineHeight = ImGui::GetTextLineHeight();
    float numberWidth = 60.0f;
    
    ImGui::Dummy(vector2(0, diff.length() * lineHeight));
    
    SimpleClipper clip = computeClipper(diff.length(), lineHeight);
    
    int lineA = 1;
    int lineB = 1;
    
    for (int i = 0; i < clip.displayStart; i++)
    {
        if (diff[i].type != Added)   lineA++;
        if (diff[i].type != Removed) lineB++;
    }
    
    ImGui::SetCursorPosY(clip.displayStart * lineHeight);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, vector2(0,0));
    
    for (int i = clip.displayStart; i < clip.displayEnd; i++)
    {
        const DiffLine@ d = diff[i];
        ImGui::PushID(i);
        
        // --- Line numbers ---
        ImGui::BeginGroup();
        
        if (d.type == Added)
        ImGui::PushStyleColor(ImGuiCol_Text, COLOR_LINENUM_ADDED);
        else if (d.type == Removed)
        ImGui::PushStyleColor(ImGuiCol_Text, COLOR_LINENUM_REMOVED);
        else
        ImGui::PushStyleColor(ImGuiCol_Text, COLOR_LINENUM_SAME);
        
        if (d.type != Added)
        ImGui::Text("" + lineA);
        else
        {
            vector2 digitSize = ImGui::CalcTextSize(""+lineA);
            ImGui::Dummy(digitSize);
        }
        
        ImGui::PopStyleColor();
        ImGui::SameLine(0, 8);
        
        if (d.type == Added)
        ImGui::PushStyleColor(ImGuiCol_Text, COLOR_LINENUM_ADDED);
        else if (d.type == Removed)
        ImGui::PushStyleColor(ImGuiCol_Text, COLOR_LINENUM_REMOVED);
        else
        ImGui::PushStyleColor(ImGuiCol_Text, COLOR_LINENUM_SAME);
        
        if (d.type != Removed)
        ImGui::Text("" + lineB);
        else
        {
            vector2 digitSize = ImGui::CalcTextSize(""+lineB);
            ImGui::Dummy(digitSize);
        }
        
        ImGui::PopStyleColor();
        ImGui::EndGroup();
        
        // --- Text ---
        ImGui::SameLine(numberWidth);
        
        if (d.type == Added)
        ImGui::PushStyleColor(ImGuiCol_Text, COLOR_ADDED);
        else if (d.type == Removed)
        ImGui::PushStyleColor(ImGuiCol_Text, COLOR_REMOVED);
        else
        ImGui::PushStyleColor(ImGuiCol_Text, COLOR_SAME);
        
        string prefix = "  ";
        if (d.type == Added)   prefix = "+ ";
        if (d.type == Removed) prefix = "- ";
        
        ImGui::Text(prefix + d.text);
        ImGui::PopStyleColor();
        
        if (d.type != Added)   lineA++;
        if (d.type != Removed) lineB++;
        
        ImGui::PopID();
    }
    
    ImGui::PopStyleVar(); // ItemSpacing
    
    ImGui::EndChild();
}




// #endregion
