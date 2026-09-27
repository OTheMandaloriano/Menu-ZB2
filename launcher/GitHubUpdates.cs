using System;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Net.Http.Headers;
using System.Threading.Tasks;
using System.Web.Script.Serialization;

namespace Zb2Launcher {
    public sealed class ReleaseAsset {public string name,digest,url;public long size;}
    public sealed class Release {public bool draft;public string tag_name;public ReleaseAsset[] assets;}
    public sealed class GitHubUpdates {
        public const string Repository="OTheMandaloriano/Menu-ZB2-Releases";
        public async Task<ReleaseAsset> Check(string token,Version installed) {
            byte[] data=await Get(new Uri("https://api.github.com/repos/"+Repository+"/releases?per_page=20"),token,false,2*1024*1024);
            var releases=new JavaScriptSerializer().Deserialize<Release[]>(System.Text.Encoding.UTF8.GetString(data));
            foreach(var release in releases??new Release[0]) {
                if(release.draft)continue;
                Version candidate;try{candidate=PackageStore.ParseVersion((release.tag_name??"").TrimStart('v'));}catch(InvalidDataException){continue;}
                if(candidate<=installed)continue;
                var asset=(release.assets??new ReleaseAsset[0]).FirstOrDefault(a=>a.name=="ZB2Menu-runtime.zip");
                if(asset==null)continue;
                if(asset.digest==null || !System.Text.RegularExpressions.Regex.IsMatch(asset.digest,@"^sha256:[a-f0-9]{64}$"))throw new InvalidDataException("Release sem hash verificavel");
                if(asset.size<=0 || asset.size>64*1024*1024)throw new InvalidDataException("Tamanho de release invalido");
                return asset;
            }
            return null;
        }
        public Task<byte[]> Download(ReleaseAsset asset,string token) {
            var uri=new Uri(asset.url);
            if(uri.Host!="api.github.com" || !uri.AbsolutePath.StartsWith("/repos/"+Repository+"/releases/assets/",StringComparison.Ordinal))throw new InvalidDataException("Origem inesperada");
            return Get(uri,token,true,64*1024*1024);
        }
        static async Task<byte[]> Get(Uri uri,string token,bool binary,int limit) {
            ServicePointManager.SecurityProtocol=SecurityProtocolType.Tls12;
            using(var handler=new HttpClientHandler{AllowAutoRedirect=false})using(var client=new HttpClient(handler)){
                client.Timeout=TimeSpan.FromSeconds(90);
                for(int redirects=0;redirects<4;++redirects) {
                    if(uri.Scheme!="https" || !(uri.Host=="api.github.com" || uri.Host=="github.com" || uri.Host.EndsWith(".githubusercontent.com",StringComparison.OrdinalIgnoreCase)))throw new InvalidDataException("Redirecionamento nao autorizado");
                    using(var request=new HttpRequestMessage(HttpMethod.Get,uri)) {
                        request.Headers.UserAgent.ParseAdd("ZB2Menu-Launcher/0.2.0");
                        request.Headers.Accept.Add(new MediaTypeWithQualityHeaderValue(binary?"application/octet-stream":"application/vnd.github+json"));
                        if(uri.Host=="api.github.com" && !string.IsNullOrWhiteSpace(token))request.Headers.Authorization=new AuthenticationHeaderValue("Bearer",token.Trim());
                        using(var response=await client.SendAsync(request,HttpCompletionOption.ResponseHeadersRead)) {
                            int status=(int)response.StatusCode;
                            if(status==301 || status==302 || status==307 || status==308){uri=new Uri(uri,response.Headers.Location);continue;}
                            if(!response.IsSuccessStatusCode)throw new InvalidOperationException("GitHub HTTP "+status+". Confira sua permissao de leitura no canal privado.");
                            if(response.Content.Headers.ContentLength>limit)throw new InvalidDataException("Resposta excessiva");
                            using(var input=await response.Content.ReadAsStreamAsync())using(var output=new MemoryStream()) {
                                byte[] buffer=new byte[8192];int read;
                                while((read=await input.ReadAsync(buffer,0,buffer.Length))>0){if(output.Length+read>limit)throw new InvalidDataException("Resposta excessiva");output.Write(buffer,0,read);}
                                return output.ToArray();
                            }
                        }
                    }
                }
            }
            throw new InvalidDataException("Redirecionamentos excessivos");
        }
    }
}
