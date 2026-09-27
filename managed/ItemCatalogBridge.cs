using System;
using System.Runtime.InteropServices;
using System.Text;
using UnityEngine;

namespace Zb2Menu {
    public static class ItemCatalogBridge {
        [StructLayout(LayoutKind.Sequential,Pack=4)]
        public struct Entry {public int Id;[MarshalAs(UnmanagedType.ByValArray,SizeConst=96)]public byte[] Name;}
        public static int Read(IntPtr buffer,int capacity) {
            if(buffer==IntPtr.Zero || capacity<1 || ItemsBase.instance==null)return 0;
            int count=0,stride=Marshal.SizeOf(typeof(Entry));
            foreach(var item in ItemsBase.instance.Items) {
                if(!ItemEligibility.Allowed(item) || count>=capacity)continue;
                string name=item.GetName;
                if(string.IsNullOrWhiteSpace(name) || name=="<?>")name=item.itemID.ToString();
                if(name.Length>28)name=name.Substring(0,28);
                var entry=new Entry{Id=(int)item.itemID,Name=new byte[96]};
                var encoded=Encoding.UTF8.GetBytes(name);Array.Copy(encoded,entry.Name,Math.Min(encoded.Length,95));
                Marshal.StructureToPtr(entry,IntPtr.Add(buffer,count++*stride),false);
            }
            return count;
        }
        public static bool Icon(int id,IntPtr buffer) {
            if(buffer==IntPtr.Zero || ItemsBase.instance==null || id<=0 || id>=ItemsBase.ItemCount)return false;
            var item=ItemsBase.instance.GetItem((InventoryItem.ID)id);
            if(!ItemEligibility.Allowed(item) || item.sprite==null)return false;
            var sprite=item.sprite;var texture=sprite.texture;if(texture==null)return false;
            RenderTexture temporary=null;Texture2D readback=null;var previous=RenderTexture.active;
            try {
                var rect=sprite.textureRect;
                if(!(rect.width>0 && rect.height>0))return false;
                float scale=Math.Min(48/rect.width,48/rect.height);
                int width=Math.Max(1,(int)(rect.width*scale)),height=Math.Max(1,(int)(rect.height*scale));
                temporary=RenderTexture.GetTemporary(width,height,0,RenderTextureFormat.ARGB32);
                Graphics.Blit(texture,temporary,new Vector2(rect.width/texture.width,rect.height/texture.height),new Vector2(rect.x/texture.width,rect.y/texture.height));
                RenderTexture.active=temporary;
                readback=new Texture2D(width,height,TextureFormat.RGBA32,false);
                readback.ReadPixels(new Rect(0,0,width,height),0,0,false);readback.Apply(false,false);
                var colors=readback.GetPixels32();var bytes=new byte[48*48*4];
                int padX=(48-width)/2,padY=(48-height)/2;
                for(int y=0;y<height;++y)for(int x=0;x<width;++x){var c=colors[(height-1-y)*width+x];int offset=((y+padY)*48+x+padX)*4;bytes[offset]=c.r;bytes[offset+1]=c.g;bytes[offset+2]=c.b;bytes[offset+3]=c.a;}
                Marshal.Copy(bytes,0,buffer,bytes.Length);return true;
            } catch {return false;}
            finally {RenderTexture.active=previous;if(temporary!=null)RenderTexture.ReleaseTemporary(temporary);if(readback!=null)UnityEngine.Object.Destroy(readback);}
        }
    }
}
