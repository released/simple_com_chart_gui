$ErrorActionPreference='Stop'
$source=Join-Path (Split-Path -Parent $PSScriptRoot) 'scripts\verify_review_contracts.ps1'
$tokens=$null;$errors=$null
$ast=[Management.Automation.Language.Parser]::ParseFile($source,[ref]$tokens,[ref]$errors)
if($errors.Count){throw 'Runtime driver syntax error'}
foreach($name in @('Get-RuntimeBindingInputs','Assert-RuntimeReceipt')){
    $function=$ast.FindAll({param($node) $node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -ceq $name},$true)
    if($function.Count -ne 1){throw ('Missing unique function: '+$name)}
    . ([scriptblock]::Create($function[0].Extent.Text))
}
function Inputs([string]$driver='driver',[string]$test='test',[string]$delivery='delivery'){
    @(@{path='src/message_lens.cpp';sha256='source'},@{path='scripts/verify_review_contracts.ps1';sha256=$driver},@{path='tests/discovery_tests.cpp';sha256=$test},@{path='scripts/verify_delivery.ps1';sha256=$delivery})
}
function Equal-Binding($a,$b){((Get-RuntimeBindingInputs $a)|ConvertTo-Json -Compress) -ceq ((Get-RuntimeBindingInputs $b)|ConvertTo-Json -Compress)}
function Check([bool]$ok,[string]$text){if(!$ok){throw $text};Write-Output ('PASS: '+$text)}
Check (Equal-Binding (Inputs) (Inputs)) 'identical runtime source/driver/test bindings accepted'
Check (!(Equal-Binding (Inputs) (Inputs 'changed'))) 'runtime driver parameter or selection change invalidates old evidence'
Check (!(Equal-Binding (Inputs) (Inputs 'driver' 'changed'))) 'native test change invalidates old evidence'
Check (Equal-Binding (Inputs) (Inputs 'driver' 'test' 'changed')) 'independent delivery probe remains separately verified'
function Receipt([string]$case='discovery-review'){
    $commands=@();foreach($stem in @('discovery-core','message-lens','message-scroll-stability')){$commands+=@{name=$stem+'-current';exit=0;arguments=@()}}
    $results=@();foreach($language in @('en','tw')){$args=@('--smoke-test='+$case);if($language -eq 'tw'){$args+='--font125'};$commands+=@{name=$case+'-'+$language+'-attempt1';exit=0;arguments=$args};$results+=@{name=$case+'-'+$language+'-report';lines=@('PASS: actual owned native controls')}}
    @{version='1.0.1.054';configuration='Release';outcome='PASS';inputs=(Inputs);commands=$commands;results=$results}
}
function Rejected($r,[string]$case='discovery-review'){try{Assert-RuntimeReceipt $r '1.0.1.054' 'Release' $case;return $false}catch{return $true}}
Assert-RuntimeReceipt (Receipt) '1.0.1.054' 'Release' 'discovery-review'
Write-Output 'PASS: affected Review receipt requires core/lens/scroll and actual EN/TW Font125'
$r=Receipt;$r.commands=@($r.commands|Where-Object {$_.name -notlike '*tw-attempt*'})
Check (Rejected $r) 'missing actual TW command rejected'
$r=Receipt;$r.commands=@($r.commands|Where-Object {$_.name -notlike 'message-lens-*'})
Check (Rejected $r) 'missing actual lens command rejected'
$r=Receipt;$r.results[0].lines=@('FAIL: native controls')
Check (Rejected $r) 'failed native report rejected'
$r=Receipt 'discovery'
Check (Rejected $r 'discovery') 'original full Discovery Release still requires successful receive/render soak'
$r.results+=@{name='discovery-en-attempt1-soak-result.txt';lines=@('PASS: UI pump remains responsive in simulated load')}
Assert-RuntimeReceipt $r '1.0.1.054' 'Release' 'discovery'
Write-Output 'PASS: original successful Discovery soak receipt accepted without weakening its gate'
