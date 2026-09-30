using System;
using System.Collections.Generic;
using System.IO;
using System.Security.Cryptography;
using System.Text;
using System.Web.Script.Serialization;
using Zb2Admin;

internal static class AdminCoreTests
{
    private static int checks;
    private static void Check(bool value, string name) { ++checks; if (!value) throw new Exception(name); }
    private static void Reject(Action action, string name)
    {
        bool failed = false; try { action(); } catch (InvalidDataException) { failed = true; }
        Check(failed, name);
    }
    public static int Main(string[] args)
    {
        try
        {
            JavaScriptSerializer json = new JavaScriptSerializer();
            string pub = json.Deserialize<Dictionary<string, string>>(File.ReadAllText(args[1]))["public_xy"];
            string directory = Path.GetFullPath(args[2]);
            AdminStore owner = new AdminStore(Path.Combine(directory, "owner"), pub);
            owner.ImportOwner(args[0], "Proprietário de teste");
            Check(owner.MaxDays() == 3650, "Owner maximum");
            LicenseRecord direct = owner.Issue("Cliente teste", new string('1', 64), 7);
            Check(direct.Token.StartsWith("ZB2L1."), "Owner protocol");
            string[] parts = direct.Token.Split('.');
            Check(Crypto.Verify(pub, Crypto.Unhex(parts[1]), Crypto.Unhex(parts[2])), "Owner signature");
            Check(owner.History().Count == 1, "History after issue");
            Check(new AdminStore(Path.Combine(directory, "owner"), pub).History()[0].Customer == "Cliente teste", "History persists");
            Reject(delegate { owner.Issue("", new string('1', 64), 7); }, "Empty name");
            Reject(delegate { owner.Issue("Cliente", "bad", 7); }, "Bad device");
            Reject(delegate { owner.Issue("Cliente", new string('1', 64), 0); }, "Bad days");
            Reject(delegate { owner.ImportOwner(args[0], "Outra pessoa"); }, "No key overwrite");

            AdminStore member = new AdminStore(Path.Combine(directory, "member"), pub);
            Request request = member.CreateStation("Operador de teste");
            Check(request.PublicXY.Length == 128, "Operator public key");
            Reject(delegate { member.Issue("Cliente", new string('1', 64), 1); }, "Unapproved operator");
            string grant = owner.Authorize(owner.ParseRequest(member.ExportRequest()), 365, 30);
            member.ImportAuthorization(grant);
            Check(member.MaxDays() == 30, "Operator maximum");
            LicenseRecord delegated = member.Issue("Cliente delegado", new string('1', 64), 30);
            Check(delegated.Days==30 && delegated.Expires-delegated.Issued==30L*86400,"Delegated exact 30 days");
            Check(delegated.Token.StartsWith("ZB2L2."), "Delegated protocol");
            Reject(delegate { member.Issue("Cliente", new string('1', 64), 31); }, "Operator day limit");
            Reject(delegate { member.Authorize(request, 365, 30); }, "Operator cannot authorize other operators");
            AdminStore stranger = new AdminStore(Path.Combine(directory, "stranger"), pub);
            stranger.CreateStation("Outra estação");
            Reject(delegate { stranger.ImportAuthorization(grant); }, "Grant bound to station");
            Reject(delegate { member.ImportAuthorization(grant.Substring(0, grant.Length-1) + (grant.EndsWith("0") ? "1" : "0")); }, "Tampered grant");
            Check(owner.History().Count == 1 && member.History().Count == 1, "Rejected actions leave history intact");
            Check(owner.TeamHistory().Count == 1 && owner.TeamHistory()[0].MaxDays == 30, "Team history persists");
            member.Current.Role = "owner";
            Reject(delegate { member.Issue("Cliente", new string('1', 64), 90); }, "Editing role cannot grant owner authority");
            member.Current.Role = "operator";

            Dictionary<string, string> fixtures = new Dictionary<string, string>();
            fixtures["owner"] = direct.Token; fixtures["operator"] = delegated.Token;
            using (CngKey key = member.OpenKey())
            {
                string[] delegatedParts = delegated.Token.Split('.');
                string payload = Encoding.ASCII.GetString(Crypto.Unhex(delegatedParts[1]));
                string longPayload = payload.Replace(delegated.Expires.ToString(), (delegated.Issued + 31L * 86400).ToString());
                fixtures["over_limit"] = Crypto.Envelope("ZB2L2", Encoding.ASCII.GetBytes(longPayload), key) + "." + delegatedParts[3] + "." + delegatedParts[4];
            }
            File.WriteAllText(Path.Combine(directory, "fixtures.json"), json.Serialize(fixtures), new UTF8Encoding(false));
            string oldStation=File.ReadAllText(Path.Combine(directory,"member","station.json"));
            member.RecoverOwner(args[0],"WeFagundes");
            Check(member.Current.Role=="owner" && member.Current.Name=="WeFagundes" && member.MaxDays()==3650,"Verified recovery from mistaken member registration");
            Check(member.History().Count==1,"Recovery preserves license history");
            string[] backups=Directory.GetFiles(Path.Combine(directory,"member","backups"),"*.json");
            Check(backups.Length==1 && File.ReadAllText(backups[0])==oldStation,"Recovery retains protected old station backup");
            Console.WriteLine(checks + " admin core checks passed.");
            return 0;
        }
        catch (Exception error) { Console.Error.WriteLine(error); return 1; }
    }
}
