using System;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Text;

namespace Zb2Admin
{
    // Versioned authenticated envelope. No plaintext key is ever written to disk.
    // AES-256-CBC + HMAC-SHA256 with separate keys, authenticated before decryption.
    public static class Recovery
    {
        private static readonly byte[] Magic=Encoding.ASCII.GetBytes("DBREC001");
        private const int Iterations=600000, MaxFile=16384;
        private static void Clear(byte[] value){if(value!=null)Array.Clear(value,0,value.Length);}
        private static void Password(string password){Crypto.Require(password!=null&&password.Length>=12&&password.Length<=128&&!String.IsNullOrWhiteSpace(password),"Use uma senha entre 12 e 128 caracteres. Guarde-a fora do arquivo.");}
        private static void Safe(string path)
        {
            string full=Path.GetFullPath(path);
            if(File.Exists(full))Crypto.Require((File.GetAttributes(full)&FileAttributes.ReparsePoint)==0,"Arquivo redirecionado não permitido.");
            for(DirectoryInfo d=new DirectoryInfo(Path.GetDirectoryName(full));d!=null;d=d.Parent)
                if(d.Exists)Crypto.Require((d.Attributes&FileAttributes.ReparsePoint)==0,"Pasta redirecionada não permitida para recuperação.");
        }
        public static void Export(AdminStore store,string path,string password)
        {
            Password(password);Crypto.Require(store.Current.Role=="owner"&&store.MaxDays()>0,"Somente o proprietário pode exportar sua recuperação.");
            Crypto.Require(Path.IsPathRooted(path)&&Path.GetExtension(path).Equals(".dbrecovery",StringComparison.OrdinalIgnoreCase),"Escolha um arquivo .dbrecovery.");
            Safe(path);Crypto.Require(Directory.Exists(Path.GetDirectoryName(path))&&!File.Exists(path),"Escolha um nome novo em uma pasta existente. Backups não são sobrescritos.");
            byte[] raw=null,plain=null,keys=null,encrypted=null;
            string temporary=path+"."+Guid.NewGuid().ToString("N")+".tmp";
            try
            {
                using(CngKey key=store.OpenKey())
                {
                    Crypto.Require(Crypto.Public(key)==store.RootPublic,"Chave de proprietário incompatível.");
                    Crypto.Require(store.Current.KeyFormat=="PKCS8_PRIVATEKEY","Formato de proprietário não suportado para exportação.");
                    raw=ProtectedData.Unprotect(Convert.FromBase64String(store.Current.ProtectedKey),null,DataProtectionScope.CurrentUser);
                }
                using(MemoryStream data=new MemoryStream())using(BinaryWriter writer=new BinaryWriter(data,new UTF8Encoding(false,true)))
                {writer.Write("Menu-ZB2");writer.Write(store.RootPublic);writer.Write(Crypto.CheckName(store.Current.Name));writer.Write(raw.Length);writer.Write(raw);writer.Flush();plain=data.ToArray();}
                byte[] salt=new byte[16],iv=new byte[16];using(RandomNumberGenerator random=RandomNumberGenerator.Create()){random.GetBytes(salt);random.GetBytes(iv);}
                using(Rfc2898DeriveBytes kdf=new Rfc2898DeriveBytes(password,salt,Iterations,HashAlgorithmName.SHA256))keys=kdf.GetBytes(64);
                using(Aes aes=Aes.Create()){aes.KeySize=256;aes.Mode=CipherMode.CBC;aes.Padding=PaddingMode.PKCS7;aes.Key=keys.Take(32).ToArray();aes.IV=iv;using(ICryptoTransform transform=aes.CreateEncryptor())encrypted=transform.TransformFinalBlock(plain,0,plain.Length);}
                byte[] envelope;
                using(MemoryStream data=new MemoryStream())using(BinaryWriter writer=new BinaryWriter(data))
                {writer.Write(Magic);writer.Write(Iterations);writer.Write(salt);writer.Write(iv);writer.Write(encrypted.Length);writer.Write(encrypted);writer.Flush();envelope=data.ToArray();}
                byte[] tag;using(HMACSHA256 hmac=new HMACSHA256(keys.Skip(32).ToArray()))tag=hmac.ComputeHash(envelope);
                using(FileStream file=new FileStream(temporary,FileMode.CreateNew,FileAccess.Write,FileShare.None)){file.Write(envelope,0,envelope.Length);file.Write(tag,0,tag.Length);file.Flush(true);}
                Safe(path);File.Move(temporary,path);
            }
            finally{Clear(raw);Clear(plain);Clear(keys);Clear(encrypted);if(File.Exists(temporary))File.Delete(temporary);}
        }
        public static void Restore(AdminStore store,string path,string password)
        {
            Password(password);Crypto.Require(String.IsNullOrEmpty(store.Current.ProtectedKey)&&!File.Exists(Path.Combine(store.Root,"station.json")),"Este perfil já está configurado. A recuperação não substitui uma estação existente.");
            Safe(path);Safe(Path.Combine(store.Root,"station.json"));
            Crypto.Require(new FileInfo(path).Length>=96&&new FileInfo(path).Length<=MaxFile,"Arquivo de recuperação inválido ou incompleto.");
            byte[] envelope=File.ReadAllBytes(path),keys=null,plain=null,raw=null;
            try
            {
                byte[] salt,iv,cipher;int iterations,length;
                using(MemoryStream stream=new MemoryStream(envelope))using(BinaryReader reader=new BinaryReader(stream))
                {Crypto.Require(reader.ReadBytes(8).SequenceEqual(Magic),"Formato de recuperação desconhecido.");iterations=reader.ReadInt32();Crypto.Require(iterations==Iterations,"Versão de recuperação não suportada.");salt=reader.ReadBytes(16);iv=reader.ReadBytes(16);length=reader.ReadInt32();Crypto.Require(length>0&&length%16==0&&length<=8192&&envelope.Length==48+length+32,"Arquivo de recuperação inválido.");cipher=reader.ReadBytes(length);}
                using(Rfc2898DeriveBytes kdf=new Rfc2898DeriveBytes(password,salt,iterations,HashAlgorithmName.SHA256))keys=kdf.GetBytes(64);
                byte[] expected;using(HMACSHA256 hmac=new HMACSHA256(keys.Skip(32).ToArray()))expected=hmac.ComputeHash(envelope,0,envelope.Length-32);
                int difference=0;for(int i=0;i<32;++i)difference|=expected[i]^envelope[envelope.Length-32+i];
                Crypto.Require(difference==0,"Senha incorreta ou arquivo alterado. Nenhum dado foi modificado.");
                using(Aes aes=Aes.Create()){aes.KeySize=256;aes.Mode=CipherMode.CBC;aes.Padding=PaddingMode.PKCS7;aes.Key=keys.Take(32).ToArray();aes.IV=iv;using(ICryptoTransform transform=aes.CreateDecryptor())plain=transform.TransformFinalBlock(cipher,0,cipher.Length);}
                string name;
                using(MemoryStream data=new MemoryStream(plain))using(BinaryReader reader=new BinaryReader(data,new UTF8Encoding(false,true)))
                {Crypto.Require(reader.ReadString()=="Menu-ZB2"&&reader.ReadString()==store.RootPublic,"Este backup pertence a outro produto/autoridade.");name=Crypto.CheckName(reader.ReadString());int size=reader.ReadInt32();Crypto.Require(size>0&&size<=4096&&data.Length-data.Position==size,"Conteúdo de recuperação inválido.");raw=reader.ReadBytes(size);}
                using(CngKey key=CngKey.Import(raw,CngKeyBlobFormat.Pkcs8PrivateBlob))Crypto.Require(Crypto.Public(key)==store.RootPublic,"A chave restaurada não corresponde ao produto.");
                // Re-protect under the target Windows account. The portable envelope has no DPAPI dependency.
                store.RestorePortableOwner(raw,name);
            }
            finally{Clear(envelope);Clear(keys);Clear(plain);Clear(raw);}
        }
    }
}
