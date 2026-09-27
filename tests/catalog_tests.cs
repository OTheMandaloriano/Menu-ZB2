using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using Zb2Menu;
namespace UnityEngine {
    public class Object {public static int destroyed;public static void Destroy(object item){destroyed++;}}
    public struct Vector2 {public float x,y;public Vector2(float a,float b){x=a;y=b;}}
    public struct Rect {public float x,y,width,height;public Rect(float a,float b,float c,float d){x=a;y=b;width=c;height=d;}}
    public struct Color32 {public byte r,g,b,a;}
    public enum RenderTextureFormat {ARGB32}public enum TextureFormat {RGBA32}
    public class Texture2D {public int width=256,height=256;public Texture2D(int w,int h,TextureFormat f,bool m){width=w;height=h;}public void ReadPixels(Rect r,int x,int y,bool recalc){}public void Apply(bool a,bool b){}public Color32[] GetPixels32(){var result=new Color32[width*height];for(int y=0;y<height;++y)for(int x=0;x<width;++x)result[y*width+x]=new Color32{r=(byte)y,a=255};return result;}}
    public class Sprite {public Texture2D texture=new Texture2D(256,256,TextureFormat.RGBA32,false);public Rect textureRect=new Rect(64,32,128,64);}
    public class RenderTexture {public static RenderTexture active;public static int released;public static RenderTexture GetTemporary(int w,int h,int depth,RenderTextureFormat f){return new RenderTexture();}public static void ReleaseTemporary(RenderTexture r){released++;}}
    public static class Graphics {public static Vector2 Scale,Offset;public static void Blit(Texture2D source,RenderTexture target,Vector2 scale,Vector2 offset){Scale=scale;Offset=offset;}}
}
public class InventoryItem {public enum ID {None,Wood,PistolAmmo}}
public class DatabaseItem {public InventoryItem.ID itemID;public string GetName;public UnityEngine.Sprite sprite;}
public class ItemsBase {public static ItemsBase instance;public List<DatabaseItem> Items=new List<DatabaseItem>();public static float ItemCount {get{return instance.Items.Count;}}public DatabaseItem GetItem(InventoryItem.ID id){return Items[(int)id];}}
class CatalogTests {
    static int checks;static void Check(bool b,string n){if(!b)throw new Exception(n);checks++;}
    static void Main(){
        IntPtr buffer=Marshal.AllocHGlobal(48*48*4+8);
        try {
            Check(ItemCatalogBridge.Read(buffer,128)==0,"unavailable catalog empty");
            ItemsBase.instance=new ItemsBase();ItemsBase.instance.Items.Add(new DatabaseItem());ItemsBase.instance.Items.Add(new DatabaseItem{itemID=InventoryItem.ID.Wood,GetName="Madeira",sprite=new UnityEngine.Sprite()});ItemsBase.instance.Items.Add(new DatabaseItem{itemID=InventoryItem.ID.PistolAmmo,GetName="Munição de pistola"});
            Check(ItemCatalogBridge.Read(buffer,1)==1,"catalog bounded");Check(Marshal.ReadInt32(buffer)==1,"database id retained");byte[] name=new byte[96];Marshal.Copy(IntPtr.Add(buffer,4),name,0,96);Check(System.Text.Encoding.UTF8.GetString(name).StartsWith("Madeira"),"localized name rather than enum");
            var active=new UnityEngine.RenderTexture();UnityEngine.RenderTexture.active=active;Marshal.WriteInt32(buffer,48*48*4,9123);
            Check(ItemCatalogBridge.Icon(1,buffer),"sprite icon converted");Check(Marshal.ReadByte(buffer,12*48*4)==23 && Marshal.ReadByte(buffer,3)==0,"orientation and aspect ratio preserved with transparent padding");Check(Marshal.ReadInt32(buffer,48*48*4)==9123,"icon buffer bounded");
            Check(ReferenceEquals(active,UnityEngine.RenderTexture.active) && UnityEngine.RenderTexture.released==1 && UnityEngine.Object.destroyed==1,"render target restored and temporary resources released");
            Check(UnityEngine.Graphics.Scale.x==.5f && UnityEngine.Graphics.Offset.x==.25f,"sprite atlas cropped");Check(!ItemCatalogBridge.Icon(2,buffer),"missing image returns no fabricated icon");
            Console.WriteLine(checks+" catalog and icon checks passed");
        }finally{Marshal.FreeHGlobal(buffer);}
    }
}
