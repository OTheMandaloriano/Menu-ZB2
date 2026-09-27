// Optional authenticated smoke test. Token is supplied only in the process environment.
using System;
using Zb2Launcher;
class RemoteSmoke {
    static void Main(string[] args) {
        var token=Environment.GetEnvironmentVariable("ZB2_UPDATE_TEST_TOKEN");
        if(string.IsNullOrWhiteSpace(token))throw new InvalidOperationException("Test credential missing");
        var client=new GitHubUpdates();
        var asset=client.Check(token,new Version(0,0,0)).GetAwaiter().GetResult();
        if(asset==null)throw new Exception("Expected a published runtime");
        var bytes=client.Download(asset,token).GetAwaiter().GetResult();
        var store=new PackageStore(args[0],()=>false);
        var manifest=store.Install(bytes,asset.digest.Substring(7));
        if(store.Current().version!=manifest.version)throw new Exception("Installed version mismatch");
        if(client.Check(token,PackageStore.ParseVersion(manifest.version)).GetAwaiter().GetResult()!=null)throw new Exception("Same version offered as update");
        Console.WriteLine("PASS: authenticated release lookup, private download, digest, install and no-repeat update; version="+manifest.version);
    }
}
