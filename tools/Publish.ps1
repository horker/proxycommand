$key = cat private\NugetApiKey.txt

Publish-Module -Path $PSScriptRoot\..\ProxyCommand -NugetApiKey $key -Verbose
