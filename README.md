# ProxyCommand

ProxyCommand is a PowerShell module to create and manage a proxy command.

A proxy command is a small program that calls a program specified when it is created. In the command line environment, it works just like a shortcut (`*.lnk`) in Explorer.

## Background

On Windows, applications reside in their own directories. When you want to use them in the command line environment (cmd.exe or PowerShell), you have to add each application directory to the PATH environment variable. Sooner or later, your PATH environment variable will become too long and difficult to maintain.

What we want is a central directory like `usr/lcoal/bin` in Unix-like systems, in which we can collect executables we want to use in the command line environment. The problem is that Windows applications expect their data files and DLLs to be placed in the same directory where they exist. Applications copied to the different directory will try to find their files in the directory that they are copied to and fail to run. Symbolic or hard links are no help.

A proxy command effectively avoids this issue by calling a target program as a child process transparently. The program called by a proxy command gets started as if its icon is double-clicked in Explorer.

## Installation

The module is available in the [PowerShell Gallery](https://www.powershellgallery.com/packages/ProxyCommand).

```PowerShell
Install-Module ProxyCommand
```

## Synopsis

This module exports two cmdlets.

-----

```PowerShell
New-ProxyCommand [-ProxyPath] <string> [-TargetPath] <string> [-Async]
```

Creates a proxy command for a target program.

- `-ProxyPath` A path of a proxy command. If a directory is specified, the name of a proxy command is the same as the target program.

- `-TargetPath` A path of a target program. If it is a file, it should be an executable (`.exe`) or a batch file (`.bat`). If a directory is given, the cmdlet looks for executables and batch files in it and creates proxy commands for all executables and batch files found. This parameter can be given from the pipeline.

- `-Async` When specified, a proxy command doesn't wait for a target program to exit. This option is suitable for GUI applications.

-----

```PowerShell
Show-ProxyCommand [-Path] <string[]>
```
Shows information on proxy commands.

- `-Path` A directory or name of proxy commands.

## Usage

### Making a directory for proxy commands

First, you have to make a directory to place proxy commands. For example:

```PowerShell
PS> mkdir ~/proxybin
```

Add this directory to the PATH environment in the Control Panel.

### Creating a proxy command

Use the `New-ProxyCommand` cmdlet.

For example, if you want to use Microsoft Excel in the command line, enter the following:

```PowerShell
PS> New-ProxyCommand ~/proxybin 'C:\Program Files\Microsoft Office\Root\Office16\EXCEL.EXE' -Async
```

It would be better to add the `-Async` option when you create a proxy command for a GUI application to avoid the console to halt until it terminates.

Once it is created, you can call it anytime in the command line:

```PowerShell
PS> excel myfile.xlsx
```

*(Note: The location of the executable may differ depending on your Windows and Office versions.)*

### Creating proxy commands at once

You can specify multiple target programs by giving them from the pipeline.

For example, if you want to create the proxy commands for all C++ tools (`cl.exe`, `nmake.exe`, and so on) in Visual Studio 2017, enter the following:

```PowerShell
PS> dir 'C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\14.14.26428\bin\Hostx64\x64\*.exe' | New-ProxyCommand ~/proxybin
```

Furthermore, you can give directories instead of executables to make the cmdlet search for executables in it. By the following commands, you can obtain the proxy commands for all tools in Visual Studio 2017:

```PowerShell
PS> # A directory listing file has been prepared beforehand
PS> Get-Content vsdir.txt
C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\14.14.26428\bin\HostX64\x64
C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\IDE\VC\VCPackages
C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\bin\Roslyn
C:\Program Files (x86)\Microsoft SDKs\Windows\v10.0A\bin\NETFX 4.6.1 Tools\x64\
C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\\MSBuild\15.0\bin
C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\Tools\

PS> # Creates proxy commands at once for all executables that exist in the directories
PS> Get-Content vsdir.txt | New-ProxyCommand ~/proxybin
```

*(Note: The above directories are found by starting "Developer Command Prompt for VS 2017" and examining the `PATH`  environment variable.)*

## License

Licensed under the MIT License.
