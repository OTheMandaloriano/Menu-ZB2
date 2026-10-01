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
    // Inherited local pipes carry requests, including recovery passwords; never log
    // packets or put credentials on the process command line. Keys stay in this service.
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
                case "client_starter":
                    Crypto.Require(args.Length==1,"Escolha onde salvar o programa inicial do cliente.");
                    Packages.ClientStarter(args[0]);notice="Programa inicial salvo. Envie o ZIP; o cliente abre e clica em Copiar ID.";break;
                case "team_starter":
                    Crypto.Require(args.Length==2,"Escolha onde salvar o programa inicial da equipe.");
                    Packages.TeamStarter(args[0],args[1]);notice="Programa inicial salvo. O integrante abre Meu acesso, cria a solicitação e envia para você.";break;
                case "export_recovery":
                    Crypto.Require(args.Length==2,"Informe destino e senha da recuperação.");
                    Recovery.Export(store,args[0],args[1]);notice="Recuperação salva. Guarde arquivo e senha separados. Este backup não inclui o histórico.";break;
                case "restore_recovery":
                    Crypto.Require(args.Length==2,"Informe arquivo e senha da recuperação.");
                    Recovery.Restore(store,args[0],args[1]);notice="Acesso de proprietário restaurado neste Windows. Histórico anterior não foi importado.";break;
                case "snapshot":
                    Crypto.Require(args.Length<=1,"Consulta incorreta.");query=args.Length==1?args[0]:"";break;
                case "issue":
                    Crypto.Require(args.Length==3,"Dados da licença incompletos.");
                    LicenseRecord issued=store.Issue(args[0],args[1],Number(args[2]));
                    Row(extra,"I",issued.Id);
                    Row(extra,"X",issued.Token,"Licenca-"+issued.Id.Substring(0,8)+".zb2license");
                    notice="Licença emitida: "+issued.Days+" dias · até "+issued.DateText+".";break;
                case "issue_package":
                    Crypto.Require(args.Length==4,"Informe cliente, computador, prazo e destino do ZIP.");
                    Packages.CheckDestination(args[3]);
                    LicenseRecord packed=store.Issue(args[0],args[1],Number(args[2]));
                    Row(extra,"I",packed.Id);
                    Row(extra,"X",packed.Token,"Licenca-"+packed.Id.Substring(0,8)+".zb2license");
                    try { Packages.Client(args[3],packed);notice="ZIP pronto: "+packed.Days+" dias · até "+packed.DateText+"."; }
                    catch(Exception error){notice="Licença salva no histórico; ZIP não concluído: "+error.Message+" Use a ação ZIP dessa licença no histórico, sem emitir novamente.";}
                    break;
                case "client_package":
                    Crypto.Require(args.Length==2,"Selecione uma licença e o destino do ZIP.");
                    Crypto.Require(store.MaxDays()>0,"Esta estação não está autorizada.");
                    LicenseRecord existing=store.History().SingleOrDefault(record=>record.Id==args[0]);
                    Crypto.Require(existing!=null,"Licença não encontrada no histórico desta estação.");
                    Packages.Client(args[1],existing);notice="ZIP pronto. Envie somente esse ZIP ao cliente.";break;
                case "team_package":
                    Crypto.Require(args.Length==3 && store.Current.Role=="owner" && store.MaxDays()>0,"Somente o proprietário prepara pacotes da equipe.");
                    Grant team=store.TeamHistory().SingleOrDefault(entry=>entry.Id==args[0]);Crypto.Require(team!=null,"Selecione uma autorização da equipe.");
                    Packages.Team(args[1],args[2],team);notice="ZIP pronto para o integrante autorizado. Ele extrai e abre o Admin no PC da solicitação.";break;
                case "authorize_package":
                    Crypto.Require(args.Length==5,"Selecione a solicitação, os limites e o destino do ZIP.");
                    Packages.CheckDestination(args[3]);Request candidate=store.ParseRequest(args[0]);
                    string approval=store.Authorize(candidate,Number(args[1]),Number(args[2]));
                    Row(extra,"X",approval,"Autorizacao-"+candidate.Id.Substring(0,8)+".zb2issuer");
                    try { Packages.Team(args[3],args[4],Crypto.ReadGrant(approval,store.RootPublic,Crypto.Now()));notice="Integrante autorizado. Envie o ZIP pronto para o PC da solicitação."; }
                    catch(Exception error){notice="Autorização salva; ZIP não concluído: "+error.Message+" Abra as ações da estação no histórico para salvar o ZIP novamente.";}
                    break;
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
                    Crypto.Require(args.Length==2,"Informe nome e chave do proprietário.");store.RecoverOwner(args[0],args[1]);notice="Proprietário confirmado pela chave original. Histórico preservado.";break;
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
                Crypto.Require((args.Length==2||args.Length==4) && args[0]=="--data","Pasta da estação não informada.");
                string startupNotice="";
                if(args.Length==4 && args[2]=="--authorization")
                {
                    AdminStore station=new AdminStore(args[1],PublicKey());
                    if(station.Current.Role=="operator")try
                    {
                        string grant=AdminStore.ReadLimited(args[3]).Trim();
                        if(grant!=station.Current.Certificate)station.ImportAuthorization(grant);
                    }
                    catch(Exception error){startupNotice="Autorização do pacote não aplicada: "+error.Message;}
                }
                using(BinaryReader reader=new BinaryReader(Console.OpenStandardInput(),Utf8,true))
                {
                    int length=reader.ReadInt32();Crypto.Require(length>0 && length<=131072,"Pedido fora do limite.");
                    byte[] request=reader.ReadBytes(length);Crypto.Require(request.Length==length,"Pedido incompleto.");
                    string result=Execute(args[1],Utf8.GetString(request));
                    if(startupNotice.Length>0)result += "W\t"+Field(startupNotice)+"\n";
                    response=Utf8.GetBytes(result);
                }
            }
            catch(Exception error){StringBuilder text=new StringBuilder();Row(text,"ERR",error.Message);response=Utf8.GetBytes(text.ToString());}
            using(BinaryWriter writer=new BinaryWriter(Console.OpenStandardOutput(),Utf8,true)){writer.Write(response.Length);writer.Write(response);writer.Flush();}
            return 0;
        }
    }
}
