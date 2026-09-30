using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Web.Script.Serialization;

namespace Zb2Admin
{
    // Headless local service. Pipes carry public UI data only, never station keys.
    internal static class Backend
    {
        private static readonly UTF8Encoding Utf8 = new UTF8Encoding(false, true);
        private static string Field(string value) { return Crypto.Hex(Utf8.GetBytes(value ?? "")); }
        private static void Row(StringBuilder output, string type, params string[] fields)
        {
            output.Append(type);
            foreach(string value in fields) output.Append('\t').Append(Field(value));
            output.Append('\n');
        }
        private static int Number(string value)
        {
            int result;
            Crypto.Require(Int32.TryParse(value,NumberStyles.None,CultureInfo.InvariantCulture,out result),"Informe uma quantidade inteira de dias.");
            return result;
        }
        private static string PublicKey()
        {
            using(Stream stream=Assembly.GetExecutingAssembly().GetManifestResourceStream("Admin.Public"))
            using(StreamReader reader=new StreamReader(stream))
                return new JavaScriptSerializer().Deserialize<Dictionary<string,string>>(reader.ReadToEnd())["public_xy"];
        }
        internal static string Execute(string root,string packet)
        {
            string[] fields=packet.Split('\t');
            string command=fields[0];
            string[] args=fields.Skip(1).Select(item=>Utf8.GetString(Crypto.Unhex(item))).ToArray();
            AdminStore store=new AdminStore(root,PublicKey());
            StringBuilder extra=new StringBuilder();string notice="";string query="";
            switch(command)
            {
                case "snapshot":
                    Crypto.Require(args.Length<=1,"Consulta incorreta.");query=args.Length==1?args[0]:"";break;
                case "issue":
                    Crypto.Require(args.Length==3,"Dados da licença incompletos.");
                    LicenseRecord issued=store.Issue(args[0],args[1],Number(args[2]));
                    Row(extra,"X",issued.Token,"Licenca-"+issued.Id.Substring(0,8)+".zb2license");
                    notice="Licença gerada. Salve o arquivo e envie ao cliente.";break;
                case "create_station":
                    Crypto.Require(args.Length==1,"Informe o nome do integrante.");store.CreateStation(args[0]);
                    Row(extra,"X",store.ExportRequest(),"Solicitacao-"+store.Current.Id.Substring(0,8)+".zb2station");
                    notice="Solicitação criada. Envie o arquivo ao proprietário.";break;
                case "export_request":
                    Crypto.Require(args.Length==0,"Pedido incorreto.");
                    Row(extra,"X",store.ExportRequest(),"Solicitacao-"+store.Current.Id.Substring(0,8)+".zb2station");notice="Solicitação pronta para salvar.";break;
                case "inspect_request":
                    Crypto.Require(args.Length==1,"Selecione a solicitação.");
                    string requestText=AdminStore.ReadLimited(args[0]);Request request=store.ParseRequest(requestText);
                    Row(extra,"R",request.Name,request.Id,requestText);notice="Solicitação conferida. Defina os limites e autorize.";break;
                case "authorize":
                    Crypto.Require(args.Length==3,"Dados da autorização incompletos.");
                    Request selected=store.ParseRequest(args[0]);string grant=store.Authorize(selected,Number(args[1]),Number(args[2]));
                    Row(extra,"X",grant,"Autorizacao-"+selected.Id.Substring(0,8)+".zb2issuer");notice="Autorização gerada. Envie-a ao integrante.";break;
                case "import_authorization":
                    Crypto.Require(args.Length==1,"Selecione a autorização.");store.ImportAuthorization(AdminStore.ReadLimited(args[0]));notice="Estação autorizada para emitir licenças.";break;
                case "import_owner":
                    Crypto.Require(args.Length==2,"Informe nome e chave do proprietário.");store.ImportOwner(args[0],args[1]);notice="Proprietário configurado neste perfil do Windows.";break;
                default:throw new InvalidDataException("Ação não reconhecida. Atualize o Admin.");
            }
            int maxDays=0;string problem="";string grantExpiry="";
            if(!String.IsNullOrEmpty(store.Current.ProtectedKey))
                try { maxDays=store.MaxDays();if(store.Current.Role=="operator")grantExpiry=Crypto.Date(Crypto.ReadGrant(store.Current.Certificate,store.RootPublic,Crypto.Now()).Expires); }
                catch(Exception error){problem=error.Message;}
            List<LicenseRecord> history=new List<LicenseRecord>();List<Grant> grants=new List<Grant>();
            try { history=store.History();grants=store.TeamHistory(); }
            catch(Exception error){notice+=(notice.Length>0?" ":"")+"Histórico indisponível: "+error.Message;}
            StringBuilder output=new StringBuilder();Row(output,"OK",notice);
            Row(output,"S",store.Current.Role,store.Current.Name,store.Current.Id,maxDays.ToString(CultureInfo.InvariantCulture),problem,grantExpiry,Environment.UserName,String.IsNullOrEmpty(store.Current.ProtectedKey)?"0":"1");
            IEnumerable<LicenseRecord> filtered=history.Where(record=>(record.Customer+" "+record.Device+" "+record.Operator).IndexOf(query,StringComparison.OrdinalIgnoreCase)>=0);
            Row(output,"N",history.Count.ToString(CultureInfo.InvariantCulture),filtered.Count().ToString(CultureInfo.InvariantCulture));
            foreach(LicenseRecord record in filtered.Take(100))Row(output,"L",record.Id,record.Customer,record.Device,record.Operator,record.Token,record.DateText,record.Days.ToString(CultureInfo.InvariantCulture),record.Status,record.Expires.ToString(CultureInfo.InvariantCulture));
            foreach(Grant entry in grants.Take(100))Row(output,"G",entry.Id,entry.Name,entry.DateText,entry.MaxDays.ToString(CultureInfo.InvariantCulture),entry.Token);
            output.Append(extra);return output.ToString();
        }
        private static int Main(string[] args)
        {
            byte[] response;
            try
            {
                Crypto.Require(args.Length==2 && args[0]=="--data","Pasta da estação não informada.");
                using(BinaryReader reader=new BinaryReader(Console.OpenStandardInput(),Utf8,true))
                {
                    int length=reader.ReadInt32();Crypto.Require(length>0 && length<=131072,"Pedido fora do limite.");
                    byte[] request=reader.ReadBytes(length);Crypto.Require(request.Length==length,"Pedido incompleto.");
                    response=Utf8.GetBytes(Execute(args[1],Utf8.GetString(request)));
                }
            }
            catch(Exception error){StringBuilder text=new StringBuilder();Row(text,"ERR",error.Message);response=Utf8.GetBytes(text.ToString());}
            using(BinaryWriter writer=new BinaryWriter(Console.OpenStandardOutput(),Utf8,true)){writer.Write(response.Length);writer.Write(response);writer.Flush();}
            return 0;
        }
    }
}
