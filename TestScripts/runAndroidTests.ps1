param(
    $PathToGradlewBat = "E:\Hackathons\VulkanBufferThings\Vulkan-Samples\build\android_gradle\gradlew.bat",
    $PathToAdb = "C:\Program Files (x86)\Android\android-sdk\platform-tools\adb.exe",
    [Parameter(Mandatory=$true)][ValidateSet("Build", "Deploy", "Run", "BuildAndDeploy", "DeployAndRun", "PullTestData", "FullBuild", "BuildAndTest", "TestWithoutBuild", "Kill")] $Operation
)

function Build()
{
    & $PathToGradlewBat assembleRelease
}

function Deploy()
{
    & $PathToAdb install ".\app\build\outputs\apk\release\vulkan_samples-release.apk"
}

function Run()
{
    & $PathToAdb shell am start -n com.khronos.vulkan_samples/com.khronos.vulkan_samples.SampleLauncherActivity -e sample hack_dynamic_uniform_buffer
}

function KillTestApp()
{
    & $PathToAdb shell am force-stop com.khronos.vulkan_samples
}

function PullTestData()
{
    $timestamp = Get-Date -Format "yyyy-MM-dd_hh-mm-ss"

    if (-not (Test-Path "$PSScriptRoot/TestResults")) { mkdir "$PSScriptRoot/TestResults" | Out-Null }
    if (-not (Test-Path "$PSScriptRoot/TestResults/$timestamp")) { mkdir "$PSScriptRoot/TestResults/$timestamp" | Out-Null }

    & $PathToAdb shell "run-as com.khronos.vulkan_samples cat /data/data/com.khronos.vulkan_samples/files/data.json" > "$PSScriptRoot/TestResults/$timestamp/data.json"
}

try
{
    Push-Location
    Set-Location (Split-Path $PathToGradlewBat)

    if ($Operation -eq "Build")
    {
        Build
    }

    if ($Operation -eq "BuildAndDeploy")
    {
        Build
        Deploy
    }

    if ($Operation -eq "Deploy")
    {
        Deploy
    }

    if ($Operation -eq "DeployAndRun")
    {
        Deploy
        Run
    }

    if ($Operation -eq "Run")
    {
        Run
    }

    if ($Operation -eq "FullBuild")
    {
        Build
        Deploy
        Run
    }

    if ($Operation -eq "BuildAndTest")
    {
        Build
        Deploy
        Run
        Start-Sleep -Seconds 30
        KillTestApp
        PullTestData
    }

    if ($Operation -eq "TestWithoutBuild")
    {
        Run
        Start-Sleep -Seconds 30
        KillTestApp
        PullTestData
    }

    if ($Operation -eq "PullTestData")
    {
        PullTestData
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
