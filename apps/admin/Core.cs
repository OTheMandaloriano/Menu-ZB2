using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using System.Text.RegularExpressions;
using System.Web.Script.Serialization;

namespace Zb2Admin
{
    public sealed class Station
    {
        public int Schema = 1;
        public string Role = "pending";
        public string Name = "";
        public string Id = "";
        public string ProtectedKey = "";
        public string KeyFormat = "ECCPRIVATEBLOB";
        public string Certificate = "";
    }

    public sealed class Request
    {
        public string Product = "Menu-ZB2";
        public string Id = "";
        public string Name = "";
        public string PublicXY = "";
    }

    public sealed class Grant
    {
        public string Id { get; set; }
        public string Name { get; set; }
        public string PublicXY { get; set; }
        public string Token { get; set; }
        public long Issued { get; set; }
        public long Expires { get; set; }
        public int MaxDays { get; set; }
        public string DateText { get { return Crypto.Date(Expires); } }
    }

    public sealed class LicenseRecord
    {
        public string Id { get; set; }
        public string Customer { get; set; }
        public string Device { get; set; }
        public string Operator { get; set; }
        public string Token { get; set; }
        public long Issued { get; set; }
        public long Expires { get; set; }
        public int Days { get; set; }
        public string Status { get { return Expires > Crypto.Now() ? "Válida" : "Expirada"; } }
        public string DateText { get { return Crypto.Date(Expires); } }
    }

    public static class Crypto
    {
        public static readonly DateTime Epoch = new DateTime(1970, 1, 1, 0, 0, 0, DateTimeKind.Utc);
        public static long Now() { return (long)(DateTime.UtcNow - Epoch).TotalSeconds; }
        public static string Date(long seconds) { return Epoch.AddSeconds(seconds).ToLocalTime().ToString("dd/MM/yyyy HH:mm", CultureInfo.InvariantCulture); }
        public static string Hex(byte[] data) { return BitConverter.ToString(data).Replace("-", "").ToLowerInvariant(); }
        public static byte[] Unhex(string value)
        {
            if (value.Length % 2 != 0 || !Regex.IsMatch(value, "\\A[0-9a-f]*\\z")) throw new InvalidDataException("Formato de assinatura inválido.");
            byte[] result = new byte[value.Length / 2];
            for (int i = 0; i < result.Length; ++i) result[i] = byte.Parse(value.Substring(i * 2, 2), NumberStyles.HexNumber, CultureInfo.InvariantCulture);
            return result;
        }
        public static void Require(bool value, string message) { if (!value) throw new InvalidDataException(message); }
        public static void CheckHex(string value, int length, string field)
        {
            Require(value != null && value.Length == length && Regex.IsMatch(value, "\\A[0-9a-f]+\\z"), field + " incorreto.");
        }
        public static string CheckName(string name)
        {
            name = (name ?? "").Trim();
            Require(name.Length >= 2 && name.Length <= 80 && !name.Any(char.IsControl), "Informe um nome entre 2 e 80 caracteres.");
            return name;
        }
        public static string Public(CngKey key) { return Hex(key.Export(CngKeyBlobFormat.EccPublicBlob).Skip(8).ToArray()); }
        public static byte[] Sign(CngKey key, byte[] payload)
        {
            using (ECDsaCng signer = new ECDsaCng(key))
            using (SHA256 hash = SHA256.Create())
                return signer.SignHash(hash.ComputeHash(payload));
        }
        public static bool Verify(string publicXY, byte[] payload, byte[] signature)
        {
            if (signature.Length != 64) return false;
            byte[] xy = Unhex(publicXY);
            if (xy.Length != 64) return false;
            byte[] blob = new byte[72];
            Array.Copy(new byte[] { 69, 67, 83, 49, 32, 0, 0, 0 }, blob, 8);
            Array.Copy(xy, 0, blob, 8, 64);
            using (CngKey key = CngKey.Import(blob, CngKeyBlobFormat.EccPublicBlob))
            using (ECDsaCng signer = new ECDsaCng(key))
            using (SHA256 hash = SHA256.Create())
                return signer.VerifyHash(hash.ComputeHash(payload), signature);
        }
        public static string Envelope(string prefix, byte[] payload, CngKey key)
        {
            return prefix + "." + Hex(payload) + "." + Hex(Sign(key, payload));
        }
        public static Grant ReadGrant(string token, string rootPublic, long now, bool checkTime = true)
        {
            Require(token != null && token.Length <= 3000, "Autorização inválida.");
            string[] parts = token.Trim().Split('.');
            Require(parts.Length == 3 && parts[0] == "ZB2I1", "Selecione uma autorização de integrante (.zb2issuer).");
            byte[] payload = Unhex(parts[1]);
            Require(Verify(rootPublic, payload, Unhex(parts[2])), "A autorização não foi assinada pelo proprietário deste produto.");
            string[] fields = Encoding.ASCII.GetString(payload).Split('\n');
            Require(fields.Length == 9 && fields[8] == "" && fields[0] == "ZB2-ISSUER-1" && fields[1] == "Menu-ZB2", "Autorização malformada.");
            CheckHex(fields[2], 32, "ID do integrante"); CheckHex(fields[4], 128, "Chave do integrante");
            Grant grant = new Grant();
            grant.Id = fields[2]; grant.Name = new UTF8Encoding(false, true).GetString(Unhex(fields[3]));
            CheckName(grant.Name); Require(Encoding.UTF8.GetByteCount(grant.Name) <= 80, "Nome do integrante excede o limite.");
            grant.PublicXY = fields[4];
            grant.Issued = long.Parse(fields[5], CultureInfo.InvariantCulture);
            grant.Expires = long.Parse(fields[6], CultureInfo.InvariantCulture);
            grant.MaxDays = int.Parse(fields[7], CultureInfo.InvariantCulture);
            Require(grant.Issued > 0 && grant.Expires > grant.Issued && grant.Expires - grant.Issued <= 3650L * 86400 && grant.MaxDays >= 1 && grant.MaxDays <= 3650, "Limites da autorização incorretos.");
            if (checkTime) Require(now >= grant.Issued && now < grant.Expires, "Autorização fora da validade. Solicite uma nova ao proprietário.");
            grant.Token = token.Trim();
            return grant;
        }
    }

    public sealed class AdminStore
    {
        public readonly string Root;
        public readonly string RootPublic;
        public Station Current { get; private set; }
        private readonly JavaScriptSerializer json = new JavaScriptSerializer { MaxJsonLength = 32768 };

        public AdminStore(string root, string rootPublic)
        {
            Crypto.CheckHex(rootPublic, 128, "Chave pública do produto");
            Root = Path.GetFullPath(root); RootPublic = rootPublic;
            string path = Path.Combine(Root, "station.json");
            Current = File.Exists(path) ? json.Deserialize<Station>(ReadLimited(path)) : new Station();
            Crypto.Require(Current != null && Current.Schema == 1, "Estação incompatível. Consulte o proprietário.");
        }

        public static string ReadLimited(string path)
        {
            Crypto.Require(new FileInfo(path).Length <= 32768, "Arquivo muito grande. Confira se selecionou o arquivo correto.");
            return File.ReadAllText(path, Encoding.UTF8);
        }

        public static void AtomicWrite(string path, string text)
        {
            Directory.CreateDirectory(Path.GetDirectoryName(Path.GetFullPath(path)));
            string temporary = path + "." + Guid.NewGuid().ToString("N") + ".tmp";
            try
            {
                byte[] bytes = new UTF8Encoding(false).GetBytes(text);
                using (FileStream file = new FileStream(temporary, FileMode.CreateNew, FileAccess.Write, FileShare.None))
                {
                    file.Write(bytes, 0, bytes.Length); file.Flush(true);
                }
                if (File.Exists(path)) File.Replace(temporary, path, null);
                else File.Move(temporary, path);
            }
            finally { if (File.Exists(temporary)) File.Delete(temporary); }
        }

        private void Save(Station station)
        {
            AtomicWrite(Path.Combine(Root, "station.json"), json.Serialize(station));
            Current = station;
        }

        public CngKey OpenKey()
        {
            Crypto.Require(!String.IsNullOrEmpty(Current.ProtectedKey), "Esta estação ainda não foi configurada.");
            byte[] data;
            try { data = ProtectedData.Unprotect(Convert.FromBase64String(Current.ProtectedKey), null, DataProtectionScope.CurrentUser); }
            catch (CryptographicException) { throw new InvalidDataException("A chave desta estação pertence a outro usuário do Windows. Configure este PC com uma solicitação própria."); }
            try { return CngKey.Import(data, new CngKeyBlobFormat(Current.KeyFormat)); }
            finally { Array.Clear(data, 0, data.Length); }
        }

        public void ImportOwner(string path, string name)
        {
            name = Crypto.CheckName(name);
            Crypto.Require(String.IsNullOrEmpty(Current.ProtectedKey), "Esta estação já possui uma chave. Não substitua uma estação em uso.");
            Crypto.Require(new FileInfo(path).Length < 8192, "Arquivo de chave incorreto.");
            byte[] protectedKey = File.ReadAllBytes(path);
            byte[] raw = ProtectedData.Unprotect(protectedKey, null, DataProtectionScope.CurrentUser);
            try
            {
                using (CngKey key = CngKey.Import(raw, CngKeyBlobFormat.Pkcs8PrivateBlob))
                    Crypto.Require(Crypto.Public(key) == RootPublic, "Essa chave não pertence ao produto instalado.");
            }
            finally { Array.Clear(raw, 0, raw.Length); }
            Save(new Station { Role = "owner", Name = name, Id = Guid.NewGuid().ToString("N"), KeyFormat = "PKCS8_PRIVATEKEY", ProtectedKey = Convert.ToBase64String(protectedKey) });
        }

        public Request CreateStation(string name)
        {
            name = Crypto.CheckName(name);
            Crypto.Require(Encoding.UTF8.GetByteCount(name) <= 80, "Use um nome de integrante mais curto.");
            Crypto.Require(String.IsNullOrEmpty(Current.ProtectedKey), "A estação já existe. Use Exportar solicitação.");
            CngKeyCreationParameters options = new CngKeyCreationParameters { ExportPolicy = CngExportPolicies.AllowPlaintextExport, KeyUsage = CngKeyUsages.Signing };
            using (CngKey key = CngKey.Create(CngAlgorithm.ECDsaP256, null, options))
            {
                byte[] raw = key.Export(CngKeyBlobFormat.EccPrivateBlob);
                try
                {
                    Save(new Station { Role = "operator", Name = name, Id = Guid.NewGuid().ToString("N"), KeyFormat = "ECCPRIVATEBLOB",
                        ProtectedKey = Convert.ToBase64String(ProtectedData.Protect(raw, null, DataProtectionScope.CurrentUser)) });
                }
                finally { Array.Clear(raw, 0, raw.Length); }
            }
            return StationRequest();
        }

        public void RecoverOwner(string path,string name)
        {
            name=Crypto.CheckName(name);
            Crypto.Require(new FileInfo(path).Length<8192,"Arquivo de chave incorreto.");
            byte[] protectedKey=File.ReadAllBytes(path);
            byte[] raw=ProtectedData.Unprotect(protectedKey,null,DataProtectionScope.CurrentUser);
            try {using(CngKey key=CngKey.Import(raw,CngKeyBlobFormat.Pkcs8PrivateBlob))Crypto.Require(Crypto.Public(key)==RootPublic,"Essa chave não pertence ao proprietário deste produto.");}
            finally {Array.Clear(raw,0,raw.Length);}
            if(!String.IsNullOrEmpty(Current.ProtectedKey))AtomicWrite(Path.Combine(Root,"backups","station-"+Guid.NewGuid().ToString("N")+".json"),json.Serialize(Current));
            Save(new Station {Role="owner",Name=name,Id=Guid.NewGuid().ToString("N"),KeyFormat="PKCS8_PRIVATEKEY",ProtectedKey=Convert.ToBase64String(protectedKey)});
        }

        public Request StationRequest()
        {
            Crypto.Require(Current.Role == "operator", "Somente estações de integrantes precisam de solicitação.");
            using (CngKey key = OpenKey()) return new Request { Id = Current.Id, Name = Current.Name, PublicXY = Crypto.Public(key) };
        }
        public string ExportRequest() { return json.Serialize(StationRequest()); }
        public Request ParseRequest(string text)
        {
            Crypto.Require(text.Length <= 8192, "Solicitação muito grande.");
            Request request = json.Deserialize<Request>(text);
            Crypto.Require(request != null && request.Product == "Menu-ZB2", "Solicitação de outro produto.");
            request.Name = Crypto.CheckName(request.Name);
            Crypto.Require(Encoding.UTF8.GetByteCount(request.Name) <= 80, "Nome do integrante muito longo.");
            Crypto.CheckHex(request.Id, 32, "ID da estação"); Crypto.CheckHex(request.PublicXY, 128, "Chave da estação");
            return request;
        }

        public string Authorize(Request request, int validityDays, int maxLicenseDays)
        {
            Crypto.Require(Current.Role == "owner", "Apenas o proprietário autoriza integrantes.");
            request = ParseRequest(json.Serialize(request));
            Crypto.Require(validityDays >= 1 && validityDays <= 3650 && maxLicenseDays >= 1 && maxLicenseDays <= validityDays, "Confira os limites da autorização.");
            long now = Crypto.Now();
            string payload = "ZB2-ISSUER-1\nMenu-ZB2\n" + request.Id + "\n" + Crypto.Hex(Encoding.UTF8.GetBytes(request.Name)) + "\n" + request.PublicXY + "\n" + now + "\n" + (now + validityDays * 86400L) + "\n" + maxLicenseDays + "\n";
            using (CngKey key = OpenKey())
            {
                Crypto.Require(Crypto.Public(key) == RootPublic, "Chave principal incompatível.");
                string token = Crypto.Envelope("ZB2I1", Encoding.ASCII.GetBytes(payload), key);
                AtomicWrite(Path.Combine(Root, "team", request.Id + ".zb2issuer"), token);
                return token;
            }
        }

        public void ImportAuthorization(string token)
        {
            Crypto.Require(Current.Role == "operator", "Crie primeiro a solicitação desta estação.");
            Grant grant = Crypto.ReadGrant(token, RootPublic, Crypto.Now());
            using (CngKey key = OpenKey())
                Crypto.Require(grant.Id == Current.Id && grant.PublicXY == Crypto.Public(key), "A autorização pertence a outra estação.");
            Station next = json.Deserialize<Station>(json.Serialize(Current));
            next.Certificate = token.Trim(); Save(next);
        }

        public int MaxDays()
        {
            using (CngKey key = OpenKey())
            {
                if (Current.Role == "owner") { Crypto.Require(Crypto.Public(key) == RootPublic, "Chave principal incompatível."); return 3650; }
                Grant grant = Crypto.ReadGrant(Current.Certificate, RootPublic, Crypto.Now());
                Crypto.Require(grant.Id == Current.Id && grant.PublicXY == Crypto.Public(key), "Autorização incompatível com esta estação.");
                return Math.Max(0, Math.Min(grant.MaxDays, (int)((grant.Expires - Crypto.Now()) / 86400)));
            }
        }

        public LicenseRecord Issue(string customer, string device, int days)
        {
            customer = Crypto.CheckName(customer); device = (device ?? "").Trim().ToLowerInvariant();
            Crypto.CheckHex(device, 64, "ID do computador do cliente");
            int maximum = MaxDays();
            Crypto.Require(days >= 1 && days <= maximum, "O prazo excede sua autorização. Máximo disponível: " + maximum + " dias.");
            long now = Crypto.Now();
            LicenseRecord record = new LicenseRecord { Id = Guid.NewGuid().ToString("N"), Customer = customer, Device = device, Operator = Current.Name, Issued = now, Expires = now + days * 86400L, Days = days };
            string payload = "ZB2-LICENSE-1\nMenu-ZB2\n" + record.Id + "\n" + device + "\n" + now + "\n" + record.Expires + "\n";
            using (CngKey key = OpenKey())
            {
                record.Token = Crypto.Envelope(Current.Role == "owner" ? "ZB2L1" : "ZB2L2", Encoding.ASCII.GetBytes(payload), key);
                if (Current.Role != "owner")
                {
                    Grant grant = Crypto.ReadGrant(Current.Certificate, RootPublic, now);
                    record.Operator = grant.Name;
                    Crypto.Require(record.Expires <= grant.Expires, "A autorização expira antes deste prazo. Solicite renovação.");
                    string[] certificate = Current.Certificate.Split('.'); record.Token += "." + certificate[1] + "." + certificate[2];
                }
            }
            AtomicWrite(Path.Combine(Root, "licenses", record.Id + ".json"), json.Serialize(record));
            return record;
        }

        public List<LicenseRecord> History()
        {
            string directory = Path.Combine(Root, "licenses");
            if (!Directory.Exists(directory)) return new List<LicenseRecord>();
            List<LicenseRecord> records = new List<LicenseRecord>();
            foreach (string file in Directory.EnumerateFiles(directory, "*.json"))
            {
                LicenseRecord record = json.Deserialize<LicenseRecord>(ReadLimited(file));
                Crypto.Require(record != null && record.Id == Path.GetFileNameWithoutExtension(file) && record.Token != null, "Histórico local danificado: " + Path.GetFileName(file));
                records.Add(record);
            }
            return records.OrderByDescending(r => r.Issued).ToList();
        }

        public List<Grant> TeamHistory()
        {
            string directory = Path.Combine(Root, "team");
            if (!Directory.Exists(directory)) return new List<Grant>();
            return Directory.EnumerateFiles(directory, "*.zb2issuer")
                .Select(path => Crypto.ReadGrant(ReadLimited(path), RootPublic, Crypto.Now(), false))
                .OrderByDescending(grant => grant.Issued).ToList();
        }
    }
}
