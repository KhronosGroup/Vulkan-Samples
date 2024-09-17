param(
    $PathToCmake = "cmake",
    [Parameter(Mandatory=$true)][ValidateSet("Build", "Run", "BuildAndRun", "BuildAndTest", "TestWithoutBuild", "SaveTestData", "Kill")] $Operation
)

function Build()
{
    Push-Location
    Set-Location "$PSScriptRoot/.."
    & $PathToCmake --build build/windows --config relwithdebinfo --target vulkan_samples
    Pop-Location
}

function Run()
{
    Start-Process -FilePath "$PSScriptRoot/../build/windows/app/bin/relwithdebinfo/AMD64/vulkan_samples.exe" -ArgumentList @("sample", "hack_dynamic_uniform_buffer") -WorkingDirectory "$PSScriptRoot/.."
}

function KillTestApp()
{
    Get-Process "vulkan_samples" | Stop-Process
}

function WaitForTestFinish()
{
    while (-not (Test-Path "$PSScriptRoot/../data.json"))
    {
        Start-Sleep -Milliseconds 10
    }
}

function SaveTestData()
{
    $timestamp = Get-Date -Format "yyyy-MM-dd_hh-mm-ss"

    if (-not (Test-Path "$PSScriptRoot/TestResults")) { mkdir "$PSScriptRoot/TestResults" | Out-Null }
    if (-not (Test-Path "$PSScriptRoot/TestResults/$timestamp")) { mkdir "$PSScriptRoot/TestResults/$timestamp" | Out-Null }

    Move-Item "$PSScriptRoot/../data.json" "$PSScriptRoot/TestResults/$timestamp/data.json"
}

try
{
    Push-Location
    Set-Location $PSScriptRoot
    
    if ($Operation -eq "Build")
    {
        Build
    }

    if ($Operation -eq "Run")
    {
        Run
    }

    if ($Operation -eq "BuildAndRun")
    {
        Build
        Run
    }

    if ($Operation -eq "BuildAndTest")
    {
        Build
        Run
        WaitForTestFinish
        KillTestApp
        SaveTestData
    }

    if ($Operation -eq "TestWithoutBuild")
    {
        Run
        WaitForTestFinish
        KillTestApp
        SaveTestData
    }

    if ($Operation -eq "SaveTestData")
    {
        SaveTestData
    }

    if ($Operation -eq "Kill")
    {
        KillTestApp
    }
}
finally
{
    Pop-Location
}