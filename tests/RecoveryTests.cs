using System;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using Zb2Admin;
class RecoveryTests
{
    const string Password="Test-only recovery phrase 2026!";
    static int count;
    static void Check(bool value,string name){++count;if(!value)throw new Exception(name);}
    static void Reject(Action action,string name){bool rejected=false;try{action();}catch(Exception){rejected=true;}Check(rejected,name);}
    static string Public(string path){return File.ReadAllText(path).Trim();}
    static int Main(string[] args)
    {
        try{
            string root=Path.GetFullPath(args[1]);Directory.CreateDirectory(root);
            if(args[0]=="create"){
                byte[] raw;string pub;
                using(CngKey key=CngKey.Create(CngAlgorithm.ECDsaP256,null,new CngKeyCreationParameters{ExportPolicy=CngExportPolicies.AllowPlaintextExport})){raw=key.Export(CngKeyBlobFormat.Pkcs8PrivateBlob);pub=Crypto.Public(key);}
                try{File.WriteAllBytes(Path.Combine(root,"fixture.dpapi"),ProtectedData.Protect(raw,null,DataProtectionScope.CurrentUser));}finally{Array.Clear(raw,0,raw.Length);}
                File.WriteAllText(Path.Combine(root,"public.txt"),pub);
                AdminStore owner=new AdminStore(Path.Combine(root,"original"),pub);owner.ImportOwner(Path.Combine(root,"fixture.dpapi"),"RecoveryFixture");
                Recovery.Export(owner,Path.Combine(root,"portable.dbrecovery"),Password);
                File.WriteAllText(Path.Combine(root,"source-user.txt"),Environment.UserDomainName+"/"+Environment.UserName);
                Console.WriteLine("Test authority and encrypted recovery created. No production key used.");return 0;
            }
            string publicKey=Public(Path.Combine(root,"public.txt"));string backup=Path.Combine(root,"portable.dbrecovery");
            if(args[0]=="recover-after-loss"){
                Check(!File.Exists(Path.Combine(root,"original","station.json"))&&!File.Exists(Path.Combine(root,"fixture.dpapi")),"test source really removed");
                AdminStore recovered=new AdminStore(Path.Combine(root,"after-loss"),publicKey);Recovery.Restore(recovered,backup,Password);Check(recovered.MaxDays()==3650,"restore after losing fixture source");
                var restored=recovered.Issue("Recovered client",new string('1',64),7);string[] parts=restored.Token.Split('.');Check(Crypto.Verify(publicKey,Crypto.Unhex(parts[1]),Crypto.Unhex(parts[2])),"same authority after source deletion");Console.WriteLine(count+" loss-of-source checks passed.");return 0;
            }
            if(args[0]=="cross-user"){
                Check(File.ReadAllText(Path.Combine(root,"source-user.txt"))!=Environment.UserDomainName+"/"+Environment.UserName,"must test another Windows user");
                Reject(delegate{ProtectedData.Unprotect(File.ReadAllBytes(Path.Combine(root,"fixture.dpapi")),null,DataProtectionScope.CurrentUser);},"original DPAPI cannot be opened by target user");
                AdminStore target=new AdminStore(Path.Combine(root,"new-user"),publicKey);Recovery.Restore(target,backup,Password);
                Check(target.MaxDays()==3650,"owner recovered on different Windows user");var license=target.Issue("Recovery client",new string('1',64),30);string[] token=license.Token.Split('.');Check(Crypto.Verify(publicKey,Crypto.Unhex(token[1]),Crypto.Unhex(token[2])),"restored owner signs with original authority");
                Console.WriteLine(count+" cross-user DPAPI recovery checks passed.");return 0;
            }
            byte[] original=File.ReadAllBytes(backup);AdminStore owner2=new AdminStore(Path.Combine(root,"original"),publicKey);
            string second=Path.Combine(root,"second.dbrecovery");Recovery.Export(owner2,second,Password);Check(!original.SequenceEqual(File.ReadAllBytes(second)),"fresh salt and IV");
            Check(!Encoding.UTF8.GetString(original).Contains("RecoveryFixture"),"name not plaintext");
            Reject(delegate{Recovery.Export(owner2,backup,Password);},"no overwrite backup");
            Reject(delegate{Recovery.Export(owner2,Path.Combine(root,"weak.dbrecovery"),"short");},"reject weak length");
            AdminStore fresh=new AdminStore(Path.Combine(root,"fresh"),publicKey);
            Reject(delegate{Recovery.Restore(fresh,backup,"Incorrect test phrase");},"wrong password");Check(!File.Exists(Path.Combine(fresh.Root,"station.json")),"wrong password no mutation");
            byte[] changed=(byte[])original.Clone();changed[60]^=1;string tampered=Path.Combine(root,"tampered.dbrecovery");File.WriteAllBytes(tampered,changed);
            Reject(delegate{Recovery.Restore(fresh,tampered,Password);},"authenticated ciphertext");
            changed=(byte[])original.Clone();changed[24]^=1;File.WriteAllBytes(tampered,changed);Reject(delegate{Recovery.Restore(fresh,tampered,Password);},"authenticated IV");
            File.WriteAllBytes(tampered,original.Take(50).ToArray());Reject(delegate{Recovery.Restore(fresh,tampered,Password);},"truncation");
            File.WriteAllBytes(tampered,new byte[20000]);Reject(delegate{Recovery.Restore(fresh,tampered,Password);},"file limit");
            Recovery.Restore(fresh,backup,Password);Check(fresh.MaxDays()==3650,"restore to empty profile");
            byte[] saved=File.ReadAllBytes(Path.Combine(fresh.Root,"station.json"));Reject(delegate{Recovery.Restore(fresh,backup,Password);},"no overwrite station");Check(saved.SequenceEqual(File.ReadAllBytes(Path.Combine(fresh.Root,"station.json"))),"existing station intact");
            string other;using(CngKey key=CngKey.Create(CngAlgorithm.ECDsaP256))other=Crypto.Public(key);
            Reject(delegate{Recovery.Restore(new AdminStore(Path.Combine(root,"other-product"),other),backup,Password);},"reject different authority");
            AdminStore member=new AdminStore(Path.Combine(root,"member"),publicKey);member.CreateStation("MemberFixture");Reject(delegate{Recovery.Export(member,Path.Combine(root,"member.dbrecovery"),Password);},"member cannot export owner recovery");
            Check(!Directory.GetFiles(root,"*.tmp",SearchOption.AllDirectories).Any(),"no residual temp files");
            Check(owner2.MaxDays()==3650,"source owner intact");Console.WriteLine(count+" recovery integrity and failure checks passed.");return 0;
        }catch(Exception e){Console.Error.WriteLine(e);return 1;}
    }
}
