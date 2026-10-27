# Installs only the Windows Graphics Tools capability requested by the user.
# Run from an elevated PowerShell. It does not change execution policy or drivers.
param(
    [Parameter(Mandatory = $true)]
    [string] $ReportPath
)

$ErrorActionPreference = 'Stop'
$capabilityName = 'Tools.Graphics.DirectX~~~~0.0.1.0'
$result = [ordered]@{
    capability = $capabilityName
    status = 'starting'
    restartNeeded = $false
    error = $null
}

try {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = [Security.Principal.WindowsPrincipal]::new($identity)
    if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
        throw 'Graphics Tools installation requires an administrator.'
    }
    $result | ConvertTo-Json | Set-Content -LiteralPath $ReportPath -Encoding UTF8
    $capability = Get-WindowsCapability -Online -Name $capabilityName
    if ($capability.State -ne 'Installed') {
        $result.status = 'installing'
        $result | ConvertTo-Json | Set-Content -LiteralPath $ReportPath -Encoding UTF8
        $installation = Add-WindowsCapability -Online -Name $capabilityName
        $result.restartNeeded = [bool] $installation.RestartNeeded
    }
    $verified = Get-WindowsCapability -Online -Name $capabilityName
    if ($verified.State -ne 'Installed') {
        throw "Graphics Tools state after installation: $($verified.State)"
    }
    $result.status = 'installed'
    $result | ConvertTo-Json | Set-Content -LiteralPath $ReportPath -Encoding UTF8
    exit 0
} catch {
    $result.status = 'failed'
    $result.error = $_.Exception.Message
    $result | ConvertTo-Json | Set-Content -LiteralPath $ReportPath -Encoding UTF8
    exit 1
}
