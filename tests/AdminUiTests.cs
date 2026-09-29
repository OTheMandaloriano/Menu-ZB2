using System;
using System.IO;
using System.Windows.Forms;
namespace Zb2Admin
{
    internal static class AdminUiTests
    {
        [STAThread]
        private static int Main(string[] args)
        {
            try
            {
                Application.EnableVisualStyles();Application.SetCompatibleTextRenderingDefault(false);Design.LoadFont();
                System.Threading.SynchronizationContext.SetSynchronizationContext(new WindowsFormsSynchronizationContext());
                AdminStore store=new AdminStore(args[1],Program.PublicKey());store.ImportOwner(args[0],"Teste UI");
                using(AdminForm form=new AdminForm(store))form.CheckInteractions();
                Console.WriteLine("Admin UI: empty input, issuance, selection, search and export availability passed.");return 0;
            }
            catch(Exception error){Console.Error.WriteLine(error);return 1;}
        }
    }
}
