<#
Convert a PDF into a Paper OS SD-card book folder on Windows.

One-time setup in PowerShell:
  winget install oschwartz10612.Poppler
  py -m pip install Pillow

Example:
  .\tools\Convert-PdfToPaperS3.ps1 -Pdf "$HOME\Downloads\book.pdf" -Output "E:\PaperOS\books"
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateScript({ Test-Path $_ -PathType Leaf })]
    [string]$Pdf,

    [Parameter(Mandatory = $true)]
    [string]$Output,

    [string]$Title,
    [ValidateRange(72, 300)]
    [int]$Dpi = 150
)

$ErrorActionPreference = 'Stop'

if (-not (Get-Command pdftoppm -ErrorAction SilentlyContinue)) {
    throw "pdftoppm was not found. Install Poppler with: winget install oschwartz10612.Poppler"
}

$Python = Get-Command py -ErrorAction SilentlyContinue
if (-not $Python) {
    $Python = Get-Command python -ErrorAction SilentlyContinue
}
if (-not $Python) {
    throw "Python was not found. Install Python, then run: py -m pip install Pillow"
}

$ScriptPath = Join-Path $PSScriptRoot 'pdf_to_papers3.py'
if (-not (Test-Path $ScriptPath -PathType Leaf)) {
    throw "Shared converter was not found: $ScriptPath"
}

$Arguments = @($ScriptPath, $Pdf, '--output', $Output, '--dpi', $Dpi)
if ($Title) {
    $Arguments += @('--title', $Title)
}

if ($Python.Name -ieq 'py.exe' -or $Python.Name -ieq 'py') {
    & $Python.Source @Arguments
} else {
    & $Python.Source @Arguments
}
if ($LASTEXITCODE -ne 0) {
    throw "PDF conversion failed with exit code $LASTEXITCODE. If Pillow is missing, run: py -m pip install Pillow"
}
