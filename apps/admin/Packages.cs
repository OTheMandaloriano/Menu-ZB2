using System;
using System.IO;
using System.IO.Compression;
using System.Reflection;
using System.Text;
namespace Zb2Admin
{
    public static class Packages
    {
        public static void CheckDestination(string path)
        {
            Crypto.Require(!String.IsNullOrWhiteSpace(path) && Path.IsPathRooted(path) && String.Equals(Path.GetExtension(path),".zip",StringComparison.OrdinalIgnoreCase),"Escolha onde salvar o arquivo ZIP.");
            string directory=Path.GetDirectoryName(Path.GetFullPath(path));
            Crypto.Require(Directory.Exists(directory),"A pasta de destino não existe.");
            for(DirectoryInfo folder=new DirectoryInfo(directory);folder!=null;folder=folder.Parent)
                Crypto.Require((folder.Attributes&FileAttributes.ReparsePoint)==0,"Escolha uma pasta local sem redirecionamento.");
            if(File.Exists(path))Crypto.Require((File.GetAttributes(path)&FileAttributes.ReparsePoint)==0,"O destino é um arquivo redirecionado.");
        }
        private static void Write(string path,byte[] executable,string executableName,string token,string tokenName)
        {
            CheckDestination(path);
            Crypto.Require(executable.Length>1024 && executable.Length<32*1024*1024 && executable[0]==77 && executable[1]==90,"Executável de entrega inválido.");
            string temporary=path+"."+Guid.NewGuid().ToString("N")+".tmp";
            try
            {
                using(FileStream file=new FileStream(temporary,FileMode.CreateNew,FileAccess.Write,FileShare.None))
                {
                    using(ZipArchive zip=new ZipArchive(file,ZipArchiveMode.Create,true))
                    {
                        using(Stream item=zip.CreateEntry(executableName,CompressionLevel.Optimal).Open())item.Write(executable,0,executable.Length);
                        if(!String.IsNullOrEmpty(token))using(Stream item=zip.CreateEntry(tokenName,CompressionLevel.Optimal).Open())
                        { byte[] data=new UTF8Encoding(false).GetBytes(token.Trim()+"\n");item.Write(data,0,data.Length); }
                    }
                    file.Flush(true);
                }
                using(ZipArchive zip=ZipFile.OpenRead(temporary))
                    Crypto.Require(zip.Entries.Count==(String.IsNullOrEmpty(token)?1:2),"Pacote incompleto.");
                if(File.Exists(path))File.Replace(temporary,path,null);else File.Move(temporary,path);
            }
            finally {if(File.Exists(temporary))File.Delete(temporary);}
        }
        public static void Client(string path,LicenseRecord license)
        {
            Crypto.Require(license.Expires>Crypto.Now(),"Esta licença expirou. Gere uma renovação antes de preparar o pacote.");
            byte[] bytes;
            using(Stream stream=Assembly.GetExecutingAssembly().GetManifestResourceStream("Admin.Client"))
            {
                Crypto.Require(stream!=null && stream.Length<32*1024*1024,"Loader não incluído nesta versão do Admin.");
                using(MemoryStream output=new MemoryStream()){stream.CopyTo(output);bytes=output.ToArray();}
            }
            Write(path,bytes,"ZB2Menu.exe",license.Token,"licenca.zb2license");
        }
        public static void Team(string path,string executable,Grant grant)
        {
            Crypto.Require(grant.Expires>Crypto.Now(),"Autorização expirada. Renove antes de preparar o pacote.");
            Crypto.Require(File.Exists(executable) && new FileInfo(executable).Length<32*1024*1024,"Aplicativo Admin indisponível para empacotar.");
            Write(path,File.ReadAllBytes(executable),"ZB2Admin.exe",grant.Token,"autorizacao.zb2issuer");
        }
    }
}
