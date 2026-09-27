/// \title MATH UTILS
/// \brief min()/max()/abs() and the sort...
/// Cleans up old copypasta between scripts:
/// * imin(), imax(), iabs()
/// * fmin(), fmax(), fabs(), fclamp()
/// * getMouseShortestDistance()
/// * setAxisVal() // vector3 helper
/// * formatVector3()
// ===================================================

// By convention, all includes have filename '*_utils' and namespace matching filename.
namespace math_utils
{
    
float fmax(float a, float b) { return a>b?a:b; }
    
float fmin(float a, float b) { return a<b?a:b; }
    
float fclamp(float val, float vmin, float vmax) { return (val > vmax) ? vmax : ((val < vmin) ? vmin : val); }
    
    void setAxisVal(vector3&inout vec, int axis, float val)
    {
        switch(axis) {
            case 0: vec.x=val; break;
            case 1: vec.y=val; break;
            case 2: vec.z=val; break;
            default: break;
        }
    }
    
    int getMouseShortestDistance(vector2 mouse, vector2 target)
    {
        int dx = iabs(int(mouse.x) - int(target.x));
        int dy = iabs(int(mouse.y) - int(target.y));
        return imax(dx, dy);
    }
    
    int imin(int a, int b)
    {
        return (a < b) ? a : b;
    }
    
    int imax(int a, int b)
    {
        return (a > b) ? a : b;
    }
    
    int iabs(int a)
    {
        return (a < 0) ? -a : a;
    }
    
    string formatVector3(vector3 val, int total, int frac)
    {
        return "X:" + formatFloat(val.x, "", total, frac)
        + " Y:" + formatFloat(val.y, "", total, frac)
        + " Z:" + formatFloat(val.z, "", total, frac);
    }
    
} // namespace math_utils
