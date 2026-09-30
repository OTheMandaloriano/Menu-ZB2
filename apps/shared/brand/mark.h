#pragma once
// Original DEADBLOCK mark; coordinates shared with deadblock-symbol.svg.
#include "../theme.h"
namespace Deadblock {
inline void Draw(ImDrawList* draw,float x,float y,float size){
    const float s=size/32.f;ImVec2 points[4];
    points[0]=ImVec2(x+2*s,y+2*s);
    points[1]=ImVec2(x+20*s,y+2*s);
    points[2]=ImVec2(x+26*s,y+8*s);
    points[3]=ImVec2(x+2*s,y+8*s);
    draw->AddConvexPolyFilled(points,4,IM_COL32(243,244,246,255));
    points[0]=ImVec2(x+2*s,y+8*s);
    points[1]=ImVec2(x+8*s,y+8*s);
    points[2]=ImVec2(x+8*s,y+30*s);
    points[3]=ImVec2(x+2*s,y+30*s);
    draw->AddConvexPolyFilled(points,4,IM_COL32(243,244,246,255));
    points[0]=ImVec2(x+8*s,y+24*s);
    points[1]=ImVec2(x+24*s,y+24*s);
    points[2]=ImVec2(x+24*s,y+30*s);
    points[3]=ImVec2(x+8*s,y+30*s);
    draw->AddConvexPolyFilled(points,4,IM_COL32(243,244,246,255));
    points[0]=ImVec2(x+24*s,y+6*s);
    points[1]=ImVec2(x+30*s,y+12*s);
    points[2]=ImVec2(x+30*s,y+24*s);
    points[3]=ImVec2(x+24*s,y+30*s);
    draw->AddConvexPolyFilled(points,4,IM_COL32(37,99,235,255));
    points[0]=ImVec2(x+11*s,y+19*s);
    points[1]=ImVec2(x+21*s,y+9*s);
    points[2]=ImVec2(x+21*s,y+17*s);
    points[3]=ImVec2(x+11*s,y+27*s);
    draw->AddConvexPolyFilled(points,4,IM_COL32(37,99,235,255));
}
}
