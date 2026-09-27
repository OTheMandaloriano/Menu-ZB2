using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Web.Script.Serialization;
using System.Windows.Forms;

namespace Zb2Launcher {
    public sealed class Preferences {public bool CheckOnStart;}
    public partial class Launcher : Form {
        readonly string root=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments),"ZB2Menu");
        readonly TextBox token=new TextBox{UseSystemPasswordChar=true,Width=470};
        readonly CheckBox remember=new CheckBox{Text="Salvar credencial protegida neste usuario Windows",AutoSize=true};
        readonly CheckBox automatic=new CheckBox{Text="Consultar e instalar atualizacoes ao abrir (opcional)",AutoSize=true};
        readonly CheckBox inMap=new CheckBox{Text="Estou dentro do mapa do jogo",AutoSize=true};
        readonly Label status=new Label{AutoSize=false,Width=480,Height=75};
        readonly Panel panel=new Panel{Dock=DockStyle.Fill};
        readonly PackageStore store;
        bool busy;
        string TokenPath {get{return Path.Combine(root,"configs","github.token.enc");}}
        string PrefPath {get{return Path.Combine(root,"configs","launcher.json");}}
        static bool GameRunning(){return Process.GetProcessesByName("ZumbiBlocks2").Length!=0;}
        public Launcher(bool preview=false) {
            if(!preview) {
                store=new PackageStore(root,GameRunning);
                foreach(string dir in new[]{"configs","logs","licenses"})Directory.CreateDirectory(Path.Combine(root,dir));
                MigratePresets();
            }
            BuildInterface(preview);
            if(preview){status.Text="Versão local 0.2.1 · Modo local · Prévia visual sem conexão";return;}
            LoadPreferences();
            automatic.CheckedChanged+=(s,e)=>SavePreferences();
            remember.CheckedChanged+=(s,e)=>SavePreferences();
            Shown+=async(s,e)=>await Run(async()=>{
                using(var stream=Assembly.GetExecutingAssembly().GetManifestResourceStream("runtime.zip"))using(var output=new MemoryStream()) {
                    if(stream==null)throw new InvalidDataException("Pacote local ausente");stream.CopyTo(output);
                    var bytes=output.ToArray();var bundled=store.Inspect(bytes);var current=store.Current();
                    if(current==null || PackageStore.ParseVersion(current.version)<PackageStore.ParseVersion(bundled.version))store.Install(bytes);
                }
                ShowVersion();if(automatic.Checked)await UpdateRuntime(true);
            });
        }
        async Task Run(Func<Task> action) {
            if(busy)return;busy=true;panel.Enabled=false;progress.Visible=true;
            try{await action();}
            catch(Exception ex){status.Text=ex.Message;File.AppendAllText(Path.Combine(root,"logs","launcher.log"),DateTime.UtcNow.ToString("o")+" "+ex.GetType().Name+Environment.NewLine);}
            finally{panel.Enabled=true;progress.Visible=false;busy=false;RefreshGameState();}
        }
        void ShowVersion(){var current=store.Current();versionLabel.Text=current==null?"SEM PACOTE":"VERSÃO "+current.version;status.Text=current==null?"Nenhum pacote instalado.":"Versao local: "+current.version+". Pronta para teste no mapa.";}
        async Task UpdateRuntime(bool consent) {
            SavePreferences();status.Text="Consultando canal privado...";
            var current=store.Current();var updates=new GitHubUpdates();
            var asset=await updates.Check(token.Text,current==null?new Version(0,0,0):PackageStore.ParseVersion(current.version));
            if(asset==null){status.Text="Nenhuma versao mais recente disponivel.";return;}
            if(!consent && MessageBox.Show(this,"Baixar e instalar a nova versao do canal privado?","Atualizar",MessageBoxButtons.YesNo)!=DialogResult.Yes)return;
            status.Text="Baixando e verificando pacote...";
            var bytes=await updates.Download(asset,token.Text);var installed=store.Install(bytes,asset.digest.Substring(7));
            File.Copy(Path.Combine(store.VersionDirectory(installed.version),"Harmony.LICENSE"),Path.Combine(root,"licenses","Harmony.LICENSE"),true);
            ShowVersion();
        }
        Task ImportLocal() {
            using(var picker=new OpenFileDialog{Filter="Pacote ZB2Menu (*.zip)|*.zip"})if(picker.ShowDialog(this)==DialogResult.OK){store.Install(File.ReadAllBytes(picker.FileName));ShowVersion();}
            return Task.FromResult(0);
        }
        async Task Inject() {
            if(!inMap.Checked)throw new InvalidOperationException("Entre no mapa e confirme a opcao acima.");
            var current=store.Current();if(current==null)throw new InvalidOperationException("Instale um pacote primeiro.");
            var games=Process.GetProcessesByName("ZumbiBlocks2");if(games.Length!=1)throw new InvalidOperationException("Abra uma unica instancia do jogo.");
            using(var game=games[0]) {
                if(game.Modules.Cast<ProcessModule>().Any(m=>m.ModuleName.Equals("kiero-dx11-base.dll",StringComparison.OrdinalIgnoreCase)))throw new InvalidOperationException("Menu ja carregado. Reinicie o jogo para trocar de versao.");
                string assembly=Path.Combine(Path.GetDirectoryName(game.MainModule.FileName),"ZumbiBlocks2_Data","Managed","Assembly-CSharp.dll");
                if(!File.Exists(assembly) || PackageStore.Hash(File.ReadAllBytes(assembly))!=current.game_sha256)throw new InvalidOperationException("Versao do jogo nao compativel com este pacote.");
            }
            string dir=store.VersionDirectory(current.version);status.Text="Carregando DLL...";
            int code=await Task.Run(()=>{using(var process=Process.Start(new ProcessStartInfo(Path.Combine(dir,"injector.exe"),"--nowait"){WorkingDirectory=dir,UseShellExecute=false,CreateNoWindow=true})){if(!process.WaitForExit(60000))throw new TimeoutException("Injetor ainda em execucao; consulte o log.");return process.ExitCode;}});
            if(code!=0)throw new InvalidOperationException("Injetor retornou codigo "+code+". Consulte os logs.");
            status.Text="Injetor concluido. Confira o log do menu para confirmar a inicializacao das funcoes.";
        }
        void MigratePresets() {
            string old=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments),"kiero-dx11-base","configs");
            if(!Directory.Exists(old))return;
            foreach(string file in Directory.GetFiles(old,"*.json")){string dest=Path.Combine(root,"configs",Path.GetFileName(file));if(!File.Exists(dest))File.Copy(file,dest);}
        }
        void LoadPreferences() {
            if(File.Exists(PrefPath))try{automatic.Checked=new JavaScriptSerializer().Deserialize<Preferences>(File.ReadAllText(PrefPath)).CheckOnStart;}catch{status.Text="Preferencias invalidas; modo local mantido.";}
            if(File.Exists(TokenPath))try{token.Text=Encoding.UTF8.GetString(ProtectedData.Unprotect(File.ReadAllBytes(TokenPath),null,DataProtectionScope.CurrentUser));remember.Checked=true;}catch{status.Text="Credencial nao disponivel neste usuario Windows.";}
        }
        void SavePreferences() {
            File.WriteAllText(PrefPath,new JavaScriptSerializer().Serialize(new Preferences{CheckOnStart=automatic.Checked}));
            if(remember.Checked && token.Text.Length>0)File.WriteAllBytes(TokenPath,ProtectedData.Protect(Encoding.UTF8.GetBytes(token.Text.Trim()),null,DataProtectionScope.CurrentUser));
            else if(File.Exists(TokenPath))File.Delete(TokenPath);
        }
        [STAThread] static void Main(string[] args) {
            if(args.Length==2 && args[0]=="--preview"){Application.EnableVisualStyles();using(var view=new Launcher(true))view.SavePreview(args[1]);return;}
            bool created;
            using(var mutex=new Mutex(true,"Local\\ZB2MenuLauncher",out created)) {
                if(!created)return;
                Application.EnableVisualStyles();Application.SetCompatibleTextRenderingDefault(false);
                try{Application.Run(new Launcher());}catch(Exception ex){MessageBox.Show(ex.Message,"ZB2Menu");}
            }
        }
    }
}
