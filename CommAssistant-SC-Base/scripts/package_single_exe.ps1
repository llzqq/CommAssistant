param(
    [string]$QtRoot = "D:/Tools/Qt/6.10.3/mingw_64",
    [string]$MingwRoot = "D:/Tools/Qt/Tools/mingw1310_64",
    [string]$OutputExe = "",
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$projectFile = Join-Path $repoRoot "ScriptCommunicator/ScriptCommunicator.pro"
$buildDir = Join-Path $repoRoot "build-mingw"
$releaseExe = Join-Path $buildDir "release/ScriptCommunicator.exe"
$packageRoot = Join-Path $buildDir "single-exe"

if($OutputExe -eq "")
{
    $OutputExe = Join-Path $packageRoot "CommAssistant.exe"
}

$qmake = Join-Path $QtRoot "bin/qmake.exe"
$windeployqt = Join-Path $QtRoot "bin/windeployqt.exe"
$make = Join-Path $MingwRoot "bin/mingw32-make.exe"
$csc = Join-Path $env:WINDIR "Microsoft.NET/Framework64/v4.0.30319/csc.exe"

foreach($tool in @($qmake, $windeployqt, $make, $csc))
{
    if(!(Test-Path $tool))
    {
        throw "Required tool not found: $tool"
    }
}

New-Item -ItemType Directory -Force $buildDir | Out-Null
New-Item -ItemType Directory -Force $packageRoot | Out-Null

$env:PATH = (Join-Path $MingwRoot "bin") + ";" + (Join-Path $QtRoot "bin") + ";" + $env:PATH

if(!$SkipBuild)
{
    Push-Location $buildDir
    try
    {
        & $qmake $projectFile -spec win32-g++ "CONFIG+=release"
        & $make -j8 release
    }
    finally
    {
        Pop-Location
    }
}

if(!(Test-Path $releaseExe))
{
    throw "Release executable not found: $releaseExe"
}

$stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$stage = Join-Path $packageRoot "stage-$stamp"
$payloadZip = Join-Path $packageRoot "payload-$stamp.zip"
$launcherSource = Join-Path $packageRoot "SingleExeLauncher-$stamp.cs"

New-Item -ItemType Directory -Force $stage | Out-Null
Copy-Item -LiteralPath $releaseExe -Destination (Join-Path $stage "ScriptCommunicator.exe") -Force

$stylesheet = Join-Path $repoRoot "ScriptCommunicator/qss/stylesheet.qss"
$stylesheetRcc = Join-Path $repoRoot "ScriptCommunicator/qss/stylesheet.rcc"
Copy-Item -LiteralPath $stylesheet -Destination $stage -Force
Copy-Item -LiteralPath $stylesheetRcc -Destination $stage -Force

$templatesDir = Join-Path $repoRoot "ScriptCommunicator/templates"
if(Test-Path $templatesDir)
{
    Copy-Item -LiteralPath $templatesDir -Destination (Join-Path $stage "templates") -Recurse -Force
}

$configDir = Join-Path $repoRoot "ScriptCommunicator/config"
if(Test-Path $configDir)
{
    Copy-Item -LiteralPath $configDir -Destination (Join-Path $stage "config") -Recurse -Force
}

$apiFilesDir = Join-Path $repoRoot "ScriptCommunicator/ScriptEditor/apiFiles"
if(Test-Path $apiFilesDir)
{
    Copy-Item -LiteralPath $apiFilesDir -Destination (Join-Path $stage "apiFiles") -Recurse -Force
}

$manualFile = Join-Path $repoRoot "ScriptCommunicator/documentation/Manual_ScriptCommunicator.pdf"
if(Test-Path $manualFile)
{
    Copy-Item -LiteralPath $manualFile -Destination (Join-Path $stage "Manual_ScriptCommunicator.pdf") -Force
}

& $windeployqt --release --dir $stage (Join-Path $stage "ScriptCommunicator.exe")

Compress-Archive -Path (Join-Path $stage "*") -DestinationPath $payloadZip -Force

@'
using System;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Reflection;

internal static class SingleExeLauncher
{
    private const string ResourceName = "Payload.zip";
    private const string AppExeName = "ScriptCommunicator.exe";

    private static int Main()
    {
        string root = Path.Combine(Path.GetTempPath(), "CommAssistant-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(root);

        try
        {
            ExtractPayload(root);

            string appPath = Path.Combine(root, AppExeName);
            ProcessStartInfo info = new ProcessStartInfo();
            info.FileName = appPath;
            info.WorkingDirectory = root;
            info.UseShellExecute = false;

            Process process = Process.Start(info);
            if(process == null)
            {
                return 1;
            }

            process.WaitForExit();
            return process.ExitCode;
        }
        catch(Exception ex)
        {
            File.WriteAllText(Path.Combine(root, "launcher-error.txt"), ex.ToString());
            return 1;
        }
        finally
        {
            try
            {
                Directory.Delete(root, true);
            }
            catch
            {
            }
        }
    }

    private static void ExtractPayload(string root)
    {
        Assembly assembly = Assembly.GetExecutingAssembly();
        Stream stream = assembly.GetManifestResourceStream(ResourceName);
        if(stream == null)
        {
            throw new InvalidOperationException("Embedded payload not found.");
        }

        string rootFullPath = Path.GetFullPath(root);
        if(!rootFullPath.EndsWith(Path.DirectorySeparatorChar.ToString()))
        {
            rootFullPath += Path.DirectorySeparatorChar;
        }

        using(stream)
        using(ZipArchive archive = new ZipArchive(stream, ZipArchiveMode.Read))
        {
            foreach(ZipArchiveEntry entry in archive.Entries)
            {
                string destinationPath = Path.GetFullPath(Path.Combine(root, entry.FullName));
                if(!destinationPath.StartsWith(rootFullPath, StringComparison.OrdinalIgnoreCase))
                {
                    throw new InvalidOperationException("Invalid payload entry: " + entry.FullName);
                }

                if(String.IsNullOrEmpty(entry.Name))
                {
                    Directory.CreateDirectory(destinationPath);
                    continue;
                }

                string directory = Path.GetDirectoryName(destinationPath);
                if(!String.IsNullOrEmpty(directory))
                {
                    Directory.CreateDirectory(directory);
                }

                entry.ExtractToFile(destinationPath, true);
            }
        }
    }
}
'@ | Set-Content -Path $launcherSource -Encoding UTF8

$iconFile = Join-Path $repoRoot "ScriptCommunicator/images/main.ico"
$cscArgs = @(
    "/nologo",
    "/target:winexe",
    "/platform:x64",
    "/optimize+",
    "/out:$OutputExe",
    "/resource:$payloadZip,Payload.zip",
    "/reference:System.IO.Compression.dll",
    "/reference:System.IO.Compression.FileSystem.dll"
)

if(Test-Path $iconFile)
{
    $cscArgs += "/win32icon:$iconFile"
}

$cscArgs += $launcherSource
& $csc @cscArgs

Write-Host "Single executable created: $OutputExe"
