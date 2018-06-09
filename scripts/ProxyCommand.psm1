Set-StrictMode -Version 3

$GUID = "f482a1af-fdd3-4548-9b56-58149d54ce21"
$PROXY_COMMAND = "$PSScriptRoot\cmdproxy.exe"

function Set-ProxyCommand {
  param(
    [string]$Proxy,
    [string]$Target,
    [bool]$Async = $false
  )

  if ($Target -match "\.exe$") {
    Set-ProxyExecutable $Proxy $Target $Async
  }
  elseif ($Target -match "\.bat$") {
    Set-ProxyBatchFile $Proxy $Target
  }
}

function Set-ProxyExecutable {
  param(
    [string]$Proxy,
    [string]$Target,
    [bool]$Async
  )

  if ([IO.Directory]::Exists($Proxy)) {
    $Proxy = Join-Path $Proxy (Split-Path -Leaf $Target)
  }

  Write-Verbose "$Proxy=$Target"

  Copy-Item $PROXY_COMMAND $Proxy -Force
  $Target | Set-Content -Encoding Unicode -LiteralPath $Proxy -Stream TargetPath

  if ($Async) {
    "true" | Set-Content -Encoding Unicode -LiteralPath $Proxy -Stream Async
  }
}

function Set-ProxyBatchFile {
  param(
    [string]$Proxy,
    [string]$Target
  )

  if ([IO.Directory]::Exists($Proxy)) {
    $Proxy = Join-Path $Proxy (Split-Path -Leaf $Target)
  }

  Write-Verbose "$Proxy=$Target"

  $template = @"
@echo off
rem nugetid: $GUID
rem generated by New-ProxyCommand on $((Get-Date).ToString("yyyy/MM/dd HH:mm:ss"))
"$Target" %*
"@

  $template | Set-Content -Encoding default -LiteralPath $Proxy
}

function Test-Stream {
  param(
    [string]$Path,
    [string]$StreamName
  )

  try {
    $null = Get-Content -LiteralPath $Path -Stream $StreamName
    return $true
  }
  catch {
    return $false
  }
}

<#
.SYNOPSIS
Makes a proxy command to call a target program.

.DESCRIPTION
A proxy command is a small program that calls another program as a child process. It can be used as a shortcut in the command line environment.

.PARAMETER ProxyPath
A directory or name of a proxy command. If a directory is specified, the name of a proxy command is the same as the target program.

.PARAMETER TargetPath
A target program. A target program should be an executable (.exe) or a batch file (.bat). -TargetPath can be given from the pipeline.

.PARAMETER Async
When specified, a proxy command doesn't wait for a target program to exit. This option is suitable for GUI applications.

.LINK
https://github.com/horker/proxycommand
https://www.powershellgallery.com/packages/ProxyCommand
#>
function New-ProxyCommand {
  [CmdletBinding()]
  param(
    [Parameter(Position=0, Mandatory=$true)]
    [string]$ProxyPath,

    [Parameter(Position=1, ValueFromPipeline=$true, Mandatory=$true)]
    [object]$TargetPath,

    [Parameter(Position=2, Mandatory=$false)]
    [switch]$Async
  )

  begin {
    $ProxyPath = try { Resolve-Path $ProxyPath -EA Stop } catch { $_.TargetObject }
  }

  process {
    if ($TargetPath -is [IO.FileInfo] -or $TargetPath -is [IO.DirectoryInfo]) {
      $TargetPath = $TargetPath.FullName
    }
    if ([IO.Directory]::Exists($TargetPath)) {
      Get-ChildItem $TargetPath |
      Where-Object { $_.Extension -match "\.(exe|bat)$" } |
      ForEach-Object {
        Set-ProxyCommand $ProxyPath $_.FullName $Async
      }
    }
    else {
      Set-ProxyCommand $ProxyPath $TargetPath $Async
    }
  }
}

<#
.SYNOPSIS
Shows information on proxy commands.

.DESCRIPTION
Shows information on proxy commands.

.PARAMETER Path
The location or name of proxy commands.
#>
function Show-ProxyCommand {
  [CmdletBinding()]
  param(
    [Parameter(Position=0, Mandatory=$true)]
    [string[]]$Path
  )

  foreach ($p in $Path) {
    $files = Get-ChildItem $p | where { $_.Extension -match "\.(exe|bat)$" }
    foreach ($f in $files) {
      if ($f.Extension -match "\.exe$") {
        if (Test-Stream $f.FullName "TargetPath") {
          $t = Get-Content $f.FullName -Stream "TargetPath"
          $a = Test-Stream $f.FullName "Async"
          [PSCustomObject]@{
            Name = $f.Name
            FullName = $f.FullName
            TargetPath = $t
            Async = $a
          }
        }
      }
      else {
        $content = Get-Content -Total 4 $f.FullName
        if ($content -match $GUID) {
          $m = ([regex]'^"([^"]+)"').Match($content[3])
          [PSCustomObject]@{
            Name = $f.Name
            FullName = $f.FullName
            TargetPath = $m.Groups[1].Value
          }
        }
      }
    }
  }
}
