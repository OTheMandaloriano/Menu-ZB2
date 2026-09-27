using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace Zb2Launcher {
    public sealed class Surface : Panel {
        public Surface(){DoubleBuffered=true;BackColor=Color.FromArgb(31,32,39);Padding=new Padding(24);}
        protected override void OnPaint(PaintEventArgs e){base.OnPaint(e);using(var pen=new Pen(Color.FromArgb(48,50,61)))e.Graphics.DrawRectangle(pen,0,0,Width-1,Height-1);}
    }
    public partial class Launcher {
        static readonly Color Accent=Color.FromArgb(224,67,101),Muted=Color.FromArgb(155,159,177),Ink=Color.FromArgb(240,241,247);
        readonly Label versionLabel=new Label();
        readonly Label gameLabel=new Label();
        readonly ProgressBar progress=new ProgressBar{Style=ProgressBarStyle.Marquee,MarqueeAnimationSpeed=25,Visible=false,Height=3,Dock=DockStyle.Bottom};
        readonly Panel pages=new Panel{Dock=DockStyle.Fill};
        readonly System.Windows.Forms.Timer gameTimer=new System.Windows.Forms.Timer{Interval=1500};
        readonly FlowLayoutPanel navigation=new FlowLayoutPanel{FlowDirection=FlowDirection.TopDown,WrapContents=false,Dock=DockStyle.Left,Width=184,Padding=new Padding(16,22,12,0)};
        Control[] sections;
        void BuildInterface(bool preview) {
            Text="ZB2Menu | Launcher";ClientSize=new Size(920,620);MinimumSize=new Size(860,600);
            AutoScaleMode=AutoScaleMode.Dpi;Font=new Font("Segoe UI",10);BackColor=Color.FromArgb(18,19,24);ForeColor=Ink;
            FormBorderStyle=FormBorderStyle.Sizable;MaximizeBox=false;DoubleBuffered=true;
            var header=new Panel{Dock=DockStyle.Top,Height=84,Padding=new Padding(28,18,24,12),BackColor=Color.FromArgb(23,24,30)};
            header.Controls.Add(TextLabel("ZB2",22,Accent,DockStyle.Left,100));
            var title=TextLabel("MENU LAUNCHER\nSeu menu. Seus controles.",10,Muted,DockStyle.Left,310);header.Controls.Add(title);title.BringToFront();
            versionLabel.Text="PRÉVIA 0.2.1";versionLabel.ForeColor=Muted;versionLabel.AutoSize=false;versionLabel.Dock=DockStyle.Right;versionLabel.Width=150;versionLabel.TextAlign=ContentAlignment.MiddleRight;header.Controls.Add(versionLabel);
            var footer=new Panel{Dock=DockStyle.Bottom,Height=76,Padding=new Padding(24,12,24,12),BackColor=Color.FromArgb(23,24,30)};
            status.Dock=DockStyle.Fill;status.ForeColor=Muted;status.Font=new Font("Segoe UI",9);footer.Controls.Add(status);footer.Controls.Add(progress);
            panel.Dock=DockStyle.Fill;panel.BackColor=BackColor;panel.Controls.Add(pages);panel.Controls.Add(navigation);
            Controls.Add(panel);Controls.Add(footer);Controls.Add(header);
            sections=new Control[]{HomePage(),UpdatesPage(),PreferencesPage()};
            for(int i=0;i<sections.Length;++i){sections[i].Dock=DockStyle.Fill;pages.Controls.Add(sections[i]);}
            string[] titles={"01   Início","02   Atualizações","03   Preferências"};
            for(int i=0;i<titles.Length;++i){int index=i;var button=Button(titles[i],false);button.Width=154;button.Margin=new Padding(0,0,0,10);button.Click+=(s,e)=>SelectPage(index);navigation.Controls.Add(button);}
            navigation.Controls.Add(new Label{Text="CANAL PRIVADO\n\nModo local disponível\nsem conexão.",ForeColor=Muted,AutoSize=false,Width=150,Height=120,Margin=new Padding(6,30,0,0),Font=new Font("Segoe UI",9)});
            SelectPage(0);RefreshGameState();
            if(!preview){gameTimer.Tick+=(s,e)=>RefreshGameState();gameTimer.Start();}
            FormClosed+=(s,e)=>gameTimer.Dispose();
        }
        static Label TextLabel(string text,int size,Color color,DockStyle dock=DockStyle.None,int width=0){return new Label{Text=text,Font=new Font("Segoe UI",size,size>=20?FontStyle.Bold:FontStyle.Regular),ForeColor=color,Dock=dock,Width=width,AutoSize=false,TextAlign=ContentAlignment.MiddleLeft};}
        static Button Button(string text,bool primary){var b=new Button{Text=text,Height=43,Width=260,FlatStyle=FlatStyle.Flat,BackColor=primary?Accent:Color.FromArgb(42,44,55),ForeColor=Ink,Cursor=Cursors.Hand,Font=new Font("Segoe UI",10,FontStyle.Bold),Margin=new Padding(0,6,0,6)};b.FlatAppearance.BorderSize=0;b.FlatAppearance.MouseOverBackColor=primary?Color.FromArgb(240,84,120):Color.FromArgb(55,58,72);return b;}
        static FlowLayoutPanel Column(){return new FlowLayoutPanel{Dock=DockStyle.Fill,FlowDirection=FlowDirection.TopDown,WrapContents=false,AutoScroll=true,Padding=new Padding(20)};}
        static void Heading(FlowLayoutPanel parent,string title,string subtitle){parent.Controls.Add(new Label{Text=title,Font=new Font("Segoe UI",23,FontStyle.Bold),AutoSize=true,Margin=new Padding(0,0,0,8)});parent.Controls.Add(new Label{Text=subtitle,ForeColor=Muted,AutoSize=false,Width=600,Height=42,Margin=new Padding(0,0,0,14)});}
        Button ActionButton(string title,Func<Task> action,bool primary=false){var b=Button(title,primary);b.Click+=async(s,e)=>await Run(action);return b;}
        Control HomePage() {
            var page=Column();Heading(page,"Pronto para entrar?","Verifique o jogo e carregue a versão instalada. Sem downloads obrigatórios.");
            var card=new Surface{Size=new Size(620,242),Margin=new Padding(0,0,0,16)};
            var title=TextLabel("ZUMBI BLOCKS 2",20,Ink);title.SetBounds(24,20,560,38);card.Controls.Add(title);
            var subtitle=TextLabel("MENU ZB2  /  UNITY MONO  /  DIRECTX 11",9,Muted);subtitle.SetBounds(26,64,550,24);card.Controls.Add(subtitle);
            gameLabel.SetBounds(26,101,540,27);gameLabel.ForeColor=Accent;card.Controls.Add(gameLabel);
            inMap.Text="Já entrei no mapa do jogo";inMap.SetBounds(26,137,540,28);inMap.AutoSize=true;inMap.ForeColor=Ink;card.Controls.Add(inMap);
            var load=ActionButton("Carregar menu",Inject,true);load.SetBounds(24,181,270,42);card.Controls.Add(load);
            page.Controls.Add(card);
            page.Controls.Add(new Label{Text="CONTROLES NO JOGO",ForeColor=Accent,AutoSize=true,Margin=new Padding(0,0,0,8)});
            page.Controls.Add(new Label{Text="Insert   Menu     ·     F6   NoClip     ·     H   Magnet\nOs atalhos podem ser alterados e salvos dentro do menu.",ForeColor=Muted,AutoSize=false,Width=610,Height=52});
            return page;
        }
        Control UpdatesPage() {
            var page=Column();Heading(page,"Atualizações","Escolha quando atualizar. Seus presets e logs são preservados.");
            automatic.Width=590;automatic.ForeColor=Ink;automatic.Margin=new Padding(0,0,0,18);page.Controls.Add(automatic);
            page.Controls.Add(ActionButton("Verificar atualização",()=>UpdateRuntime(false),true));
            page.Controls.Add(ActionButton("Importar pacote local (.zip)",ImportLocal));
            page.Controls.Add(new Label{Text="Feche o jogo antes de instalar uma versão.\nO executável do launcher é atualizado manualmente.\n\nCanal: "+GitHubUpdates.Repository,ForeColor=Muted,Width=595,Height=115,Margin=new Padding(0,20,0,0)});
            return page;
        }
        Control PreferencesPage() {
            var page=Column();Heading(page,"Preferências","Acesso ao canal privado e arquivos locais do menu.");
            page.Controls.Add(new Label{Text="CREDENCIAL GITHUB",ForeColor=Accent,AutoSize=true,Margin=new Padding(0,0,0,8)});
            page.Controls.Add(new Label{Text="Opcional para modo local. Para baixar do canal privado, use seu\npróprio token com acesso ao repositório e permissão Contents: read.",ForeColor=Muted,Width=600,Height=48});
            token.Width=575;token.BorderStyle=BorderStyle.FixedSingle;token.BackColor=Color.FromArgb(38,40,49);token.ForeColor=Ink;page.Controls.Add(token);
            remember.ForeColor=Ink;remember.Margin=new Padding(0,12,0,12);page.Controls.Add(remember);
            page.Controls.Add(ActionButton("Salvar preferências",()=>{SavePreferences();status.Text="Preferências salvas.";return Task.FromResult(0);}));
            page.Controls.Add(ActionButton("Abrir pasta do menu",()=>{System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo(root){UseShellExecute=true});return Task.FromResult(0);}));
            return page;
        }
        void SelectPage(int index){for(int i=0;i<sections.Length;++i)sections[i].Visible=i==index;foreach(Control c in navigation.Controls)if(c is Button)c.BackColor=Color.FromArgb(42,44,55);navigation.Controls[index].BackColor=Accent;}
        void RefreshGameState(){gameLabel.Text=GameRunning()?"●  Jogo aberto, entre no mapa antes de carregar":"○  Aguardando Zumbi Blocks 2";}
        static void PreparePreview(Control control){var handle=control.Handle;control.PerformLayout();foreach(Control child in control.Controls)PreparePreview(child);}
        public void SavePreview(string path){
            for(int page=0;page<3;++page){SelectPage(page);PreparePreview(this);using(var bitmap=new Bitmap(Width,Height)){
                DrawToBitmap(bitmap,new Rectangle(Point.Empty,Size));
                string file=page==0?path:System.IO.Path.Combine(System.IO.Path.GetDirectoryName(path),System.IO.Path.GetFileNameWithoutExtension(path)+"-"+page+".png");
                bitmap.Save(file,System.Drawing.Imaging.ImageFormat.Png);
            }}
        }
    }
}
