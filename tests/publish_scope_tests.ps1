[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$projectRoot=Split-Path -Parent $PSScriptRoot
. (Join-Path $projectRoot 'scripts\pc_tool_publish.ps1')
. (Join-Path (Split-Path -Parent $projectRoot) '#READ_ME_FIRST#\scripts\invoke_hub_test.ps1')
$script:scopeChecks=New-Object 'Collections.Generic.List[string]'
function Check-Scope($ok,[string]$name){if(!$ok){throw $name};$script:scopeChecks.Add('PASS: '+$name)}
function Reject-Scope([scriptblock]$action,[string]$name){$rejected=$false;try{& $action}catch{$rejected=$true};Check-Scope $rejected $name}
$result=Invoke-HubOwnedTest -ProjectRoot $projectRoot -Purpose 'CR1-CR5 local publisher scope fixtures' -MaxBytes 32MB -Action {
    param($run)
    $repo=Join-Path $run.Work 'repo';$remote=Join-Path $run.Work 'remote.git'
    [void][IO.Directory]::CreateDirectory($repo)
    [void](Invoke-PcGit $repo @('init','--bare',$remote))
    [void](Invoke-PcGit $repo @('init','-b','main'))
    [void](Invoke-PcGit $repo @('config','user.name','Scope Fixture'))
    [void](Invoke-PcGit $repo @('config','user.email','fixture@example.invalid'))
    [IO.File]::WriteAllText((Join-Path $repo 'README.md'),'# Scope fixture')
    [IO.File]::WriteAllText((Join-Path $repo '.gitignore'),"task.json`nprivate.ini`n")
    [IO.File]::WriteAllText((Join-Path $repo 'source.txt'),'before')
    [IO.File]::WriteAllText((Join-Path $repo 'removed.txt'),'owned obsolete source')
    [void](Invoke-PcGit $repo @('add','--','README.md','.gitignore','source.txt','removed.txt'))
    [void](Invoke-PcGit $repo @('commit','-m','Seed local fixture'))
    [void](Invoke-PcGit $repo @('remote','add','origin',$remote))
    [void](Invoke-PcGit $repo @('push','-u','origin','main'))
    $base=(Invoke-PcGit $repo @('rev-parse','HEAD')).Text
    [IO.File]::WriteAllText((Join-Path $repo 'source.txt'),'after')
    [IO.File]::WriteAllText((Join-Path $repo 'literal[1].txt'),'literal wildcard name')
    Remove-Item -LiteralPath (Assert-HubPath $run.Work (Join-Path $repo 'removed.txt'))
    [IO.File]::WriteAllText((Join-Path $repo 'private.ini'),'private fixture, must remain untracked')
    $target=@{Name='scope-fixture';Path=$repo;Remote=$remote;Branch='main';IdentityFiles=@('README.md');ReadmeTitle='Scope fixture';DeliveryFiles=@()}
    $plan=New-PcPublishPlan $target
    $manifest=@{schema=1;base_head=$base;files=@($plan.Snapshot.Pending|ForEach-Object {@{path=$_;sha256=$plan.Snapshot.Entries[$_].Hash}})}
    $path=Join-Path $repo 'task.json'
    [IO.File]::WriteAllText($path,($manifest|ConvertTo-Json -Depth 4))
    Assert-PcTaskScope $target $plan $path
    Check-Scope ($plan.RequiredRemoteHead -ceq $base) 'exact owned scope pins the remote baseline'
    $bad=($manifest|ConvertTo-Json -Depth 4)|ConvertFrom-Json;$bad.files[0].sha256='wrong'
    [IO.File]::WriteAllText($path,($bad|ConvertTo-Json -Depth 4))
    Reject-Scope {Assert-PcTaskScope $target $plan $path} 'changed manifest content is rejected'
    [IO.File]::WriteAllText($path,($manifest|ConvertTo-Json -Depth 4))
    [IO.File]::WriteAllText((Join-Path $repo 'unowned.txt'),'not in approved scope')
    $mixed=New-PcPublishPlan $target
    Reject-Scope {Assert-PcTaskScope $target $mixed $path} 'unowned dirty file stops scoped publication'
    Reject-Scope {Invoke-PcPublication @($plan) 'Must not commit changed worktree'} 'late worktree change stops publication before staging'
    Check-Scope (!(Invoke-PcGit $repo @('diff','--cached','--name-only')).Text -and (Invoke-PcGit $repo @('rev-parse','HEAD')).Text -ceq $base) 'rejections preserve index and HEAD'
    Remove-Item -LiteralPath (Assert-HubPath $run.Work (Join-Path $repo 'unowned.txt'))
    $plan=New-PcPublishPlan $target;Assert-PcTaskScope $target $plan $path
    $plan.RequiredRemoteHead='0000000000000000000000000000000000000000'
    Reject-Scope {Invoke-PcPublication @($plan) 'Must not commit wrong remote baseline'} 'remote baseline mismatch prevents commit'
    Assert-PcTaskScope $target $plan $path
    Invoke-PcPublication @($plan) 'Publish exact local fixture scope'
    $changed=@((Invoke-PcGit $repo @('diff-tree','--no-commit-id','--name-only','-r','HEAD')).Text -split '\r?\n')
    Check-Scope (($changed|Sort-Object) -join '|' -ceq 'literal[1].txt|removed.txt|source.txt') 'commit contains only exact literal additions/changes/deletions'
    Check-Scope (!(Invoke-PcGit $repo @('ls-files','private.ini','task.json')).Text -and [IO.File]::ReadAllText((Join-Path $repo 'private.ini')) -ceq 'private fixture, must remain untracked') 'private INI and scope manifest stay local unchanged'
    $remoteHead=(Invoke-PcGit $repo @('ls-remote','origin','refs/heads/main')).Text.Split([char]9)[0]
    Check-Scope ((Invoke-PcGit $repo @('rev-parse','HEAD')).Text -ceq $remoteHead) 'local bare remote matches the published commit'
} -CollectArtifacts {
    param($run)
    Register-HubArtifactTree -Run $run -Producer 'Fresh local Git/bare fixtures and synthetic task scopes; no network'
} -Evidence {
    param($run,$outcome)
    [IO.File]::WriteAllText((Join-Path $projectRoot 'docs\verification\publish-scope-latest.json'),(@{outcome=$outcome;checks=$script:scopeChecks.ToArray()}|ConvertTo-Json -Depth 4),(New-Object Text.UTF8Encoding($false)))
}
$script:scopeChecks|Write-Output
$result|Select-Object Success,Outcome,CleanupRemoved,Errors|ConvertTo-Json -Depth 4
if(!$result.Success){throw 'Local publisher scope checks or cleanup failed'}
