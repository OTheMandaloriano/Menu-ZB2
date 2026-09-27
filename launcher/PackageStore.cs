using System;
using System.IO;
using System.IO.Compression;
using System.Collections.Generic;
using System.Linq;
using System.Security.Cryptography;
using System.Text.RegularExpressions;
using System.Web.Script.Serialization;

namespace Zb2Launcher {
    public sealed class PackageManifest {
        public int schema;
        public string version;
        public string game_sha256;
        public Dictionary<string,string> files;
    }
    public sealed class PackageStore {
        public static readonly string[] Required={"injector.exe","config.ini","kiero-dx11-base.dll","Zb2.AimBridge.dll","0Harmony.dll","Harmony.LICENSE"};
        public readonly string Root;
        readonly Func<bool> gameRunning;
        readonly JavaScriptSerializer json=new JavaScriptSerializer();
        public PackageStore(string root,Func<bool> running) {Root=Path.GetFullPath(root);gameRunning=running;Directory.CreateDirectory(Root);}
        public static string Hash(byte[] bytes) {using(var hash=SHA256.Create())return BitConverter.ToString(hash.ComputeHash(bytes)).Replace("-","").ToLowerInvariant();}
        public static Version ParseVersion(string text) {
            Version version;
            if(text==null || !Regex.IsMatch(text,@"^\d+\.\d+\.\d+$") || !Version.TryParse(text,out version))throw new InvalidDataException("Versao de pacote invalida");
            return version;
        }
        public PackageManifest Current() {
            string pointer=Path.Combine(Root,"current.txt");
            if(!File.Exists(pointer))return null;
            string version=File.ReadAllText(pointer).Trim();ParseVersion(version);
            string dir=VersionDirectory(version);
            var manifest=ReadManifest(File.ReadAllText(Path.Combine(dir,"manifest.json")));
            if(manifest.version!=version)throw new InvalidDataException("Versao local inconsistente");
            foreach(var entry in manifest.files)
                if(Hash(File.ReadAllBytes(Path.Combine(dir,entry.Key)))!=entry.Value)throw new InvalidDataException("Arquivo local alterado: "+entry.Key);
            return manifest;
        }
        public string VersionDirectory(string version) {ParseVersion(version);return Path.Combine(Root,"runtime",version);}
        public PackageManifest Inspect(byte[] package) {
            using(var zip=new ZipArchive(new MemoryStream(package),ZipArchiveMode.Read)) {
                var entry=zip.GetEntry("manifest.json");
                if(entry==null || entry.Length>32768)throw new InvalidDataException("Manifesto ausente ou excessivo");
                using(var stream=entry.Open())using(var reader=new StreamReader(stream))return ReadManifest(reader.ReadToEnd());
            }
        }
        PackageManifest ReadManifest(string value) {
            if(value.Length>32768)throw new InvalidDataException("Manifesto excessivo");
            var m=json.Deserialize<PackageManifest>(value);
            if(m==null || m.schema!=1 || m.files==null || m.files.Count!=Required.Length)throw new InvalidDataException("Manifesto incompativel");
            ParseVersion(m.version);
            if(!Regex.IsMatch(m.game_sha256??"",@"^[a-f0-9]{64}$"))throw new InvalidDataException("Hash de jogo invalido");
            foreach(string name in Required)
                if(!m.files.ContainsKey(name) || !Regex.IsMatch(m.files[name],@"^[a-f0-9]{64}$"))throw new InvalidDataException("Arquivo obrigatorio invalido: "+name);
            return m;
        }
        public PackageManifest Install(byte[] package,string expectedDigest=null) {
            if(gameRunning())throw new InvalidOperationException("Feche o jogo antes de instalar uma atualizacao.");
            if(package.Length>64*1024*1024)throw new InvalidDataException("Pacote excessivo");
            if(expectedDigest!=null && Hash(package)!=expectedDigest)throw new InvalidDataException("Hash do download nao confere");
            var contents=new Dictionary<string,byte[]>(StringComparer.Ordinal);
            using(var zip=new ZipArchive(new MemoryStream(package),ZipArchiveMode.Read)) {
                if(zip.Entries.Count!=Required.Length+1)throw new InvalidDataException("Lista de arquivos inesperada");
                foreach(var entry in zip.Entries) {
                    if(entry.FullName!="manifest.json" && !Required.Contains(entry.FullName))throw new InvalidDataException("Caminho nao permitido no pacote");
                    if(contents.ContainsKey(entry.FullName) || entry.Length>32*1024*1024)throw new InvalidDataException("Entrada duplicada ou excessiva");
                    using(var input=entry.Open())using(var output=new MemoryStream()){
                        byte[] buffer=new byte[8192];int read;
                        while((read=input.Read(buffer,0,buffer.Length))>0){if(output.Length+read>32*1024*1024)throw new InvalidDataException("Entrada excessiva");output.Write(buffer,0,read);}
                        contents.Add(entry.FullName,output.ToArray());
                    }
                }
            }
            var manifest=ReadManifest(System.Text.Encoding.UTF8.GetString(contents["manifest.json"]));
            foreach(var entry in manifest.files)if(Hash(contents[entry.Key])!=entry.Value)throw new InvalidDataException("Integridade invalida: "+entry.Key);
            var current=Current();
            if(current!=null && ParseVersion(manifest.version)<ParseVersion(current.version))throw new InvalidOperationException("Pacote antigo: downgrade recusado.");
            string destination=VersionDirectory(manifest.version);
            if(Directory.Exists(destination)) {
                foreach(var entry in contents)if(!File.Exists(Path.Combine(destination,entry.Key)) || Hash(File.ReadAllBytes(Path.Combine(destination,entry.Key)))!=Hash(entry.Value))
                    throw new InvalidDataException("Esta versao ja existe com outros arquivos; publique uma nova versao.");
            } else {
                Directory.CreateDirectory(Path.GetDirectoryName(destination));
                string stage=destination+".staging-"+Guid.NewGuid().ToString("N");Directory.CreateDirectory(stage);
                try {foreach(var entry in contents)File.WriteAllBytes(Path.Combine(stage,entry.Key),entry.Value);Directory.Move(stage,destination);}
                finally {if(Directory.Exists(stage))Directory.Delete(stage,true);}
            }
            string pointer=Path.Combine(Root,"current.txt"),temporary=pointer+".tmp";
            File.WriteAllText(temporary,manifest.version);
            if(File.Exists(pointer))File.Replace(temporary,pointer,Path.Combine(Root,"previous.txt"));else File.Move(temporary,pointer);
            return manifest;
        }
    }
}
