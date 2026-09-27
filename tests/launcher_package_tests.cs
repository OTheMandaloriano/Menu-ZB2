using System;
using System.IO;
using System.IO.Compression;
using System.Collections.Generic;
using System.Text;
using System.Web.Script.Serialization;
using Zb2Launcher;
class PackageTests {
    static int checks;
    static void Check(bool b,string name){if(!b)throw new Exception(name);++checks;}
    static byte[] Package(string version,string evil=null,bool corrupt=false){
        var files=new Dictionary<string,string>();
        foreach(var name in PackageStore.Required)files[name]=PackageStore.Hash(Encoding.UTF8.GetBytes(name));
        var manifest=new PackageManifest{schema=1,version=version,game_sha256=new string('a',64),files=files};
        using(var memory=new MemoryStream()) {
            using(var zip=new ZipArchive(memory,ZipArchiveMode.Create,true)) {
                foreach(var name in PackageStore.Required)using(var output=new StreamWriter(zip.CreateEntry(evil!=null && name=="injector.exe"?evil:name).Open()))output.Write(corrupt && name=="injector.exe"?"corrupt":name);
                using(var output=new StreamWriter(zip.CreateEntry("manifest.json").Open()))output.Write(new JavaScriptSerializer().Serialize(manifest));
            }
            return memory.ToArray();
        }
    }
    static void Reject(Action action,string name){bool rejected=false;try{action();}catch(InvalidDataException){rejected=true;}catch(InvalidOperationException){rejected=true;}Check(rejected,name);}
    static void Main(string[] args){
        string root=Path.GetFullPath(args[0]);Directory.CreateDirectory(root);
        bool running=false;var store=new PackageStore(root,()=>running);
        Reject(()=>store.Install(Package("1.0.0","../escape.exe")),"reject traversal");Check(!File.Exists(Path.Combine(root,"escape.exe")),"no path escape");
        Reject(()=>store.Install(Package("1.0.0",null,true)),"reject corrupt payload");
        Reject(()=>store.Install(Package("1.0.0"),new string('0',64)),"reject download digest mismatch");
        running=true;Reject(()=>store.Install(Package("1.0.0")),"no update while game running");running=false;
        var first=store.Install(Package("1.0.0"));Check(first.version=="1.0.0" && store.Current().version=="1.0.0","install initial package");
        Check(store.Inspect(Package("1.1.0")).version=="1.1.0","embedded package version available offline");
        Directory.CreateDirectory(Path.Combine(root,"configs"));File.WriteAllText(Path.Combine(root,"configs","user.json"),"keep");
        store.Install(Package("1.1.0"));Check(store.Current().version=="1.1.0","atomic version activation");
        Check(File.ReadAllText(Path.Combine(root,"previous.txt"))=="1.0.0","previous version retained");
        Check(File.ReadAllText(Path.Combine(root,"configs","user.json"))=="keep","presets preserved");
        Reject(()=>store.Install(Package("1.0.0")),"automatic downgrade rejected");
        store.Install(Package("1.1.0"));Check(store.Current().version=="1.1.0","repeat package idempotent");
        File.WriteAllText(Path.Combine(store.VersionDirectory("1.1.0"),"injector.exe"),"changed");Reject(()=>store.Current(),"verify files before execution");
        Reject(()=>PackageStore.ParseVersion("../outside"),"reject invalid version directory");
        Console.WriteLine(checks+" launcher package checks passed");
    }
}
