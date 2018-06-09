# app names
$testApp = "source\x64\Release\TestApp.exe"
$testRoot = "tests\temp"
$proxyRoot = "$testRoot\proxybin"
$proxy = "$proxyRoot\test1.exe"

# test directory
$null = New-Item -Type Directory -Force "tests\temp\proxybin"

Describe "Paths" {

  It "works on the path with white spaces" {
    $target = "$testRoot\path with spaces\TestApp.exe"
    New-Item -Type Directory -Force (Split-Path -Parent $target)
    Copy-Item $testApp $target

    New-ProxyCommand $proxy $target

    $result = . $proxy test

    $result | Should -Be "test"
  }

}

Describe "Parameters" {

  It "passes arguments to the target command" {
    New-ProxyCommand $proxy $testApp

    $result = . $proxy arg1 "arg 2"

    $result | Should -Be "arg1 arg 2"
  }

}

Describe "Redirect" {

  It "can redirect the child process's output to a file" {
    New-ProxyCommand $proxy $testApp

    $file = "$testRoot\test.txt"

    . $proxy test | Set-Content $file

    Get-Content $file | Should -Be "test"
  }

}

Describe "Async" {

  It "doesn't add the Async data stream unless -Async is specified" {
    New-ProxyCommand $proxy $testApp

    { Get-Content $proxy -Stream Async -EA Stop } | Should -Throw
  }

#  It "works asynchronously when -Async is specified" {
#    $app = "C:\Windows\system32\notepad.exe"
#    New-ProxyCommand $proxy $app -Async
#
#    Get-Content $proxy -Stream Async | Should -Not -BeNullOrEmpty
#
#    . $proxy
#
#    $p = Get-Process $app -EA SilentlyContinue
#    $p | Should -Not -Be $null
#
#    $p.Close()
#  }

}

Describe "Batch file" {

  It "accept a batch file" {
    $testBatch = "$testRoot\test.bat"
    "@echo off`necho test" | Set-Content $testBatch

    New-ProxyCommand $proxyRoot $testBatch

    $result = . $testBatch

    $result | Should -Be "test"
  }

}
