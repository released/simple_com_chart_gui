# Git-only publication helpers. Application builds and personal runtime data are separate.
function Invoke-PcGit {
    param([string]$Root, [string[]]$Arguments, [int[]]$Allowed = @(0))
    foreach ($variable in @('GIT_DIR','GIT_WORK_TREE','GIT_COMMON_DIR','GIT_INDEX_FILE','GIT_OBJECT_DIRECTORY','GIT_ALTERNATE_OBJECT_DIRECTORIES','GIT_NAMESPACE','GIT_CONFIG_PARAMETERS')) {
        if ([Environment]::GetEnvironmentVariable($variable)) { throw "Git environment override refused: $variable" }
    }
    if ($env:GIT_CONFIG_COUNT -and $env:GIT_CONFIG_COUNT -ne '0') { throw 'Git environment override refused: GIT_CONFIG_COUNT' }
    $info = New-Object Diagnostics.ProcessStartInfo
    $info.FileName = (Get-Command git -CommandType Application -ErrorAction Stop).Source
    $info.Arguments = (@('--literal-pathspecs','-C',$Root,'-c','core.quotepath=false') + $Arguments | ForEach-Object {
        '"' + [regex]::Replace([regex]::Replace($_,'(\\*)"','$1$1\"'),'(\\+)$','$1$1') + '"'
    }) -join ' '
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    $info.StandardOutputEncoding = New-Object Text.UTF8Encoding($false)
    $info.StandardErrorEncoding = New-Object Text.UTF8Encoding($false)
    $info.EnvironmentVariables['GIT_TERMINAL_PROMPT'] = '0'
    $info.EnvironmentVariables['GCM_INTERACTIVE'] = 'Never'
    $info.EnvironmentVariables['GIT_OPTIONAL_LOCKS'] = '0'
    $process = New-Object Diagnostics.Process
    $process.StartInfo = $info
    try {
        [void]$process.Start()
        $output = $process.StandardOutput.ReadToEndAsync()
        $errors = $process.StandardError.ReadToEndAsync()
        $process.WaitForExit()
        $result = @{ Code=$process.ExitCode; Text=$output.Result.TrimEnd(); Error=$errors.Result.TrimEnd() }
        if ($result.Code -notin $Allowed) { throw "Git $($Arguments[0]) failed in $Root : $($result.Error)" }
        return $result
    } finally { $process.Dispose() }
}

function Assert-PcPath([string]$Root, [string]$Relative) {
    $rootPath = [IO.Path]::GetFullPath($Root).TrimEnd('\')
    $path = if ([IO.Path]::IsPathRooted($Relative)) { [IO.Path]::GetFullPath($Relative) } else { [IO.Path]::GetFullPath((Join-Path $rootPath $Relative)) }
    if ($path -ne $rootPath -and !$path.StartsWith($rootPath+'\',[StringComparison]::OrdinalIgnoreCase)) { throw "Path escaped repository: $Relative" }
    $part = $path
    while ($part) {
        if (Test-Path -LiteralPath $part) {
            if ((Get-Item -LiteralPath $part -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Reparse path refused: $Relative" }
        }
        if ($part -eq $rootPath) { break }
        $part = Split-Path -Parent $part
    }
    return $path
}

function Get-PcRemoteKey([string]$Remote) {
    if ($Remote -match '^https://github\.com/([A-Za-z0-9_.-]+)/([A-Za-z0-9_.-]+?)(?:\.git)?/?$' -or
        $Remote -match '^git@github\.com:([A-Za-z0-9_.-]+)/([A-Za-z0-9_.-]+?)(?:\.git)?/?$' -or
        $Remote -match '^ssh://git@github\.com/([A-Za-z0-9_.-]+)/([A-Za-z0-9_.-]+?)(?:\.git)?/?$') {
        return ($Matches[1]+'/'+$Matches[2]).ToLowerInvariant()
    }
    # The core also supports explicit local bare remotes for isolated Git fixtures.
    # The public entry validates and pins GitHub-only destinations separately.
    if ([IO.Path]::IsPathRooted($Remote)) { return [IO.Path]::GetFullPath($Remote).TrimEnd('\').ToLowerInvariant() }
    throw 'Unsupported origin URL; embedded credentials and indirect schemes are refused.'
}

function Get-PcIndex([string]$Root) {
    $entries = @{}
    foreach ($record in (Invoke-PcGit $Root @('ls-files','--stage','-z')).Text.Split([char]0)) {
        if (!$record) { continue }
        $parts = $record.Split(@([char]9),2)
        $meta = $parts[0].Split(' ')
        if ($meta[2] -ne '0' -or $meta[0] -notin @('100644','100755')) { throw 'Unmerged, symlink or submodule index entries require separate review.' }
        $entries[$parts[1]] = @{ Mode=$meta[0]; Sha=$meta[1] }
    }
    foreach ($record in (Invoke-PcGit $Root @('ls-files','-v','-z')).Text.Split([char]0)) {
        if ($record -and $record[0] -cne 'H') { throw 'Skip-worktree/assume-unchanged index flags require separate review.' }
    }
    return $entries
}

function Assert-PcRepository($Target) {
    [void](Assert-PcPath $Target.Path '.')
    $gitDirectory = Assert-PcPath $Target.Path '.git'
    if (!(Test-Path -LiteralPath $gitDirectory -PathType Container)) { throw 'Expected a dedicated repository with its own .git directory.' }
    if ((Invoke-PcGit $Target.Path @('rev-parse','--show-toplevel')).Text.Replace('/','\').TrimEnd('\') -ine $Target.Path.TrimEnd('\')) { throw "Expected repository root: $($Target.Name)" }
    if ((Invoke-PcGit $Target.Path @('branch','--show-current')).Text -cne $Target.Branch) { throw "Expected branch $($Target.Branch): $($Target.Name)" }
    $expected = Get-PcRemoteKey $Target.Remote
    foreach ($gitArguments in @(@('remote','get-url','--all','origin'),@('remote','get-url','--push','--all','origin'))) {
        $urls = @((Invoke-PcGit $Target.Path $gitArguments).Text -split '\r?\n' | Where-Object { $_ })
        if ($urls.Count -ne 1 -or (Get-PcRemoteKey $urls[0]) -cne $expected) { throw "Origin mismatch: $($Target.Name)" }
    }
    if ((Invoke-PcGit $Target.Path @('config','--bool','--get','remote.origin.mirror') @(0,1)).Text -eq 'true') { throw 'Mirror publication is refused.' }
    $trackingRemote = (Invoke-PcGit $Target.Path @('config','--get',('branch.'+$Target.Branch+'.remote')) @(0,1)).Text
    $trackingRef = (Invoke-PcGit $Target.Path @('config','--get',('branch.'+$Target.Branch+'.merge')) @(0,1)).Text
    if (($trackingRemote -and $trackingRemote -cne 'origin') -or ($trackingRef -and $trackingRef -cne ('refs/heads/'+$Target.Branch))) { throw "Unexpected upstream: $($Target.Name)" }
    foreach ($name in @('index.lock','MERGE_HEAD','CHERRY_PICK_HEAD','REVERT_HEAD','rebase-merge','rebase-apply')) {
        $path = (Invoke-PcGit $Target.Path @('rev-parse','--git-path',$name)).Text
        if (![IO.Path]::IsPathRooted($path)) { $path = Join-Path $Target.Path $path }
        if (Test-Path -LiteralPath $path) { throw "Active Git operation $name : $($Target.Name)" }
    }
    if ((Invoke-PcGit $Target.Path @('diff','--cached','--name-only')).Text) { throw "Existing staged changes require review: $($Target.Name)" }
    foreach ($key in @('user.name','user.email')) {
        if (!(Invoke-PcGit $Target.Path @('config','--get',$key) @(0,1)).Text) { throw "Missing Git $key : $($Target.Name)" }
    }
    foreach ($file in $Target.IdentityFiles) {
        [void](Assert-PcPath $Target.Path $file)
        [void](Invoke-PcGit $Target.Path @('cat-file','-e',('HEAD:'+$file)))
    }
}

function Get-PcForbiddenReason([string]$Path, [bool]$Tracked, $DeliveryFiles) {
    if ($Path -match '^(?i:AGENTS(?:_for_Codex)?|CLAUDE|WORKSPACE_RULES|HANDOFF|CODEX_HANDOFF|SELF_CHECK|MANUAL_CLEANUP|DEVELOPMENT_CHECKLIST)\.md$' -or
        $Path -match '^docs/(verification|development|internal)/') { return 'local engineering record or generated verification evidence' }
    if ($Path -match '(^|/)(\.git|\.vs|\.venv|venv|node_modules|__pycache__|runtime|DATA|captures|recordings|sessions|logs|private|tmp|artifacts|firmware)(/|$)') { return 'private/runtime/test/firmware directory' }
    if ($Path -match '(^|/)(obj|verify|verify_obj|CMakeFiles|x64|Debug|Release)(/|$)') { return 'generated build directory' }
    if ($Path -match '(?i)\.(obj|pch|idb|pdb|ilk|res|iobj|ipdb|tlog|lastbuildstate|pyc|pyo|db|sqlite|sqlite3|jsonl|log|dmp|partial|bin|idx|sccprofile|sccnotes|sccsession|sccmeta|scccommands|sccsteps|scccases|sctproj|scteffect)$') { return 'private state or compiler/test payload' }
    if ($Path -match '(^|/)(auth\.json|credentials[^/]*|secrets?\.[^/]+|\.env(?:\.[^/]+)?|github_push\.local\.psd1)$') { return 'local configuration or credentials' }
    if ($Path -match '\.ini$' -and [IO.Path]::GetFileName($Path) -notin @('lang_en.ini','lang_tw.ini')) { return 'personal INI' }
    if ($Path -match '^build/' -and (!$Tracked -or $Path -notin $DeliveryFiles)) { return 'only explicitly listed, already tracked delivery files may be updated' }
    if (!$Tracked -and $Path -match '\.(exe|dll|lib|zip|7z|mp4|wav)$') { return 'new binary/archive/media requires separate review' }
    return ''
}

function Get-PcSnapshot($Target) {
    Assert-PcRepository $Target
    $index = Get-PcIndex $Target.Path
    $new = @((Invoke-PcGit $Target.Path @('ls-files','--others','--exclude-standard','-z')).Text.Split([char]0) | Where-Object { $_ })
    $changed = @((Invoke-PcGit $Target.Path @('diff','--no-renames','--name-only','-z','HEAD','--')).Text.Split([char]0) | Where-Object { $_ })
    $pending = @($changed+$new | Sort-Object -Unique)
    $entries = @{}
    foreach ($relative in @(@($index.Keys)+$new | Sort-Object -Unique)) {
        $full = Assert-PcPath $Target.Path $relative
        if (!(Test-Path -LiteralPath $full -PathType Leaf)) { continue }
        $reason = Get-PcForbiddenReason $relative ($index.ContainsKey($relative)) $Target.DeliveryFiles
        if ($reason) { throw "Excluded publication path ($reason): $($Target.Name)/$relative" }
        if ((Get-Item -LiteralPath $full).Length -gt 50MB) { throw "File exceeds the 50 MiB batch limit: $relative" }
    }
    foreach ($relative in $pending) {
        $full = Assert-PcPath $Target.Path $relative
        if (Test-Path -LiteralPath $full -PathType Leaf) {
            if ($relative -match '\.(py|ps1|psd1|json|md|txt|ini|cpp|h|cs|yml|yaml|toml)$' -and (Get-Item -LiteralPath $full).Length -le 4MB) {
                $text = [IO.File]::ReadAllText($full)
                if ($text -match '-----BEGIN (RSA |EC |OPENSSH )?PRIVATE KEY-----|ghp_[A-Za-z0-9]{30,}|github_pat_[A-Za-z0-9_]{40,}|sk-(proj-)?[A-Za-z0-9_-]{24,}') { throw "Possible credential in changed text; values withheld: $relative" }
            }
            $entries[$relative] = @{ Hash=(Get-FileHash -LiteralPath $full -Algorithm SHA256).Hash; Blob=(Invoke-PcGit $Target.Path @('hash-object',('--path='+$relative),'--',$full)).Text; Mode=$(if($index.ContainsKey($relative)){$index[$relative].Mode}else{'100644'}) }
        } else { $entries[$relative] = @{ Hash='deleted'; Blob=''; Mode='' } }
    }
    $head = (Invoke-PcGit $Target.Path @('rev-parse','HEAD')).Text
    $fingerprint = $head+'|'+(($pending | ForEach-Object { $_+':'+$entries[$_].Hash+':'+$entries[$_].Mode }) -join '|')
    return @{ Head=$head; Fingerprint=$fingerprint; Pending=$pending; Entries=$entries; Index=$index }
}

function New-PcPublishPlan($Target) {
    $before = Get-PcSnapshot $Target
    $hook = Join-Path $Target.Path 'scripts\verify_publish.ps1'
    if (Test-Path -LiteralPath $hook) {
        [void](Assert-PcPath $Target.Path 'scripts/verify_publish.ps1')
        & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $hook | ForEach-Object { Write-Host $_ }
        if ($LASTEXITCODE) { throw "Publication hook failed: $($Target.Name)" }
        $after = Get-PcSnapshot $Target
        if ($after.Fingerprint -cne $before.Fingerprint) { throw "Publication hook changed the approved worktree: $($Target.Name)" }
    }
    return @{ Target=$Target; Snapshot=$before }
}

function Assert-PcRemoteState($Target) {
    Assert-PcRepository $Target
    $ref = 'refs/heads/'+$Target.Branch
    [void](Invoke-PcGit $Target.Path @('ls-remote','--exit-code','origin',$ref))
    [void](Invoke-PcGit $Target.Path @('fetch','--no-tags','origin',$ref))
    if ((Invoke-PcGit $Target.Path @('merge-base','--is-ancestor','FETCH_HEAD','HEAD') @(0,1)).Code) { throw "Remote is ahead/diverged; reconcile explicitly before publication: $($Target.Name)" }
    foreach ($file in $Target.IdentityFiles) { [void](Invoke-PcGit $Target.Path @('cat-file','-e',('FETCH_HEAD:'+$file))) }
    $readme = (Invoke-PcGit $Target.Path @('show','FETCH_HEAD:README.md')).Text
    $heading = [regex]::Match($readme,'(?m)^# +([^\r\n]+)').Groups[1].Value
    if (!$heading -or $heading.IndexOf($Target.ReadmeTitle,[StringComparison]::OrdinalIgnoreCase) -lt 0) { throw "Live remote project identity mismatch: $($Target.Name)" }
}

function Invoke-PcPublication($Plans, [string]$Message) {
    if ([string]::IsNullOrWhiteSpace($Message)) { throw 'A nonempty commit message is required.' }
    # Check every destination and current tree before staging the first repository.
    foreach ($plan in $Plans) { Assert-PcRemoteState $plan.Target }
    foreach ($plan in $Plans) {
        $now = Get-PcSnapshot $plan.Target
        if ($now.Fingerprint -cne $plan.Snapshot.Fingerprint) { throw "Worktree changed after approval: $($plan.Target.Name)" }
    }
    foreach ($plan in $Plans) {
        $target=$plan.Target; $snapshot=$plan.Snapshot
        if ($snapshot.Pending.Count) {
            [void](Invoke-PcGit $target.Path @('add','--all','--','.'))
            $actual = Get-PcIndex $target.Path
            $expected = @{}
            foreach ($path in $snapshot.Index.Keys) { $expected[$path]=$snapshot.Index[$path] }
            foreach ($path in $snapshot.Pending) {
                if ($snapshot.Entries[$path].Hash -eq 'deleted') { $expected.Remove($path) }
                else { $expected[$path]=@{ Mode=$snapshot.Entries[$path].Mode; Sha=$snapshot.Entries[$path].Blob } }
            }
            if ($actual.Count -ne $expected.Count) { throw "Unexpected staged paths; preserve index for review: $($target.Name)" }
            foreach ($path in $expected.Keys) {
                if (!$actual.ContainsKey($path) -or $actual[$path].Sha -cne $expected[$path].Sha -or $actual[$path].Mode -cne $expected[$path].Mode) { throw "Staged content differs from preview; preserve index for review: $($target.Name)" }
            }
            $tree = (Invoke-PcGit $target.Path @('write-tree')).Text
            [void](Invoke-PcGit $target.Path @('commit','-m',$Message))
            if ((Invoke-PcGit $target.Path @('rev-parse','HEAD^{tree}')).Text -cne $tree -or (Invoke-PcGit $target.Path @('rev-parse','HEAD^')).Text -cne $snapshot.Head) { throw "Commit hook or concurrent change altered the approved commit: $($target.Name)" }
            Write-Host "COMMITTED $($target.Name) $((Invoke-PcGit $target.Path @('rev-parse','HEAD')).Text)"
        }
        if ((Invoke-PcGit $target.Path @('status','--porcelain','--untracked-files=all')).Text) { throw "Worktree changed during publication: $($target.Name)" }
        $plan.PublishHead = (Invoke-PcGit $target.Path @('rev-parse','HEAD')).Text
    }
    foreach ($plan in $Plans) {
        $target=$plan.Target; $ref='refs/heads/'+$target.Branch
        # Revalidate destination config and a clean state; ordinary push still guards remote races.
        Assert-PcRepository $target
        if ((Invoke-PcGit $target.Path @('rev-parse','HEAD')).Text -cne $plan.PublishHead -or (Invoke-PcGit $target.Path @('status','--porcelain','--untracked-files=all')).Text) { throw "Repository changed before push: $($target.Name)" }
        [void](Invoke-PcGit $target.Path @('push','--porcelain','--no-force','--no-follow-tags','--recurse-submodules=no','origin',($plan.PublishHead+':'+$ref)))
        $remote = (Invoke-PcGit $target.Path @('ls-remote','--exit-code','origin',$ref)).Text.Split([char]9)[0]
        if ($remote -cne $plan.PublishHead) { throw "Remote SHA verification failed: $($target.Name)" }
        Write-Host "PUSH VERIFIED $($target.Name) $remote"
    }
}
