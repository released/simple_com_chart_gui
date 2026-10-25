$ErrorActionPreference = 'Stop'

function Assert-TestArtifactPath([string]$Root, [string]$Path) {
    $boundary = [IO.Path]::GetFullPath($Root).TrimEnd('\')
    $absolute = [IO.Path]::GetFullPath($Path)
    if ($absolute -ne $boundary -and -not $absolute.StartsWith($boundary + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Test artifact escaped its owned root.' }
    $current = $absolute
    while ($current) {
        if ((Test-Path -LiteralPath $current) -and ((Get-Item -LiteralPath $current -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'Test artifact reparse point rejected.' }
        $parent = [IO.Directory]::GetParent($current)
        if ($null -eq $parent) { break }
        $current = $parent.FullName
    }
}

function Get-TestArtifactFiles([string]$Root) {
    $pending = [Collections.Generic.Stack[string]]::new()
    $pending.Push($Root)
    while ($pending.Count) {
        $directory = $pending.Pop()
        Assert-TestArtifactPath $Root $directory
        foreach ($item in Get-ChildItem -LiteralPath $directory -Force) {
            Assert-TestArtifactPath $Root $item.FullName
            if ($item.PSIsContainer) { $pending.Push($item.FullName) } else { $item }
        }
    }
}

function New-VerificationRun([string]$ProjectRoot, [string]$Key, [string]$Source) {
    if ($Key -notmatch '^[A-Za-z0-9-]+$') { throw 'Invalid verification key.' }
    $verify = [IO.Path]::GetFullPath((Join-Path $ProjectRoot 'build\verify'))
    $token = [guid]::NewGuid().ToString('N')
    $root = Join-Path $verify ("runs\$Key-$token")
    Assert-TestArtifactPath $verify $root
    if (Test-Path -LiteralPath $root) { throw 'Verification run directory must be fresh.' }
    [void](New-Item -ItemType Directory -Path $root)
    $run = [pscustomobject]@{schema=1;token=$token;key=$Key;root=$root;verify=$verify;source=[IO.Path]::GetFullPath($Source);source_sha256=(Get-FileHash -LiteralPath $Source -Algorithm SHA256).Hash;utc=[DateTime]::UtcNow.ToString('o')}
    $run | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $root 'run-owner.json') -Encoding UTF8
    return $run
}

function Remove-RegisteredTestFiles([string]$Root, $Records) {
    foreach ($record in $Records) {
        $path = [IO.Path]::GetFullPath((Join-Path $Root $record.path))
        Assert-TestArtifactPath $Root $path
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { continue }
        if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $record.sha256) { throw "Changed test artifact preserved: $path" }
    }
    foreach ($record in $Records) {
        $path = [IO.Path]::GetFullPath((Join-Path $Root $record.path))
        Assert-TestArtifactPath $Root $path
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { continue }
        if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $record.sha256) { throw "Test artifact changed during cleanup: $path" }
        Remove-Item -LiteralPath $path
    }
}

function Complete-VerificationRun($Run, [bool]$Passed, [string]$Failure) {
    Assert-TestArtifactPath $Run.verify $Run.root
    $owner = Get-Content -LiteralPath (Join-Path $Run.root 'run-owner.json') -Raw | ConvertFrom-Json
    if ($owner.token -cne $Run.token -or $owner.schema -ne 1 -or $owner.root -ne $Run.root) { throw 'Verification ownership mismatch.' }
    if (Get-CimInstance Win32_Process | Where-Object { $_.ExecutablePath -and $_.ExecutablePath.StartsWith($Run.root + '\', [StringComparison]::OrdinalIgnoreCase) }) { throw 'Test-owned process is still active; artifacts preserved.' }
    $outcome = if ($Passed) { 'last-pass' } else { 'last-failure' }
    $evidence = Join-Path $Run.verify ("evidence\" + $Run.key + "\$outcome")
    Assert-TestArtifactPath $Run.verify $evidence
    $previous = $null
    $registry = Join-Path $evidence 'artifact-retention.json'
    if (Test-Path -LiteralPath $evidence) {
        if (-not (Test-Path -LiteralPath $registry -PathType Leaf)) { throw 'Unregistered evidence directory preserved.' }
        $previous = Get-Content -LiteralPath $registry -Raw | ConvertFrom-Json
        if ($previous.schema -ne 1 -or $previous.key -ne $Run.key) { throw 'Evidence ownership mismatch.' }
    }
    $files = @(Get-TestArtifactFiles $Run.root | Where-Object FullName -ne (Join-Path $Run.root 'cleanup-plan.json'))
    $records = @()
    foreach ($file in $files) {
        $relative = $file.FullName.Substring($Run.root.Length + 1)
        $keep = $file.Name -eq 'run-owner.json' -or $file.Name -eq 'SimpleComChart.ini' -or $file.Name -match '(?i)(result|metrics|recipe).*(\.txt|\.json)$' -or $file.Extension -in @('.png', '.jpg')
        $records += [pscustomobject]@{path=$relative;bytes=$file.Length;sha256=(Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash;retained=($keep -and $file.Length -le 2MB)}
    }
    # Persist exact ownership before mutation; a crash leaves a reviewable plan.
    $records | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $Run.root 'cleanup-plan.json') -Encoding UTF8
    $kept = @($records | Where-Object retained)
    foreach ($record in $kept) {
        $destination = Join-Path $evidence $record.path
        Assert-TestArtifactPath $evidence $destination
        if (Test-Path -LiteralPath $destination) {
            $old = @($previous.retained | Where-Object path -eq $record.path)
            if ($old.Count -ne 1 -or (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash -cne $old[0].sha256) { throw 'Unknown or modified evidence preserved; refusing overwrite.' }
        }
    }
    [void](New-Item -ItemType Directory -Path $evidence -Force)
    foreach ($record in $kept) {
        $destination = Join-Path $evidence $record.path
        [void](New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force)
        Copy-Item -LiteralPath (Join-Path $Run.root $record.path) -Destination $destination -Force
        if ((Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash -cne $record.sha256) { throw 'Evidence copy verification failed.' }
    }
    if ($previous) {
        $obsolete = @($previous.retained | Where-Object { $_.path -notin $kept.path })
        Remove-RegisteredTestFiles $evidence $obsolete
    }
    $receipt = [pscustomobject]@{schema=1;key=$Run.key;run=$Run;passed=$Passed;failure=$Failure;generated_bytes=($records | Measure-Object bytes -Sum).Sum;retained_bytes=($kept | Measure-Object bytes -Sum).Sum;retained=$kept;disposed=$records;cleanup_complete=$false}
    $receipt | ConvertTo-Json -Depth 7 | Set-Content -LiteralPath $registry -Encoding UTF8
    Remove-RegisteredTestFiles $Run.root $records
    # Only these two runner-created metadata files were added after registration.
    Remove-Item -LiteralPath (Join-Path $Run.root 'cleanup-plan.json')
    $directories = @(Get-ChildItem -LiteralPath $Run.root -Recurse -Directory -Force | Sort-Object { $_.FullName.Length } -Descending)
    foreach ($directory in @($directories | ForEach-Object FullName) + @($Run.root)) {
        Assert-TestArtifactPath $Run.root $directory
        if (@(Get-ChildItem -LiteralPath $directory -Force).Count -eq 0) { Remove-Item -LiteralPath $directory }
    }
    $receipt.cleanup_complete = -not (Test-Path -LiteralPath $Run.root)
    $receipt | ConvertTo-Json -Depth 7 | Set-Content -LiteralPath $registry -Encoding UTF8
    if (-not $receipt.cleanup_complete) { throw 'Unregistered files remain in the run; preserved for review.' }
    Write-Output "Test artifacts: disposed $($receipt.generated_bytes) run bytes; retained $($receipt.retained_bytes) evidence bytes at $evidence"
}
