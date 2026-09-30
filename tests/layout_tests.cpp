#include "../src/menu/esp_layout.h"
#include "../src/menu/config_json.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <random>

using namespace EspLayout;
static int checks = 0;
static int layouts = 0;
static int drags = 0;
static void Require(bool ok, const char* text) { ++checks; if (!ok) { std::fprintf(stderr, "FAIL: %s (check %d, layouts %d, drags %d)\n", text, checks, layouts, drags); std::exit(1); } }
static bool Near(float a, float b) { return std::fabs(a - b) < 0.03f; }
static ImVec2 Center(Rect r) { return ImVec2((r.min.x + r.max.x) * 0.5f, (r.min.y + r.max.y) * 0.5f); }
static bool Same(Rect a, Rect b) { return Near(a.min.x,b.min.x) && Near(a.min.y,b.min.y) && Near(a.max.x,b.max.x) && Near(a.max.y,b.max.y); }
static Style style;
static Content content;
static Rect viewport = { ImVec2(0,0), ImVec2(1200,900) };
static Rect box = { ImVec2(540,340), ImVec2(660,540) };

static void Valid(const Result& result, Rect bounds, float gap) {
    ++layouts;
    Require(result.valid, "valid layout");
    Require(CheckGeometry(result,gap), "reserved box and items do not intersect");
    for (const auto& item : result.items) if (item.visible) Require(bounds.Contains(item.rect), "visible item fits viewport");
}

static void Geometry() {
    for (int sides = 0; sides < 256; ++sides) for (int aligns = 0; aligns < 81; ++aligns) for (float h : {30.0f,112.0f,220.0f}) {
        Model model = Preset(1);
        int s = sides, a = aligns;
        for (auto& item : model.items) { item.side = s%4; s/=4; item.position = (a%3)*0.5f; a/=3; }
        Rect b = { ImVec2(600-h*0.3f,450-h*0.5f), ImVec2(600+h*0.3f,450+h*0.5f) };
        Valid(Resolve(model,b,viewport,content,style),viewport,model.gap);
    }
    std::mt19937 random(20260912);
    for (int sample = 0; sample < 12000; ++sample) {
        Model model = Preset(1);
        for (auto& item : model.items) { item.side=random()%4; item.position=(random()%1001)/1000.0f; item.extraGap=random()%13; item.order=random()%4; item.enabled=random()%7!=0; }
        model.stack=random()%2; model.horizontalText=random()%2; model.gap=2+random()%11; model.spacing=2+random()%7;
        model.barLength=0.25f+(random()%76)/100.0f; model.barThickness=2+random()%5;
        Content text = content;
        if (sample%3==0) text.text[Name]="Zumbi_com_nome_muito_longo_acentuado_ação_0123456789";
        float h=10+random()%291;
        Rect b = { ImVec2(600-h*0.3f,450-h*0.5f), ImVec2(600+h*0.3f,450+h*0.5f) };
        Valid(Resolve(model,b,viewport,text,style),viewport,model.gap);
    }
    for (int preset=1;preset<=10;++preset) for (float h : {32.0f,112.0f,180.0f}) {
        Rect v = { ImVec2(5,5),ImVec2(415,395) };
        Rect b = { ImVec2(210-h*0.3f,200-h*0.5f),ImVec2(210+h*0.3f,200+h*0.5f) };
        Valid(Resolve(Preset(preset),b,v,content,style),v,4);
    }
    Model model=Preset(1);
    auto before=Resolve(model,box,viewport,content,style);
    for (float hp : {0.0f,0.01f,0.87f,1.0f}) {
        Content text=content; text.health=hp;
        auto result=Resolve(model,box,viewport,text,style);
        Require(Same(before.items[Health].rect,result.items[Health].rect),"HP fill never changes track/hitbox");
        Require(HitTest(result,Center(result.items[Health].rect),Name)==Health,"health selectable even at zero");
    }
    model.items[Health].enabled=false;
    auto independent=Resolve(model,box,viewport,content,style);
    Require(!independent.items[Health].visible && independent.items[Percent].visible,"percentage visibility independent from bar");
    Content unavailable=content; unavailable.healthAvailable=false;
    auto missing=Resolve(Preset(1),box,viewport,unavailable,style);
    Require(!missing.items[Health].visible && !missing.items[Percent].visible && missing.items[Name].visible,"missing HP is distinct from zero");
    model=Preset(1); model.gap=std::numeric_limits<float>::quiet_NaN(); model.items[Name].position=std::numeric_limits<float>::infinity();
    Valid(Resolve(model,box,viewport,content,style),viewport,4);
    Require(Near(Snap(-2.5f,5),-5) && Near(Snap(2.5f,5),5),"grid rounds negative and positive values symmetrically");
}

static ImVec2 Destination(Rect b,int side) {
    ImVec2 p=Center(b);
    if(side==Top)p.y=b.min.y-30; if(side==Bottom)p.y=b.max.y+30;
    if(side==Left)p.x=b.min.x-60; if(side==Right)p.x=b.max.x+60;
    return p;
}

static Result Drag(Editor& editor, int id, int side, Rect b=box, Rect v=viewport) {
    auto before=Resolve(editor.draft,b,v,content,style);
    Valid(before,v,editor.draft.gap);
    ImVec2 press=Center(before.items[id].rect);
    Update(editor,b,v,content,style,{press,true,true,false,false,true});
    Require(editor.active==id,"one owner captures clicked item");
    auto target=Destination(b,side);
    auto candidate=Update(editor,b,v,content,style,{target,false,true,false,false,true});
    Require(editor.candidateValid,"side candidate accepted");
    Require(editor.candidate.items[id].side==side,"candidate chooses intended side");
    auto result=Update(editor,b,v,content,style,{target,false,false,true,false,true});
    Require(editor.active==-1 && editor.draft.items[id].side==side,"release commits and releases capture");
    for(int i=0;i<Count;++i)Require(Same(candidate.items[i].rect,result.items[i].rect),"release never jumps from candidate");
    Valid(result,v,editor.draft.gap); ++drags;
    return result;
}

static void Dragging() {
    for(int id=0;id<Count;++id) for(int from=0;from<4;++from) for(int to=0;to<4;++to) {
        if(from==to)continue;
        Model model=Preset(1); model.items[id].side=from;
        Editor editor; Reset(editor,model); Drag(editor,id,to);
    }
    for(int id=0;id<Count;++id) {
        Editor editor;Reset(editor,Preset(1));
        for(int cycle=0;cycle<30;++cycle) for(int side : {Right,Left,Top,Bottom}) Drag(editor,id,side);
    }
    for(int id=0;id<Count;++id) for(int side=0;side<4;++side) {
        Editor first,shifted;Reset(first,Preset(1));Reset(shifted,Preset(1));
        auto a=Drag(first,id,side);
        ImVec2 translation(831,472);
        Rect b={{box.min.x+translation.x,box.min.y+translation.y},{box.max.x+translation.x,box.max.y+translation.y}};
        Rect v={{viewport.min.x+translation.x,viewport.min.y+translation.y},{viewport.max.x+translation.x,viewport.max.y+translation.y}};
        auto r=Drag(shifted,id,side,b,v);
        for(int i=0;i<Count;++i){Rect normalized={{r.items[i].rect.min.x-translation.x,r.items[i].rect.min.y-translation.y},{r.items[i].rect.max.x-translation.x,r.items[i].rect.max.y-translation.y}};Require(Same(a.items[i].rect,normalized),"window translation cannot change box-local geometry");}
    }
    for(int mode=0;mode<6;++mode) {
        Editor editor; Reset(editor,Preset(1));editor.dirty=true;
        auto before=Resolve(editor.draft,box,viewport,content,style);
        auto press=Center(before.items[Name].rect);
        Update(editor,box,viewport,content,style,{press,true,true,false,false,true});
        Update(editor,box,viewport,content,style,{Destination(box,Right),false,true,false,false,true});
        if(mode<2){ImVec2 invalid=mode==0?Center(box):ImVec2(-200,-200);Update(editor,box,viewport,content,style,{invalid,false,true,false,false,true});Require(!editor.candidateValid,"invalid destination rejected");Update(editor,box,viewport,content,style,{invalid,false,false,true,false,true});}
        else if(mode==2)Update(editor,box,viewport,content,style,{press,false,true,false,true,true});
        else if(mode==3){Rect resized=box;resized.max.x+=20;Update(editor,resized,viewport,content,style,{press,false,true,false,false,true});}
        else if(mode==4){Rect moved=viewport;moved.min.x+=10;Update(editor,box,moved,content,style,{press,false,true,false,false,true});}
        else CancelDrag(editor);
        Require(editor.active==-1 && editor.dirty && editor.draft.items[Name].side==Top,"invalid drop/cancel/focus/resize restores draft and dirty state");
    }
    for(int side : {Top,Bottom}) for(float alignment : {0.0f,1.0f}) {
        Model model=Preset(1);model.stack=false;model.grid=false;model.items[Name].side=side;
        Rect small={{550,430},{568,460}};
        Editor editor;Reset(editor,model);auto before=Resolve(model,small,viewport,content,style);
        Update(editor,small,viewport,content,style,{Center(before.items[Name].rect),true,true,false,false,true});
        float width=before.items[Name].rect.Width();ImVec2 target(small.min.x+(small.Width()-width)*alignment+width*0.5f,side==Top?small.min.y-30:small.max.y+30);
        Update(editor,small,viewport,content,style,{target,false,true,false,false,true});
        Update(editor,small,viewport,content,style,{target,false,false,true,false,true});
        Require(Near(editor.draft.items[Name].position,alignment),"text wider than small box still reaches both corner alignments");
    }
}

static void Edges() {
    Rect v={{0,0},{360,280}};
    for(int p=1;p<=10;++p) for(ImVec2 c : {ImVec2(8,10),ImVec2(350,10),ImVec2(8,270),ImVec2(350,270),ImVec2(180,140)}) {
        Model model=Preset(p);
        Rect b={{c.x-12,c.y-20},{c.x+12,c.y+20}};
        auto r=Resolve(model,b,v,content,style,true);
        Valid(r,v,model.gap);
        Require(model.preset==p,"screen-edge fallback does not mutate saved model");
        if(c.x==180)Require(r.hidden==0 && !r.relocated,"ordinary centered entity preserves all items and sides");
    }
}

static void Json() {
    using namespace ConfigJson;
    Document document;
    bool enabled=false;int side=2;float gap=4,color[4]={0,0,0,1};char name[64]="initial";
    std::vector<Field> fields={{"enabled",BoolField,&enabled},{"side",IntField,&side},{"gap",FloatField,&gap},{"color",ColorField,color},{"name",StringField,name,sizeof(name)}};
    Require(document.Parse("{\n \"_v\" : 4, \"enabled\" : true, \"side\":3,\"gap\":2.5e0,\"color\":[0,0.5,1,1],\"name\":\"a\\\"b\\\\c\\n\\u00e7\\ud83d\\ude00\" }"),"JSON whitespace escapes Unicode exponent");
    Require(document.Version()==4 && document.Apply(fields) && enabled && side==3 && gap==2.5f && color[1]==0.5f,"typed settings applied");
    std::string expected=name;
    Require(document.Parse("{\"name\":\""+Escape(name)+"\"}") && document.Apply(fields) && expected==name,"escaped search text round-trip");
    for(std::string invalid : {"{\"enabled\":false,\"side\":\"3\"}","{\"enabled\":false,\"gap\":1e39}","{\"enabled\":false,\"side\":1.5}","{\"enabled\":false,\"color\":[1,2,3,4]}","{\"enabled\":false,\"color\":[0,0,0]}","{\"enabled\":false,\"name\":null}"}) {
        Require(document.Parse(invalid) && !document.Apply(fields) && enabled && side==3,"bad field causes atomic rejection");
    }
    for(std::string invalid : {"{\"x\":nan}","{\"x\":1e999}","{\"x\":01}","{\"x\":true,}","{\"x\":1,\"x\":2}","{\"x\":\"\\ud800\"}","{\"x\":\"\\u0000\"}","[]","{}garbage"})Require(!document.Parse(invalid),"malformed JSON rejected");
    Require(document.Parse("{\"_v\":4,\"iCfgVer\":2}") && document.Version()==4,"schema uses authoritative _v despite stale iCfgVer");
    Require(document.Parse("{}") && document.Version()==0,"missing version is legacy");
    Require(document.Parse("{\"_v\":4.1}") && document.Version()==-1,"fractional schema rejected");
}

int main() {
    ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.Fonts->AddFontDefault();io.Fonts->Build();style.font=io.Fonts->Fonts[0];
    content.text[Name]="Zombie";content.text[Distance]="34m";content.text[Percent]="87%";
    Geometry();Dragging();Edges();Json();
    std::printf("PASS layout: %d resolved scenarios; %d complete drag transitions; %d assertions\n",layouts,drags,checks);
    ImGui::DestroyContext();
}
