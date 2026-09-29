using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Text;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Web.Script.Serialization;
using System.Windows.Forms;

namespace Zb2Admin
{
    internal static class Design
    {
        internal static readonly Color Canvas = Color.FromArgb(25, 26, 29);
        internal static readonly Color Surface = Color.FromArgb(32, 33, 37);
        internal static readonly Color Input = Color.FromArgb(22, 23, 26);
        internal static readonly Color Line = Color.FromArgb(57, 59, 65);
        internal static readonly Color Text = Color.FromArgb(237, 238, 241);
        internal static readonly Color Muted = Color.FromArgb(165, 170, 181);
        internal static readonly Color Green = Color.FromArgb(132, 202, 164);
        internal static readonly Color Error = Color.FromArgb(244, 159, 155);
        internal static readonly PrivateFontCollection Fonts = new PrivateFontCollection();
        private static IntPtr fontMemory;
        [DllImport("gdi32.dll")]
        private static extern IntPtr AddFontMemResourceEx(IntPtr data, uint size, IntPtr reserved, ref uint count);
        internal static void LoadFont()
        {
            using (Stream stream = Assembly.GetExecutingAssembly().GetManifestResourceStream("Admin.Font"))
            {
                byte[] bytes = new byte[stream.Length]; stream.Read(bytes, 0, bytes.Length);
                fontMemory = Marshal.AllocCoTaskMem(bytes.Length); Marshal.Copy(bytes, 0, fontMemory, bytes.Length);
                Fonts.AddMemoryFont(fontMemory, bytes.Length);
                uint count = 0; AddFontMemResourceEx(fontMemory, (uint)bytes.Length, IntPtr.Zero, ref count);
            }
        }
        internal static Font Face(float size) { return new Font(Fonts.Families[0], size, FontStyle.Regular, GraphicsUnit.Point); }
        internal static GraphicsPath Rounded(Rectangle rectangle, int radius)
        {
            GraphicsPath path = new GraphicsPath(); int diameter = radius * 2;
            path.AddArc(rectangle.Left, rectangle.Top, diameter, diameter, 180, 90);
            path.AddArc(rectangle.Right - diameter, rectangle.Top, diameter, diameter, 270, 90);
            path.AddArc(rectangle.Right - diameter, rectangle.Bottom - diameter, diameter, diameter, 0, 90);
            path.AddArc(rectangle.Left, rectangle.Bottom - diameter, diameter, diameter, 90, 90);
            path.CloseFigure(); return path;
        }
    }

    internal sealed class ActionButton : Button
    {
        internal bool Primary, Selected, Chevron;
        private bool hover, pressed;
        internal ActionButton(string text, bool primary)
        {
            Text = text; Primary = primary; Font = Design.Face(10); Height = 42;
            FlatStyle = FlatStyle.Flat; FlatAppearance.BorderSize = 0;
            Cursor = Cursors.Hand; Margin = new Padding(0, 0, 8, 0);
            SetStyle(ControlStyles.UserPaint | ControlStyles.AllPaintingInWmPaint | ControlStyles.OptimizedDoubleBuffer, true);
        }
        protected override void OnMouseEnter(EventArgs e) { hover = true; Invalidate(); base.OnMouseEnter(e); }
        protected override void OnMouseLeave(EventArgs e) { hover = pressed = false; Invalidate(); base.OnMouseLeave(e); }
        protected override void OnMouseDown(MouseEventArgs e) { pressed = true; Invalidate(); base.OnMouseDown(e); }
        protected override void OnMouseUp(MouseEventArgs e) { pressed = false; Invalidate(); base.OnMouseUp(e); }
        protected override void OnPaint(PaintEventArgs e)
        {
            e.Graphics.SmoothingMode = SmoothingMode.AntiAlias;
            Color background = Primary ? Color.FromArgb(225, 227, 231) : Selected || hover ? Color.FromArgb(47, 49, 55) : Design.Surface;
            if (pressed) background = Primary ? Color.FromArgb(195, 199, 207) : Color.FromArgb(62, 65, 72);
            if (!Enabled) background = Design.Surface;
            using (GraphicsPath path = Design.Rounded(new Rectangle(0, 0, Width - 1, Height - 1), 5))
            using (Brush brush = new SolidBrush(background)) e.Graphics.FillPath(brush, path);
            Color foreground = !Enabled ? Color.FromArgb(106, 111, 121) : Primary ? Color.FromArgb(25, 26, 29) : Design.Text;
            TextRenderer.DrawText(e.Graphics, Text, Font, ClientRectangle, foreground, TextFormatFlags.HorizontalCenter | TextFormatFlags.VerticalCenter | TextFormatFlags.SingleLine | TextFormatFlags.NoPrefix);
            if(Chevron)using(Pen pen=new Pen(foreground,1.4f))e.Graphics.DrawLines(pen,new[] {new Point(Width-20,Height/2-2),new Point(Width-16,Height/2+2),new Point(Width-12,Height/2-2)});
            if (Focused && ShowFocusCues) ControlPaint.DrawFocusRectangle(e.Graphics, Rectangle.Inflate(ClientRectangle, -5, -5), foreground, background);
        }
    }

    internal sealed class DarkMenuRenderer : ToolStripProfessionalRenderer
    {
        protected override void OnRenderMenuItemBackground(ToolStripItemRenderEventArgs e)
        {
            using(Brush brush=new SolidBrush(e.Item.Selected?Color.FromArgb(54,58,66):Design.Surface))e.Graphics.FillRectangle(brush,new Rectangle(Point.Empty,e.Item.Size));
        }
        protected override void OnRenderItemText(ToolStripItemTextRenderEventArgs e){e.TextColor=Design.Text;base.OnRenderItemText(e);}
        protected override void OnRenderToolStripBorder(ToolStripRenderEventArgs e){using(Pen pen=new Pen(Design.Line))e.Graphics.DrawRectangle(pen,0,0,e.ToolStrip.Width-1,e.ToolStrip.Height-1);}
    }

    internal sealed class TextInput : Panel
    {
        internal readonly TextBox Editor = new TextBox();
        internal TextInput()
        {
            Height = 40; BackColor = Design.Input; Padding = new Padding(12, 0, 12, 0);
            Editor.BorderStyle = BorderStyle.None; Editor.BackColor = Design.Input; Editor.ForeColor = Design.Text;
            Editor.Font = Design.Face(10); Editor.Anchor = AnchorStyles.Left | AnchorStyles.Right | AnchorStyles.Top;
            Controls.Add(Editor); TabStop = false; Resize += delegate { Position(); };
        }
        private void Position() { Editor.SetBounds(12, (Height - Editor.PreferredHeight) / 2, Math.Max(20, Width - 24), Editor.PreferredHeight); }
        protected override void OnPaint(PaintEventArgs e)
        {
            using (Pen pen = new Pen(Design.Line)) e.Graphics.DrawRectangle(pen, 0, 0, Width - 1, Height - 1);
            base.OnPaint(e);
        }
    }

    internal sealed class AdminForm : Form
    {
        private readonly AdminStore store;
        private readonly Panel body = new Panel();
        private readonly Label feedback = new Label();
        private readonly Label role = new Label();
        private readonly List<ActionButton> navigation = new List<ActionButton>();
        private bool busy, closePending;
        private string page = "Licenças";
        private TextInput customer, device, search;
        private NumericUpDown days;
        private Label preview;
        private DataGridView history;
        private List<LicenseRecord> records = new List<LicenseRecord>();
        private LicenseRecord selected;
        private ActionButton copy, save;
        private Request selectedRequest;

        internal AdminForm(AdminStore data)
        {
            store = data; Text = "ZB2 Admin — Licenças e equipe";
            if(String.IsNullOrEmpty(store.Current.ProtectedKey) || (store.Current.Role=="operator" && String.IsNullOrEmpty(store.Current.Certificate)))page="Esta estação";
            Font = Design.Face(10); BackColor = Design.Canvas; ForeColor = Design.Text;
            ClientSize = new Size(1040, 700); MinimumSize = new Size(980, 740);
            StartPosition = FormStartPosition.CenterScreen; AutoScaleMode = AutoScaleMode.Dpi;
            DoubleBuffered = true;
            Label title = Label("ZB2 Admin", 22, Design.Text); title.SetBounds(32, 24, 400, 44); Controls.Add(title);
            Label subtitle = Label("Licenças e equipe, em um só lugar.", 10, Design.Muted); subtitle.SetBounds(34, 76, 600, 28); Controls.Add(subtitle);
            role.TextAlign = ContentAlignment.MiddleRight; role.ForeColor = Design.Green; role.Font = Design.Face(10);
            role.SetBounds(640, 40, 368, 32); role.Anchor = AnchorStyles.Right | AnchorStyles.Top; Controls.Add(role);
            int x = 32;
            foreach (string name in new[] { "Licenças", "Equipe", "Esta estação" })
            {
                string tab = name;
                ActionButton button = new ActionButton(name, false); button.SetBounds(x, 120, 152, 40);
                button.Click += delegate { if (!busy) { page = tab; ShowPage(); } };
                navigation.Add(button); Controls.Add(button); x += 164;
            }
            ActionButton help = new ActionButton("Como usar", false); help.SetBounds(856, 120, 152, 40); help.Anchor = AnchorStyles.Top | AnchorStyles.Right;
            help.Click += delegate { MessageBox.Show(this,
                "CLIENTES\n1. Peça o ID exibido no loader do cliente.\n2. Informe o nome, cole o ID e escolha o prazo.\n3. Gere a licença e envie o arquivo salvo ao cliente.\n\nEQUIPE\n1. No PC do integrante, abra Esta estação e crie uma solicitação.\n2. O proprietário abre a solicitação em Equipe e escolhe os limites.\n3. O integrante importa a autorização recebida.\n\nCada PC mantém seu histórico local. Não há sincronização ou revogação instantânea offline.", "Como usar o ZB2 Admin", MessageBoxButtons.OK, MessageBoxIcon.Information); };
            Controls.Add(help);
            body.SetBounds(32, 184, 976, 448); body.Anchor = AnchorStyles.Left | AnchorStyles.Right | AnchorStyles.Top | AnchorStyles.Bottom; Controls.Add(body);
            feedback.SetBounds(32, 648, 792, 32); feedback.Anchor = AnchorStyles.Left | AnchorStyles.Right | AnchorStyles.Bottom;feedback.AutoEllipsis=true;
            feedback.ForeColor = Design.Muted; feedback.Font = Design.Face(9); feedback.Text = "Dados locais nesta estação. O aplicativo do cliente é o ZB2 Menu."; Controls.Add(feedback);
            ActionButton legal=new ActionButton("Licenças de terceiros",false);legal.SetBounds(836,648,172,32);legal.Font=Design.Face(8);legal.Anchor=AnchorStyles.Right|AnchorStyles.Bottom;
            legal.Click+=delegate { using(Form dialog=new Form { Text="Licenças de terceiros",Size=new Size(680,520),StartPosition=FormStartPosition.CenterParent,BackColor=Design.Canvas })
                using(Stream stream=Assembly.GetExecutingAssembly().GetManifestResourceStream("Admin.OFL"))
                using(StreamReader reader=new StreamReader(stream)) {
                    TextBox content=new TextBox { Dock=DockStyle.Fill,Multiline=true,ReadOnly=true,ScrollBars=ScrollBars.Vertical,BackColor=Design.Canvas,ForeColor=Design.Text,Font=Design.Face(10),Text=reader.ReadToEnd() };
                    dialog.Controls.Add(content);dialog.ShowDialog(this);
                }
            };Controls.Add(legal);
            FormClosing += delegate(object sender, FormClosingEventArgs e) { if (busy) { closePending = true; e.Cancel = true; Notice("Finalizando a operação antes de fechar...", false); } };
            Shown += delegate { ShowPage(); };
            ShowPage();
        }

        private static Label Label(string text, float size, Color color)
        {
            return new Label { Text = text, Font = Design.Face(size), ForeColor = color, AutoSize = false, BackColor = Color.Transparent, TextAlign = ContentAlignment.MiddleLeft };
        }
        private static TableLayoutPanel Columns(float first)
        {
            TableLayoutPanel layout = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 2, RowCount = 1, Padding = Padding.Empty, Margin = Padding.Empty };
            layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, first)); layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100 - first));
            layout.RowStyles.Add(new RowStyle(SizeType.Percent, 100)); return layout;
        }
        private static Panel Pane() { return new Panel { Dock = DockStyle.Fill, Margin = new Padding(0, 0, 24, 0) }; }
        private static Label AddLabel(Control parent, string text, int y, int height, float size, Color color)
        {
            Label value = Label(text, size, color); value.SetBounds(0, y, parent.Width, Math.Max(height,value.Font.Height+4)); value.Anchor = AnchorStyles.Left | AnchorStyles.Right | AnchorStyles.Top; parent.Controls.Add(value); return value;
        }
        private static TextInput AddInput(Control parent, int y, int maxLength)
        {
            TextInput input = new TextInput(); input.SetBounds(0, y, parent.Width, 40); input.Anchor = AnchorStyles.Left | AnchorStyles.Right | AnchorStyles.Top;
            input.Editor.MaxLength = maxLength; parent.Controls.Add(input); return input;
        }
        private static NumericUpDown Number(Control parent, int y, int value, int maximum)
        {
            NumericUpDown input = new NumericUpDown { Minimum = 1, Maximum = maximum, Value = value, Font = Design.Face(11), BackColor = Design.Input, ForeColor = Design.Text, BorderStyle = BorderStyle.FixedSingle };
            input.SetBounds(0, y, 104, 32); parent.Controls.Add(input); return input;
        }
        private void Notice(string text, bool error) { feedback.Text = text; feedback.ForeColor = error ? Design.Error : Design.Muted; }
        private async void Work<T>(Func<T> task, Action<T> complete)
        {
            if (busy) return;
            busy = true; body.Enabled = false; foreach (ActionButton item in navigation) item.Enabled = false;
            Notice("Processando...", false); UseWaitCursor = true;
            try { T result = await Task.Run(task); UseWaitCursor = false; complete(result); }
            catch (Exception error) { Notice(error.Message, true); }
            finally { busy = false; body.Enabled = true; foreach (ActionButton item in navigation) item.Enabled = true; UseWaitCursor = false; if (closePending) Close(); }
        }
        private string OpenFile(string title, string filter)
        {
            using (OpenFileDialog dialog = new OpenFileDialog { Title = title, Filter = filter, CheckFileExists = true })
                return dialog.ShowDialog(this) == DialogResult.OK ? dialog.FileName : null;
        }
        private void SaveText(string text, string filename, string filter)
        {
            using (SaveFileDialog dialog = new SaveFileDialog { FileName = filename, Filter = filter, OverwritePrompt = true, AddExtension = true })
            {
                if (dialog.ShowDialog(this) != DialogResult.OK) return;
                try { AdminStore.AtomicWrite(dialog.FileName, text); Notice("Arquivo salvo. Envie-o à pessoa correspondente.", false); }
                catch (Exception error) { Notice(error.Message, true); }
            }
        }
        private void ShowPage()
        {
            foreach (ActionButton tab in navigation) { tab.Selected = tab.Text == page; tab.Invalidate(); }
            foreach (Control child in body.Controls.Cast<Control>().ToArray()) child.Dispose();
            body.Controls.Clear();
            role.Text = String.IsNullOrEmpty(store.Current.ProtectedKey) ? "Estação não configurada" : store.Current.Name + " · " + (store.Current.Role == "owner" ? "Proprietário" : "Integrante");
            if (page == "Licenças") BuildLicenses(); else if (page == "Equipe") BuildTeam(); else BuildStation();
        }

        private void BuildLicenses()
        {
            TableLayoutPanel columns = Columns(42); body.Controls.Add(columns);
            Panel left = Pane(), right = Pane(); right.Margin = Padding.Empty;
            columns.Controls.Add(left, 0, 0); columns.Controls.Add(right, 1, 0); columns.PerformLayout();
            AddLabel(left, "Nova licença", 0, 28, 14, Design.Text);
            AddLabel(left, "Defina quem vai usar e por quanto tempo.", 32, 28, 9, Design.Muted);
            AddLabel(left, "Nome do cliente", 76, 22, 9, Design.Muted); customer = AddInput(left, 104, 80);
            AddLabel(left, "ID do computador do cliente", 156, 22, 9, Design.Muted); device = AddInput(left, 184, 64);
            AddLabel(left, "Prazo de uso", 236, 22, 9, Design.Muted);
            days = Number(left, 264, 30, 3650);
            ActionButton presets = new ActionButton("Escolher prazo",false);presets.Chevron=true;
            presets.SetBounds(116, 264, Math.Max(120, left.Width-116), 32); presets.Anchor = AnchorStyles.Left | AnchorStyles.Right | AnchorStyles.Top;
            left.Controls.Add(presets);
            ContextMenuStrip menu=new ContextMenuStrip { Renderer=new DarkMenuRenderer(),BackColor=Design.Surface,ForeColor=Design.Text,Font=Design.Face(10),ShowImageMargin=false };
            foreach(int amount in new[] {7,15,30,90,180,365}){int choice=amount;ToolStripItem item=menu.Items.Add(choice+" dias",null,delegate {days.Value=choice;});item.Tag=choice;}
            menu.Items.Add("Personalizado",null,delegate {days.Focus();days.Select(0,days.Text.Length);});
            presets.Click+=delegate {menu.Show(presets,new Point(0,presets.Height));};presets.Disposed+=delegate {menu.Dispose();};
            preview = AddLabel(left, "", 304, 28, 9, Design.Muted);
            days.ValueChanged += delegate { UpdatePreview(); }; UpdatePreview();
            ActionButton generate = new ActionButton("Gerar licença", true); generate.SetBounds(0, 344, left.Width, 44); generate.Anchor = AnchorStyles.Left | AnchorStyles.Right | AnchorStyles.Top; left.Controls.Add(generate);
            generate.Click += delegate
            {
                string name = customer.Editor.Text, id = device.Editor.Text; int amount = (int)days.Value;
                Work(delegate { return store.Issue(name, id, amount); }, delegate(LicenseRecord record)
                {
                    RefreshHistory(); history.ClearSelection();
                    foreach (DataGridViewRow row in history.Rows) if (((LicenseRecord)row.DataBoundItem).Id == record.Id) row.Selected = true;
                    selected = record; copy.Enabled = save.Enabled = true;
                    Notice("Licença gerada para " + record.Customer + ". Copie ou salve o arquivo para enviar ao cliente.", false);
                });
            };
            Label terms = AddLabel(left, "O prazo começa na emissão, não na primeira ativação.", 400, 48, 9, Design.Muted);
            AddLabel(right, "Histórico desta estação", 0, 28, 14, Design.Text);
            search = AddInput(right, 48, 120); search.Editor.TextChanged += delegate { FilterHistory(); };
            AddLabel(right, "Buscar por cliente, ID ou integrante", 92, 24, 9, Design.Muted);
            history = new DataGridView { ReadOnly = true, AllowUserToAddRows = false, AllowUserToDeleteRows = false, AllowUserToResizeRows = false, RowHeadersVisible = false, MultiSelect = false, SelectionMode = DataGridViewSelectionMode.FullRowSelect, AutoGenerateColumns = false,
                BackgroundColor = Design.Canvas, BorderStyle = BorderStyle.None, GridColor = Design.Line, EnableHeadersVisualStyles = false, AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode.Fill };
            history.SetBounds(0, 128, right.Width, Math.Max(140, right.Height-196)); history.Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right;
            history.DefaultCellStyle.BackColor = Design.Surface; history.DefaultCellStyle.ForeColor = Design.Text;
            history.DefaultCellStyle.SelectionBackColor = Color.FromArgb(54, 58, 66); history.DefaultCellStyle.SelectionForeColor = Design.Text;
            history.DefaultCellStyle.Font = Design.Face(9); history.DefaultCellStyle.Padding = new Padding(8, 0, 0, 0); history.RowTemplate.Height = 40;
            history.ColumnHeadersDefaultCellStyle.BackColor = Design.Canvas; history.ColumnHeadersDefaultCellStyle.ForeColor = Design.Muted;
            history.ColumnHeadersDefaultCellStyle.Font = Design.Face(9); history.ColumnHeadersHeight = 36;
            history.ColumnHeadersBorderStyle = DataGridViewHeaderBorderStyle.None;history.CellBorderStyle = DataGridViewCellBorderStyle.SingleHorizontal;
            history.Columns.Add(new DataGridViewTextBoxColumn { DataPropertyName = "Customer", HeaderText = "Cliente", FillWeight = 40 });
            history.Columns.Add(new DataGridViewTextBoxColumn { DataPropertyName = "DateText", HeaderText = "Validade", FillWeight = 38 });
            history.Columns.Add(new DataGridViewTextBoxColumn { DataPropertyName = "Status", HeaderText = "Estado", FillWeight = 22 });
            right.Controls.Add(history);
            copy = new ActionButton("Copiar licença", false); save = new ActionButton("Salvar arquivo", false);
            copy.SetBounds(0, right.Height-48, (right.Width-12)/2, 42); save.SetBounds((right.Width+12)/2, right.Height-48, (right.Width-12)/2, 42);
            copy.Anchor = save.Anchor = AnchorStyles.Left | AnchorStyles.Bottom;
            right.Resize += delegate { copy.SetBounds(0, right.Height-48, (right.Width-12)/2, 42); save.SetBounds((right.Width+12)/2, right.Height-48, (right.Width-12)/2, 42); };
            copy.Enabled = save.Enabled = false; right.Controls.Add(copy); right.Controls.Add(save);
            copy.Click += delegate { if (selected == null) return; try { Clipboard.SetText(selected.Token); Notice("Licença copiada. Envie somente a licença ao cliente.", false); } catch (Exception error) { Notice(error.Message, true); } };
            save.Click += delegate { if (selected != null) SaveText(selected.Token + "\n", "Licenca-" + selected.Id.Substring(0, 8) + ".zb2license", "Licença ZB2|*.zb2license"); };
            history.SelectionChanged += delegate { if (history.SelectedRows.Count == 1) { selected = history.SelectedRows[0].DataBoundItem as LicenseRecord; copy.Enabled = save.Enabled = selected != null; } };
            try {
                RefreshHistory(); int maximum = store.MaxDays(); days.Maximum=Math.Max(1,maximum);
                foreach(ToolStripItem item in menu.Items)if(item.Tag is int)item.Enabled=(int)item.Tag<=maximum;
                generate.Enabled=presets.Enabled=maximum>0;
                terms.Text = "O prazo começa na emissão.\nLimite da estação: " + maximum + " dias por licença.";
            }
            catch (Exception error) { generate.Enabled = false; Notice(error.Message, true); }
        }
        private void UpdatePreview() { preview.Text = "Válida até " + Crypto.Date(Crypto.Now() + (long)days.Value * 86400); }
        private void RefreshHistory() { records = store.History(); FilterHistory(); }
        private void FilterHistory()
        {
            if (history == null || search == null) return;
            string query = search.Editor.Text.Trim();
            selected = null; if (copy != null) copy.Enabled = save.Enabled = false;
            history.DataSource = records.Where(r => (r.Customer + " " + r.Device + " " + r.Operator).IndexOf(query, StringComparison.OrdinalIgnoreCase) >= 0).ToList();
        }

        private void BuildTeam()
        {
            TableLayoutPanel columns = Columns(52); body.Controls.Add(columns);
            Panel pane = Pane(), right = Pane();right.Margin = Padding.Empty;
            columns.Controls.Add(pane,0,0);columns.Controls.Add(right,1,0);columns.PerformLayout();
            AddLabel(pane, "Autorizações da equipe", 0, 30, 16, Design.Text);
            AddLabel(pane, "Autorize cada PC e escolha seus limites de emissão.", 40, 44, 10, Design.Muted);
            if (store.Current.Role != "owner")
            {
                AddLabel(pane, "Somente o proprietário autoriza integrantes. Use Esta estação para enviar sua solicitação.", 100, 60, 11, Design.Text); return;
            }
            Label detail = AddLabel(pane, "1. Abra a solicitação enviada pelo integrante.", 100, 36, 11, Design.Text);
            ActionButton open = new ActionButton("Abrir solicitação", false); open.SetBounds(0, 152, 248, 44); pane.Controls.Add(open);
            AddLabel(pane, "Autorização para emitir (dias)", 220, 28, 10, Design.Muted); NumericUpDown validity = Number(pane, 256, 365, 3650);
            AddLabel(pane, "Máximo de dias por licença", 308, 28, 10, Design.Muted); NumericUpDown maximum = Number(pane, 344, 30, 3650);
            maximum.Maximum=validity.Value;validity.ValueChanged+=delegate {maximum.Maximum=validity.Value;};
            ActionButton approve = new ActionButton("Gerar autorização", true); approve.SetBounds(160, 338, Math.Max(220,pane.Width-160), 44); approve.Anchor=AnchorStyles.Left|AnchorStyles.Right|AnchorStyles.Top;approve.Enabled = false; pane.Controls.Add(approve);
            open.Click += delegate
            {
                string file = OpenFile("Solicitação do integrante", "Estação ZB2|*.zb2station"); if (file == null) return;
                try { selectedRequest = store.ParseRequest(AdminStore.ReadLimited(file)); detail.Text = "Integrante: " + selectedRequest.Name + "  ·  Estação " + selectedRequest.Id.Substring(0, 8); approve.Enabled = true; }
                catch (Exception error) { Notice(error.Message, true); }
            };
            approve.Click += delegate
            {
                Request request = selectedRequest; int span = (int)validity.Value, limit = (int)maximum.Value;
                Work(delegate { return store.Authorize(request, span, limit); }, delegate(string certificate)
                {
                    Notice("Autorização criada para " + request.Name + ". O integrante deve importá-la no próprio PC.", false);
                    SaveText(certificate + "\n", "Autorizacao-" + request.Id.Substring(0, 8) + ".zb2issuer", "Autorização ZB2|*.zb2issuer");
                    ShowPage();
                });
            };
            AddLabel(pane, "No modo offline, uma autorização vale até vencer. Não há revogação imediata.", 396, 48, 9, Design.Muted);
            AddLabel(right,"Estações autorizadas",0,30,14,Design.Text);
            DataGridView team = new DataGridView { ReadOnly=true,AllowUserToAddRows=false,AllowUserToDeleteRows=false,RowHeadersVisible=false,AutoGenerateColumns=false,
                MultiSelect=false,SelectionMode=DataGridViewSelectionMode.FullRowSelect,BackgroundColor=Design.Canvas,BorderStyle=BorderStyle.None,GridColor=Design.Line,
                EnableHeadersVisualStyles=false,AutoSizeColumnsMode=DataGridViewAutoSizeColumnsMode.Fill,CellBorderStyle=DataGridViewCellBorderStyle.SingleHorizontal,ColumnHeadersBorderStyle=DataGridViewHeaderBorderStyle.None };
            team.SetBounds(0,56,right.Width,320);team.Anchor=AnchorStyles.Top|AnchorStyles.Left|AnchorStyles.Right;
            team.DefaultCellStyle.BackColor=Design.Surface;team.DefaultCellStyle.ForeColor=Design.Text;team.DefaultCellStyle.SelectionBackColor=Color.FromArgb(54,58,66);
            team.DefaultCellStyle.SelectionForeColor=Design.Text;team.DefaultCellStyle.Font=Design.Face(9);team.RowTemplate.Height=40;
            team.ColumnHeadersDefaultCellStyle.BackColor=Design.Canvas;team.ColumnHeadersDefaultCellStyle.ForeColor=Design.Muted;team.ColumnHeadersDefaultCellStyle.Font=Design.Face(9);team.ColumnHeadersHeight=36;
            team.Columns.Add(new DataGridViewTextBoxColumn { DataPropertyName="Name",HeaderText="Integrante",FillWeight=44 });
            team.Columns.Add(new DataGridViewTextBoxColumn { DataPropertyName="DateText",HeaderText="Autorização até",FillWeight=56 });
            right.Controls.Add(team);
            ActionButton export = new ActionButton("Salvar autorização selecionada",false);export.SetBounds(0,396,right.Width,42);export.Anchor=AnchorStyles.Top|AnchorStyles.Left|AnchorStyles.Right;right.Controls.Add(export);
            try { team.DataSource=store.TeamHistory(); } catch(Exception error) { Notice(error.Message,true); }
            export.Click+=delegate { if(team.SelectedRows.Count!=1)return;Grant grant=team.SelectedRows[0].DataBoundItem as Grant;if(grant!=null)SaveText(grant.Token+"\n","Autorizacao-"+grant.Id.Substring(0,8)+".zb2issuer","Autorização ZB2|*.zb2issuer"); };
        }

        private void BuildStation()
        {
            Panel pane = Pane(); pane.Margin = Padding.Empty; body.Controls.Add(pane);
            AddLabel(pane, "Esta estação", 0, 30, 16, Design.Text);
            AddLabel(pane, "Configure quem pode emitir licenças neste usuário do Windows.", 40, 30, 10, Design.Muted);
            if (String.IsNullOrEmpty(store.Current.ProtectedKey))
            {
                AddLabel(pane, "Nome do integrante", 100, 26, 10, Design.Muted); TextInput name = AddInput(pane, 136, 60);name.Width=480;name.Anchor=AnchorStyles.Top|AnchorStyles.Left;
                ActionButton request = new ActionButton("Criar solicitação para o proprietário", true); request.SetBounds(0, 200, 420, 44); pane.Controls.Add(request);
                request.Click += delegate
                {
                    string text = name.Editor.Text;
                    Work(delegate { store.CreateStation(text); return store.ExportRequest(); }, delegate(string content)
                    {
                        SaveText(content, "Solicitacao-" + store.Current.Id.Substring(0, 8) + ".zb2station", "Estação ZB2|*.zb2station"); ShowPage();
                    });
                };
                AddLabel(pane, "Proprietário: importe a chave já existente somente no PC em que ela foi criada.", 292, 44, 10, Design.Muted);
                ActionButton owner = new ActionButton("Importar chave do proprietário", false); owner.SetBounds(0, 352, 420, 44); pane.Controls.Add(owner);
                owner.Click += delegate
                {
                    string file = OpenFile("Chave do proprietário", "Chave protegida|*.dpapi"); if (file == null) return; string text = name.Editor.Text;
                    Work(delegate { store.ImportOwner(file, text); return true; }, delegate(bool result) { ShowPage(); Notice("Proprietário configurado. Abra Licenças para começar.", false); });
                };
            }
            else if (store.Current.Role == "owner")
            {
                AddLabel(pane, "Proprietário configurado", 108, 32, 14, Design.Green);
                AddLabel(pane, store.Current.Name + " pode emitir licenças e autorizar integrantes.", 156, 40, 11, Design.Text);
                AddLabel(pane, "A chave principal fica protegida pelo Windows neste perfil. Ela nunca integra o loader do cliente.", 216, 64, 10, Design.Muted);
                AddLabel(pane, "Histórico e autorizações ficam salvos localmente. Faça backup protegido do perfil e dos dados.", 296, 64, 10, Design.Muted);
                ActionButton openData=new ActionButton("Abrir pasta de dados",false);openData.SetBounds(0,396,260,42);pane.Controls.Add(openData);
                openData.Click+=delegate { try { System.Diagnostics.Process.Start("explorer.exe","\""+store.Root+"\""); } catch(Exception error) { Notice(error.Message,true); } };
            }
            else
            {
                AddLabel(pane, "Integrante: " + store.Current.Name, 108, 32, 14, Design.Text);
                string description;
                try { Grant grant = Crypto.ReadGrant(store.Current.Certificate, store.RootPublic, Crypto.Now()); description = "Autorizado até " + Crypto.Date(grant.Expires) + ". Limite: " + grant.MaxDays + " dias por licença."; }
                catch (Exception) { description = "Aguardando uma autorização válida do proprietário."; }
                AddLabel(pane, description, 156, 52, 11, Design.Muted);
                ActionButton export = new ActionButton("Salvar solicitação", false); export.SetBounds(0, 244, 260, 44); pane.Controls.Add(export);
                export.Click += delegate { try { SaveText(store.ExportRequest(), "Solicitacao-" + store.Current.Id.Substring(0, 8) + ".zb2station", "Estação ZB2|*.zb2station"); } catch (Exception error) { Notice(error.Message, true); } };
                ActionButton import = new ActionButton("Importar autorização", true); import.SetBounds(284, 244, 300, 44); pane.Controls.Add(import);
                import.Click += delegate
                {
                    string file = OpenFile("Autorização do proprietário", "Autorização ZB2|*.zb2issuer"); if (file == null) return;
                    Work(delegate { store.ImportAuthorization(AdminStore.ReadLimited(file)); return true; }, delegate(bool result) { ShowPage(); Notice("Estação autorizada. Abra Licenças para emitir.", false); });
                };
                AddLabel(pane, "Envie somente a solicitação. A chave privada desta estação permanece neste PC.", 324, 60, 10, Design.Muted);
            }
        }

        internal void RenderTo(string path,string view)
        {
            if(!String.IsNullOrEmpty(view)){page=view;ShowPage();}
            // A detached control tree renders through WM_PRINT without showing a window.
            using (Panel canvas = new Panel { Size = ClientSize, BackColor = BackColor })
            {
                foreach (Control control in Controls.Cast<Control>().ToArray()) canvas.Controls.Add(control);
                canvas.CreateControl(); canvas.PerformLayout();
                using (Bitmap image = new Bitmap(canvas.Width, canvas.Height)) { canvas.DrawToBitmap(image, new Rectangle(Point.Empty, canvas.Size)); image.Save(path); }
            }
        }

#if ADMIN_TESTS
        internal void CheckInteractions()
        {
            // Synthetic events stay inside this hidden test form; no desktop input.
            IntPtr unusedHandle=Handle;
            page="Licenças";ShowPage();
            int before=store.History().Count;
            ActionButton generate=Descendants(body).OfType<ActionButton>().First(item=>item.Text=="Gerar licença");
            Action click=delegate {
                typeof(Button).GetMethod("OnClick",BindingFlags.Instance|BindingFlags.NonPublic).Invoke(generate,new object[] {EventArgs.Empty});
                System.Diagnostics.Stopwatch watch=System.Diagnostics.Stopwatch.StartNew();
                while(busy && watch.ElapsedMilliseconds<5000){Application.DoEvents();System.Threading.Thread.Sleep(5);}
                Crypto.Require(!busy,"A emissão não concluiu no teste de UI.");
            };
            customer.Editor.Text="";click();Crypto.Require(store.History().Count==before,"Campo vazio gerou licença.");
            customer.Editor.Text="Cliente do fluxo UI";device.Editor.Text=new string('1',64);days.Value=15;click();
            Crypto.Require(store.History().Count==before+1 && selected!=null && selected.Customer=="Cliente do fluxo UI" && selected.Days==15,"A emissão visual perdeu os dados selecionados.");
            Crypto.Require(copy.Enabled && save.Enabled,"A licença emitida não ficou disponível para copiar/salvar.");
            search.Editor.Text="cliente-inexistente-000";Crypto.Require(selected==null && !copy.Enabled && !save.Enabled,"A busca vazia manteve uma licença selecionada.");
            search.Editor.Text="Cliente do fluxo UI";Crypto.Require(history.Rows.Count==1,"A busca não encontrou o cliente emitido.");
        }
        private static IEnumerable<Control> Descendants(Control parent)
        {
            foreach(Control child in parent.Controls){yield return child;foreach(Control nested in Descendants(child))yield return nested;}
        }
#endif
    }

    internal static class Program
    {
        [DllImport("user32.dll")] private static extern bool SetProcessDpiAwarenessContext(IntPtr context);
        internal static string PublicKey()
        {
            using (Stream stream = Assembly.GetExecutingAssembly().GetManifestResourceStream("Admin.Public"))
            using (StreamReader reader = new StreamReader(stream))
                return new JavaScriptSerializer().Deserialize<Dictionary<string, string>>(reader.ReadToEnd())["public_xy"];
        }
        [STAThread]
        private static int Main(string[] args)
        {
            try
            {
                string root = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "ZB2Admin");
                if (args.Length >= 2 && args[0] == "--data") root = args[1];
                AdminStore store = new AdminStore(root, PublicKey());
                if (args.Length == 4 && args[2] == "--setup-owner") { store.ImportOwner(args[3], Environment.UserName); return 0; }
                SetProcessDpiAwarenessContext(new IntPtr(-4)); Application.EnableVisualStyles(); Application.SetCompatibleTextRenderingDefault(false); Design.LoadFont();
                using (AdminForm form = new AdminForm(store))
                {
                    if (args.Length >= 4 && args[2] == "--render") { form.RenderTo(args[3],args.Length>4?args[4]:null); return 0; }
                    string identity;
                    using(System.Security.Cryptography.SHA256 hash=System.Security.Cryptography.SHA256.Create())identity=Crypto.Hex(hash.ComputeHash(Encoding.UTF8.GetBytes(Path.GetFullPath(root).ToUpperInvariant())));
                    bool created;
                    using(System.Threading.Mutex instance=new System.Threading.Mutex(false,"Local\\ZB2Admin."+identity,out created)) {
                        if(!created){MessageBox.Show("O ZB2 Admin desta estação já está aberto.","ZB2 Admin",MessageBoxButtons.OK,MessageBoxIcon.Information);return 0;}
                        Application.Run(form);
                    }
                }
                return 0;
            }
            catch (Exception error) { if (args.Length > 0) { Console.Error.WriteLine(error); return 1; } MessageBox.Show(error.Message, "ZB2 Admin", MessageBoxButtons.OK, MessageBoxIcon.Error); return 1; }
        }
    }
}
