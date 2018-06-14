task . Compile, Build, ImportDebug, Test

Set-StrictMode -Version 4

############################################################

$SOLUTION_FILE = "$PSScriptRoot\source\ProxyCommand.sln"

$OBJECT_PATH = "$PSScriptRoot\source\x64"
$SCRIPT_PATH = "$PSScriptRoot\scripts"

$OBJECT_FILES = @(
  "ProxyCommand.exe"
  "ProxyCommand.pdb"
)

$MODULE_PATH = "$PSScriptRoot\ProxyCommand"
$MODULE_PATH_DEBUG = "$PSScriptRoot\debug\ProxyCommand"

############################################################

function New-Folder2 {
  param(
    [string]$Path
  )

  try {
    $null = New-Item -Type Directory $Path -EA Stop
    Write-Host -ForegroundColor DarkCyan "$Path created"
  }
  catch {
    Write-Host -ForegroundColor DarkYellow $_
  }
}

function Copy-Item2 {
  param(
    [string]$Source,
    [string]$Dest
  )

  try {
    Copy-Item $Source $Dest -EA Stop
    Write-Host -ForegroundColor DarkCyan "Copy from $Source to $Dest done"
  }
  catch {
    Write-Host -ForegroundColor DarkYellow $_
  }
}

function Remove-Item2 {
  param(
    [string]$Path
  )

  try {
    Remove-Item $Path -EA Stop
    Write-Host -ForegroundColor DarkCyan "$Path removed"
  }
  catch {
    Write-Host -ForegroundColor DarkYellow $_
  }
}

############################################################

task Compile {
  msbuild $SOLUTION_FILE /nologo /v:minimal /p:Configuration=Debug
  msbuild $SOLUTION_FILE /nologo /v:minimal /p:Configuration=Release
}

task Build {
  . {
    $ErrorActionPreference = "Continue"

    function Copy-ObjectFiles {
      param(
        [string]$targetPath,
        [string]$objectPath
      )
      New-Folder2 $targetPath

      Copy-Item2 "$SCRIPT_PATH\*" $targetPath
      $OBJECT_FILES | foreach {
        $path = Join-Path $objectPath $_
        Copy-Item2 $path $targetPath
      }
    }

    Copy-ObjectFiles $MODULE_PATH "$OBJECT_PATH\Release"
    Copy-ObjectFiles $MODULE_PATH_DEBUG "$OBJECT_PATH\Debug"
  }
}

task Test {
  Invoke-Pester "$PSScriptRoot\tests"
}

task ImportDebug {
  Import-Module $MODULE_PATH_DEBUG -Force
}

task Clean {
  Remove-Item2 "$MODULE_PATH\*" -Force -Recurse -EA Continue
  Remove-Item2 "$MODULE_PATH_DEBUG\*" -Force -Recurse -EA Continue
}
