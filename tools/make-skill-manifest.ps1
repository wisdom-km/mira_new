param(
    [Parameter(Mandatory = $true)][string]$Id,
    [Parameter(Mandatory = $true)][string]$Version,
    [Parameter(Mandatory = $true)][string]$Dir,
    [string]$Category = "skill",
    [string]$License = "MIT",
    [string]$NameZh = "",
    [string]$NameEn = "",
    [string]$Author = "DirectorDesk Community",
    [string]$UrlPrefix = ""
)

$root = (Resolve-Path -LiteralPath $Dir).Path
$skill = Join-Path $root "SKILL.md"
if (-not (Test-Path -LiteralPath $skill)) {
    throw "missing $skill"
}

function Get-FileSha256Lower([string]$Path) {
    $hash = Get-FileHash -LiteralPath $Path -Algorithm SHA256
    return $hash.Hash.ToLowerInvariant()
}

$files = @()
Get-ChildItem -LiteralPath $root -Recurse -File | Sort-Object FullName | ForEach-Object {
    $rel = $_.FullName.Substring($root.Length).TrimStart('\', '/').Replace('\', '/')
    $urlRel = if ($UrlPrefix) { "$UrlPrefix$Id/$Version/$rel" } else { "skills/$Id/$Version/$rel" }
    $files += [ordered]@{
        path   = $rel
        url    = $urlRel
        sha256 = Get-FileSha256Lower $_.FullName
        size   = $_.Length
    }
}

$name = [ordered]@{}
if ($NameZh) { $name["zh-CN"] = $NameZh }
if ($NameEn) { $name["en"] = $NameEn }
if ($name.Count -eq 0) { $name["en"] = $Id }

$asset = [ordered]@{
    id         = $Id
    version    = $Version
    name       = $name
    category   = $Category
    format     = "skill"
    kind       = "skill"
    entrypoint = "SKILL.md"
    files      = $files
    license    = [ordered]@{ spdx = $License; name = $License }
    author     = [ordered]@{ name = $Author }
}

$asset | ConvertTo-Json -Depth 8
