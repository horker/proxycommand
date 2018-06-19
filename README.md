# ProxyCommand

ProxyCommand is a PowerShell module to create and manage proxy commands.

A proxy command is a small program that just calls a program specified when it is created. It can be used as a shortcut in the command line environment.

## Background

On Windows, applications reside in their own directories. When you want to use them in the command line environment (cmd.exe or PowerShell), you have to add their directories to the PATH environment variable. Sooner or later, your PATH environment variable will become too long and difficult to maintain.

What we want is a central directory like the `usr/lcoal/bin` directory in Unix-like systems, in which we can collect executables we want to use in the command line environment. The problem is that Windows applications expect their DLLs and data files to be placed in the same directory where they exist. Applications copied to the different directory will try to find their files in the directory that they are copied to and fail to run. Symbolic or hard links are no help.

A proxy command effectively avoids this issue by calling a target program as a child process. You can use proxy commands as building blocks to manage your command line environment.

## Installation

(TBD)

The module is available in the [PowerShell Gallery](https://www.powershellgallery.com/packages/ProxyCommand).

```PowerShell
Install-Module ProxyCommand
```

## Synopsis

```PowerShell
New-ProxyCommand [-ProxyPath] <string> [-TargetPath] <string> [-Async]
```

Makes a proxy command to call a target program.

- `-ProxyPath` A path of a proxy command. If a directory is specified, the name of a proxy command is the same as the target program.

- `-TargetPath` A path of a target program. If it is a file, it should be an executable (`.exe`) or a batch file (`.bat`). If a directory is given, the cmdlet looks for executables and batch files in it. This parameter can be given from the pipeline.

- `-Async` When specified, a proxy command doesn't wait for a target program to exit. This option is suitable for GUI applications.

```PowerShell
Show-ProxyCommand [-Path] <string[]>
```
Shows information on proxy commands.

- `-Path` The location or name of proxy commands.

## Usage

### Making a directory for proxy commands

First, you have to make a directory to place proxy commands. For example:

```PowerShell
PS> mkdir ~/proxybin
```

Add this directory to the PATH environment in the Control Panel.

### Creating a proxy command

Use the `New-ProxyCommand` cmdlet.

If you want to use Microsoft Excel in the command line, enter the following:

```PowerShell
PS> New-ProxyCommand ~/proxybin 'C:\Program Files\Microsoft Office\Root\Office16\EXCEL.EXE'
```

*(Note: The location of the executable may differ depending on your Windows and Office versions. Investigate the registry key `HKEY_CLASSES_ROOT\\.xlsx`.)*

### Creating proxy commands at once

You can provide multiple target programs to the cmdlet from the pipeline.

For example, if you want to create proxy commands for Visual C++ 2017 commands (`cl.exe`, `nmake.exe`, and so on), enter the following:

```PowerShell
PS> dir 'C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\14.14.26428\bin\Hostx64\x64\*.exe' | New-ProxyCommand ~/proxybin
```

In another way, you can give directories instead of executables to make the cmdlet search for executables in it. By the following commands, you can obtain the proxies for all Visual Studio 2017 commands:

```PowerShell
PS> # Preparing a directory listing file beforehand
PS> Get-Content vsdir.txt
C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\14.14.26428\bin\HostX64\x64
C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\IDE\VC\VCPackages
C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\bin\Roslyn
C:\Program Files (x86)\Microsoft SDKs\Windows\v10.0A\bin\NETFX 4.6.1 Tools\x64\
C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\\MSBuild\15.0\bin
C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\Tools\

PS> Get-Content vsdir.txt | New-ProxyCommand ~/proxybin
```

*(Note: The above directories are found by starting "Developer Command Prompt for VS 2017" and examining the `PATH`  environment variable.)*

## License

Licensed under the MIT License.
