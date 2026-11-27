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
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.Reflection;
using System.Threading;

internal static class AxionUpdater
{
    private const string AssetName = "Axion-windows-x64.zip";
    private static Form window;
    private static string stage = "Preparing update…", description = "";
    private static int progress = -1;

    private sealed class UpdateWindow : Form
    {
        private readonly Image logo = Image.FromStream(Assembly.GetExecutingAssembly().GetManifestResourceStream("AxionLogo"));
        private readonly System.Windows.Forms.Timer frames = new System.Windows.Forms.Timer { Interval = 16 };
        private float displayed;
        private readonly Stopwatch clock = Stopwatch.StartNew();
        private bool hoverClose;

        private static GraphicsPath Round(RectangleF rect, float radius)
        {
            var path = new GraphicsPath();
            float d = radius * 2;
            path.AddArc(rect.Left, rect.Top, d, d, 180, 90);
            path.AddArc(rect.Right - d, rect.Top, d, d, 270, 90);
            path.AddArc(rect.Right - d, rect.Bottom - d, d, d, 0, 90);
            path.AddArc(rect.Left, rect.Bottom - d, d, d, 90, 90);
            path.CloseFigure();
            return path;
        }

        public UpdateWindow()
        {
            Text = "Axion Updater";
            Font = new Font("Segoe UI", 10);
            ClientSize = new Size(680, 240);
            FormBorderStyle = FormBorderStyle.None;
            StartPosition = FormStartPosition.CenterScreen;
            DoubleBuffered = true;
            using (var outline = Round(new RectangleF(0, 0, Width, Height), 20))
                Region = new Region(outline);
            frames.Tick += (_, __) => { displayed += ((progress < 0 ? 0 : progress / 100f) - displayed) * 0.19f; Invalidate(); };
            frames.Start();
        }

        protected override void OnPaint(PaintEventArgs args)
        {
            var g = args.Graphics;
            g.SmoothingMode = SmoothingMode.AntiAlias;
            g.InterpolationMode = InterpolationMode.HighQualityBicubic;
            using (var face = new LinearGradientBrush(ClientRectangle, Color.FromArgb(30, 30, 30), Color.FromArgb(14, 14, 14), 70f))
                g.FillRectangle(face, ClientRectangle);
            using (var outline = Round(new RectangleF(1, 1, Width - 2, Height - 2), 19))
            using (var border = new Pen(Color.FromArgb(96, 96, 96)))
            {
                g.DrawPath(border, outline);
                g.DrawLine(border, 1, 38, Width - 2, 38);
            }
            TextRenderer.DrawText(g, Text, Font, new Point(20, 9), Color.Gainsboro);
            Color cross = !canClose ? Color.Gray : hoverClose ? Color.White : Color.Gainsboro;
            using (var pen = new Pen(cross, 1.6f))
            {
                g.DrawLine(pen, Width - 34, 13, Width - 22, 25);
                g.DrawLine(pen, Width - 22, 13, Width - 34, 25);
            }
            g.DrawImage(logo, new Rectangle(210, 59, 260, 87));
            var track = new RectangleF(68, 169, Width - 136, 16);
            using (var shape = Round(track, 8))
            using (var border = new Pen(Color.Gray))
                g.DrawPath(border, shape);
            float inner = track.Width - 6;
            float fill = progress < 0 ? inner * 0.21f : inner * displayed;
            float offset = progress < 0 ? (inner - fill) * (0.5f - 0.5f * (float)Math.Cos(clock.Elapsed.TotalSeconds * 3)) : 0;
            if (fill >= 2)
                using (var shape = Round(new RectangleF(track.X + 3 + offset, track.Y + 3, fill, 10), Math.Min(5, fill / 2)))
                    g.FillPath(Brushes.WhiteSmoke, shape);
            var flags = TextFormatFlags.HorizontalCenter | TextFormatFlags.EndEllipsis | TextFormatFlags.SingleLine;
            TextRenderer.DrawText(g, stage, Font, new Rectangle(50, 193, Width - 100, 21), Color.Gainsboro, flags);
            TextRenderer.DrawText(g, description, Font, new Rectangle(50, 213, Width - 100, 19), Color.Gray, flags);
        }

        protected override void OnMouseMove(MouseEventArgs args)
        {
            hoverClose = args.Y < 38 && args.X > Width - 54;
            base.OnMouseMove(args);
        }

        protected override void OnMouseDown(MouseEventArgs args)
        {
            if (args.Button == MouseButtons.Left && args.Y < 38)
            {
                if (args.X > Width - 54)
                {
                    if (canClose) Close();
                }
                else
                {
                    ReleaseCapture();
                    SendMessage(Handle, 0xA1, new IntPtr(2), IntPtr.Zero);
                }
            }
            base.OnMouseDown(args);
        }

        [System.Runtime.InteropServices.DllImport("user32.dll")]
        private static extern bool ReleaseCapture();
        [System.Runtime.InteropServices.DllImport("user32.dll")]
        private static extern IntPtr SendMessage(IntPtr hwnd, int message, IntPtr wparam, IntPtr lparam);

        protected override void Dispose(bool disposing)
        {
            if (disposing) { frames.Dispose(); logo.Dispose(); }
            base.Dispose(disposing);
        }
    }
    private static bool canClose;

    private static readonly string[] ObsoleteFiles =
    {
        "CHAT_SELECTION.md", "LATEST_UPDATE.md", "BESTCLIENT_LICENSE.md", "BESTCLIENT-LICENSE.txt"
    };

    private static bool IsUpdateFolder(string path)
    {
        const string prefix = ".axion-update-";
        string name = Path.GetFileName(path);
        Guid id;
        return name.StartsWith(prefix, StringComparison.OrdinalIgnoreCase) &&
            Guid.TryParseExact(name.Substring(prefix.Length), "N", out id);
    }

    private static void RemoveUpdateDirectory(string path)
    {
        if ((File.GetAttributes(path) & FileAttributes.ReparsePoint) != 0)
        {
            Directory.Delete(path);
            return;
        }
        foreach (string child in Directory.EnumerateDirectories(path))
            RemoveUpdateDirectory(child);
        foreach (string file in Directory.EnumerateFiles(path))
            File.Delete(file);
        Directory.Delete(path);
    }

    private static void Cleanup(string directory, int installerId)
    {
        try
        {
            using (var installer = Process.GetProcessById(installerId))
                if (!installer.WaitForExit(30000))
                    return;
        }
        catch (ArgumentException) { }

        foreach (string folder in Directory.EnumerateDirectories(directory, ".axion-update-*", SearchOption.TopDirectoryOnly))
        {
            if (!IsUpdateFolder(folder))
                continue;
            for (int attempt = 0; attempt < 6; ++attempt)
            {
                try
                {
                    if (Directory.Exists(folder))
                        RemoveUpdateDirectory(folder);
                    break;
                }
                catch (IOException) { Thread.Sleep(350); }
                catch (UnauthorizedAccessException) { Thread.Sleep(350); }
            }
        }
        foreach (string name in ObsoleteFiles)
        {
            // Preserve the separate notice when updating from an archive with the original license.
            if (name == "BESTCLIENT-LICENSE.txt")
            {
                string license = Path.Combine(directory, "license.txt");
                if (!File.Exists(license) || !File.ReadAllText(license).Contains("Copyright (c) 2026 BestProject"))
                    continue;
            }
            try { File.Delete(Path.Combine(directory, name)); }
            catch (IOException) { }
            catch (UnauthorizedAccessException) { }
        }
    }

    [STAThread]
    private static void Main(string[] args)
    {
        int installerId;
        if (args.Length == 2 && args[0] == "cleanup" && int.TryParse(args[1], out installerId) && installerId > 0)
        {
            using (var singleton = new Mutex(false, @"Local\AxionUpdaterInstall"))
            {
                if (!singleton.WaitOne(30000))
                    return;
                try { Cleanup(AppDomain.CurrentDomain.BaseDirectory, installerId); }
                finally { singleton.ReleaseMutex(); }
            }
            return;
        }
        bool created;
        using (var singleton = new Mutex(true, args.Length == 3 && args[0] == "install" ? @"Local\AxionUpdaterInstall" : @"Local\AxionUpdater", out created))
        {
            if (!created)
                return;
            Application.EnableVisualStyles();
            window = new UpdateWindow();
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
            stage = step;
            description = information;
            progress = percent < 0 ? -1 : Math.Min(100, percent);
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
                string metadata = http.DownloadString("https://api.github.com/repos/" + repository + "/releases/latest?check=" + DateTime.UtcNow.Ticks);
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
                    stage = "Update failed";
                    description = error.Message;
                    progress = 0;
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
            File.Copy(Path.Combine(package, "AxionUpdater.exe"), worker);
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
        if (!IsUpdateFolder(work))
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
                       relative == "DDNet.exe" || relative == "AxionUpdater.exe" || relative == "license.txt")))
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

        // This worker runs inside the directory being cleaned. Run cleanup from the installed updater.
        try
        {
            using (Process cleanup = Process.Start(Path.Combine(directory, "AxionUpdater.exe"),
                "cleanup " + Process.GetCurrentProcess().Id))
            {
                if (cleanup != null)
                    return;
            }
        }
        catch (System.ComponentModel.Win32Exception) { }
        // The installed client is already running. The next successful update can retry cleanup.
    }
}
