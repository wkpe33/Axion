using System;
using System.IO;
using System.IO.Compression;
using System.Diagnostics;
using System.Collections;
using System.Net;
using System.Security.Cryptography;
using System.Text.RegularExpressions;
using System.Web.Script.Serialization;
using System.Collections.Generic;
using System.Windows.Forms;
using System.Drawing;
using System.Threading;

internal static class AxionUpdater
{
    private const string AssetName = "Axion-windows-x64.zip";
    private static Form window;
    private static Label status;
    private static Label detail;
    private static ProgressBar progress;
    private static bool canClose;

    [STAThread]
    private static void Main(string[] args)
    {
        bool created;
        using (var singleton = new Mutex(true, args.Length == 3 && args[0] == "install" ? @"Local\AxionUpdaterInstall" : @"Local\AxionUpdater", out created))
        {
            if (!created)
                return;
            Application.EnableVisualStyles();
            window = new Form
            {
                Text = "Axion Updater",
                ClientSize = new Size(500, 230),
                BackColor = Color.FromArgb(20, 25, 34),
                ForeColor = Color.White,
                FormBorderStyle = FormBorderStyle.FixedDialog,
                MaximizeBox = false,
                StartPosition = FormStartPosition.CenterScreen
            };
            var title = new Label { Text = "AXION", Font = new Font("Segoe UI", 22, FontStyle.Bold), ForeColor = Color.FromArgb(125, 192, 255), Location = new Point(28, 22), Size = new Size(440, 44) };
            status = new Label { Text = "Preparing update…", Font = new Font("Segoe UI", 12, FontStyle.Bold), Location = new Point(30, 84), Size = new Size(440, 28) };
            detail = new Label { Text = "Checking release", Font = new Font("Segoe UI", 9), ForeColor = Color.FromArgb(165, 179, 195), Location = new Point(30, 119), Size = new Size(440, 25) };
            progress = new ProgressBar { Location = new Point(30, 160), Size = new Size(440, 16), Style = ProgressBarStyle.Marquee, MarqueeAnimationSpeed = 25 };
            window.Controls.AddRange(new Control[] { title, status, detail, progress });
            window.FormClosing += (sender, eventArgs) => { if (!canClose) eventArgs.Cancel = true; };
            window.Shown += (sender, eventArgs) =>
            {
                var worker = new Thread(() => Run(args)) { IsBackground = true };
                worker.Start();
            };
            Application.Run(window);
        }
    }

    private static void Report(string step, string information, int percent = -1)
    {
        if (window == null || window.IsDisposed || !window.IsHandleCreated)
            return;
        window.BeginInvoke((Action)(() =>
        {
            status.Text = step;
            detail.Text = information;
            progress.Style = percent < 0 ? ProgressBarStyle.Marquee : ProgressBarStyle.Continuous;
            if (percent >= 0)
                progress.Value = Math.Max(0, Math.Min(100, percent));
        }));
    }

    private static void Run(string[] args)
    {
        try
        {
            string directory = AppDomain.CurrentDomain.BaseDirectory;
            if (args.Length == 3 && args[0] == "install")
            {
                Report("Installing Axion", "Waiting for the game to close…");
                Install(directory, int.Parse(args[1]), int.Parse(args[2]));
                window.BeginInvoke((Action)(() => { canClose = true; window.Close(); }));
                return;
            }
            if (args.Length != 2)
                throw new InvalidOperationException("Start the updater using Update Axion in the main menu.");
            int clientId = int.Parse(args[0]);
            Version installed = new Version(args[1]);
            const string repository = "wkpe33/Axion";

            ServicePointManager.SecurityProtocol = SecurityProtocolType.Tls12;
            using (var http = new WebClient())
            {
                Report("Checking for updates", "Reading the latest Axion release…");
                http.Headers[HttpRequestHeader.UserAgent] = "AxionUpdater";
                string metadata = http.DownloadString("https://api.github.com/repos/" + repository + "/releases/latest");
                var release = new JavaScriptSerializer().Deserialize<Dictionary<string, object>>(metadata);
                string tag = release["tag_name"] as string;
                Version latest;
                if (tag == null || !tag.StartsWith("v", StringComparison.Ordinal) ||
                    !Version.TryParse(tag.Substring(1), out latest) || latest <= installed)
                    throw new InvalidOperationException("No newer Axion release is available.");
                foreach (Dictionary<string, object> asset in (IEnumerable)release["assets"])
                {
                    if (!String.Equals(asset["name"] as string, AssetName, StringComparison.Ordinal))
                        continue;
                    object digest;
                    if (!asset.TryGetValue("digest", out digest) || !Regex.IsMatch(digest as string ?? "", @"\Asha256:[a-fA-F0-9]{64}\z"))
                        throw new InvalidOperationException("The release archive has no SHA-256 digest.");
                    string url = asset["browser_download_url"] as string;
                    string expectedPrefix = "https://github.com/" + repository + "/releases/download/";
                    if (url == null || !url.StartsWith(expectedPrefix, StringComparison.OrdinalIgnoreCase))
                        throw new InvalidOperationException("Unexpected release download URL.");
                    Download(http, url, ((string)digest).Substring(7), directory, clientId);
                    return;
                }
                throw new InvalidOperationException("The latest release does not contain " + AssetName + ".");
            }
        }
        catch (Exception error)
        {
            if (window != null && window.IsHandleCreated)
                window.BeginInvoke((Action)(() =>
                {
                    status.Text = "Update failed";
                    detail.Text = error.Message;
                    progress.Style = ProgressBarStyle.Continuous;
                    progress.Value = 0;
                    canClose = true;
                    MessageBox.Show(window, error.Message, "Axion Updater", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }));
        }
    }

    private static void Download(WebClient http, string url, string digest, string directory, int clientId)
    {
        string updates = Path.Combine(directory, ".axion-update-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(updates);
        string temporary = Path.Combine(updates, AssetName);
        bool handedOff = false;
        try
        {
            Report("Downloading Axion", "Connecting to the release…", 0);
            var request = (HttpWebRequest)WebRequest.Create(url);
            request.UserAgent = "AxionUpdater";
            using (var response = request.GetResponse())
            using (var input = response.GetResponseStream())
            using (var output = File.Create(temporary))
            {
                byte[] buffer = new byte[128 * 1024];
                long received = 0;
                int lastPercent = -1;
                int count;
                while ((count = input.Read(buffer, 0, buffer.Length)) != 0)
                {
                    output.Write(buffer, 0, count);
                    received += count;
                    if (response.ContentLength > 0)
                    {
                        int percent = (int)Math.Min(100, received * 100 / response.ContentLength);
                        if (percent != lastPercent)
                        {
                            Report("Downloading Axion", percent + "% downloaded", percent);
                            lastPercent = percent;
                        }
                    }
                    else
                        Report("Downloading Axion", (received / 1048576) + " MB downloaded");
                }
            }
            Report("Verifying download", "Checking SHA-256 integrity…");
            using (var stream = File.OpenRead(temporary))
            using (var sha256 = SHA256.Create())
            {
                string actual = BitConverter.ToString(sha256.ComputeHash(stream)).Replace("-", "");
                if (!String.Equals(actual, digest, StringComparison.OrdinalIgnoreCase))
                    throw new InvalidOperationException("The release archive failed SHA-256 verification.");
            }
            string staging = Path.Combine(updates, "staging");
            Report("Preparing files", "Extracting the Axion release…");
            ZipFile.ExtractToDirectory(temporary, staging);
            string package = Path.Combine(staging, "Axion-client");
            if (!File.Exists(Path.Combine(package, "DDNet.exe")) ||
                !File.Exists(Path.Combine(package, "AxionUpdater.exe")) ||
                !Directory.Exists(Path.Combine(package, "data")))
                throw new InvalidOperationException("The release does not contain a complete Axion installation.");

            string worker = Path.Combine(updates, "AxionUpdater.exe");
            File.Copy(Path.Combine(directory, "AxionUpdater.exe"), worker);
            using (Process process = Process.Start(worker, "install " + clientId + " " + Process.GetCurrentProcess().Id))
                handedOff = process != null;
            if (!handedOff)
                throw new InvalidOperationException("Could not start the update installer.");
            Report("Ready to install", "Axion will close and restart automatically.", 100);
            window.BeginInvoke((Action)(() => { canClose = true; window.Close(); }));
        }
        finally
        {
            if (!handedOff)
                Directory.Delete(updates, true);
        }
    }

    private static void Install(string workerDirectory, int clientId, int bootstrapId)
    {
        string work = Path.GetFullPath(workerDirectory.TrimEnd(Path.DirectorySeparatorChar));
        if (!Path.GetFileName(work).StartsWith(".axion-update-", StringComparison.Ordinal))
            throw new InvalidOperationException("Invalid update working directory.");
        string directory = Directory.GetParent(work).FullName;
        string executable = Path.Combine(directory, "DDNet.exe");
        string package = Path.Combine(work, "staging", "Axion-client");
        if (!File.Exists(Path.Combine(package, "DDNet.exe")))
            throw new InvalidOperationException("Staged Axion executable is missing.");

        try
        {
            using (Process bootstrap = Process.GetProcessById(bootstrapId))
                if (!bootstrap.WaitForExit(10000))
                    throw new InvalidOperationException("The update downloader did not close.");
        }
        catch (ArgumentException) { }

        using (Process client = Process.GetProcessById(clientId))
        {
            if (!String.Equals(client.MainModule.FileName, executable, StringComparison.OrdinalIgnoreCase))
                throw new InvalidOperationException("The running client does not match this installation.");
            if (!client.CloseMainWindow() || !client.WaitForExit(30000))
                throw new InvalidOperationException("Axion did not close. Cancel dialogs and retry the update.");
        }

        var rollback = new Stack<Action>();
        try
        {
            Report("Installing Axion", "Replacing client files…");
            string backup = Path.Combine(work, "backup");
            foreach (string file in Directory.EnumerateFiles(package, "*", SearchOption.AllDirectories))
            {
                string relative = file.Substring(package.Length + 1);
                if (!relative.StartsWith("data" + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase) &&
                    (relative.IndexOf(Path.DirectorySeparatorChar) >= 0 ||
                     !(relative.EndsWith(".dll", StringComparison.OrdinalIgnoreCase) ||
                       relative == "DDNet.exe" || relative == "AxionUpdater.exe")))
                    continue;

                string destination = Path.Combine(directory, relative);
                string saved = Path.Combine(backup, relative);
                Directory.CreateDirectory(Path.GetDirectoryName(destination));
                if (File.Exists(destination))
                {
                    Directory.CreateDirectory(Path.GetDirectoryName(saved));
                    File.Replace(file, destination, saved);
                    rollback.Push(() => File.Replace(saved, destination, null));
                }
                else
                {
                    File.Move(file, destination);
                    rollback.Push(() => File.Delete(destination));
                }
            }
            using (Process process = Process.Start(new ProcessStartInfo(executable) { WorkingDirectory = directory }))
                if (process == null)
                    throw new InvalidOperationException("Could not restart Axion.");
            Report("Update complete", "Axion is restarting.", 100);
        }
        catch
        {
            while (rollback.Count != 0)
                rollback.Pop()();
            Process.Start(new ProcessStartInfo(executable) { WorkingDirectory = directory });
            throw;
        }

        foreach (string folder in new[] { "staging", "backup" })
            Directory.Delete(Path.Combine(work, folder), true);
        File.Delete(Path.Combine(work, AssetName));
    }
}
