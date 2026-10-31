$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..')).TrimEnd('\')
$build = [IO.Path]::GetFullPath((Join-Path $projectRoot 'build'))
if ([IO.Path]::GetDirectoryName($build) -ne $projectRoot) { throw 'Invalid build root.' }

# Preflight every ancestor and descendant before deleting any compiler files.
$ancestor = $build
while ($ancestor) {
    if (Test-Path -LiteralPath $ancestor) {
        if (((Get-Item -LiteralPath $ancestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "Refusing to traverse a reparse point: $ancestor"
        }
    }
    $ancestor = [IO.Path]::GetDirectoryName($ancestor)
}
if (-not (Test-Path -LiteralPath $build)) { Write-Host 'No build intermediates.'; exit 0 }
$pending = [Collections.Generic.Stack[string]]::new()
$pending.Push($build)
$files = @()
$directories = @()
while ($pending.Count) {
    $item = Get-Item -LiteralPath $pending.Pop() -Force
    if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Refusing to traverse a reparse point: $($item.FullName)"
    }
    if ($item.PSIsContainer) {
        $directories += $item
        foreach ($child in Get-ChildItem -LiteralPath $item.FullName -Force) {
            $pending.Push($child.FullName)
        }
    } else { $files += $item }
}

$objectRoot = Join-Path $build 'obj'
$objectPrefix = $objectRoot + '\'
$generatedRoots = @()
foreach ($leaf in @('cmake', 'cmake-runtime')) {
    $root = Join-Path $objectRoot $leaf
    $cache = Join-Path $root 'CMakeCache.txt'
    if (-not (Test-Path -LiteralPath $cache -PathType Leaf)) { continue }
    $cmakeHomeLine = Get-Content -LiteralPath $cache | Where-Object { $_ -like 'CMAKE_HOME_DIRECTORY:INTERNAL=*' } | Select-Object -First 1
    if (-not $cmakeHomeLine) { throw "Cannot verify CMake producer for $root" }
    $source = [IO.Path]::GetFullPath(($cmakeHomeLine -split '=', 2)[1].Replace('/', '\')).TrimEnd('\')
    if (-not $source.Equals($projectRoot, [StringComparison]::OrdinalIgnoreCase)) {
        throw "CMake tree belongs to another source directory: $root"
    }
    $generatedRoots += $root
}

$targetNames = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$null = $targetNames.Add('SimpleComChart')
$null = $targetNames.Add('SimpleComChartCli')
$null = $targetNames.Add('CompilerIdCXX')
foreach ($root in $generatedRoots) {
    foreach ($project in Get-ChildItem -LiteralPath $root -Filter '*.vcxproj' -File -Force) {
        $null = $targetNames.Add($project.BaseName)
        $match = [regex]::Match((Get-Content -LiteralPath $project.FullName -Raw), '<TargetName>([^<]+)</TargetName>')
        if ($match.Success) { $null = $targetNames.Add($match.Groups[1].Value) }
    }
}

$extensions = @('.obj', '.pch', '.idb', '.pdb', '.ilk', '.res', '.iobj',
    '.ipdb', '.tlog', '.lastbuildstate')
$targets = @(Get-ChildItem -LiteralPath $projectRoot -Filter '*.vcxproj' -File |
    ForEach-Object { $_.BaseName })
$remove = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$cacheFiles = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach ($file in $files) {
    $inObjects = $file.FullName.StartsWith($objectPrefix, [StringComparison]::OrdinalIgnoreCase)
    $extension = $file.Extension.ToLowerInvariant()
    $known = $extension -in $extensions
    if ($extension -in @('.lib', '.exp')) {
        $known = $inObjects -or ($file.DirectoryName -eq $build -and $file.BaseName -in $targets)
    }
    if ($inObjects -and ($extension -in @('.recipe', '.rsp', '.manifest') -or
        $file.Name -like '*.vcxproj.FileListAbsolute.txt')) { $known = $true }
    foreach ($root in $generatedRoots) {
        $prefix = $root.TrimEnd('\') + '\'
        if (-not $file.FullName.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) { continue }
        $relative = $file.FullName.Substring($prefix.Length)
        if ($extension -in @('.vcxproj', '.filters', '.sln', '.cmake', '.yaml', '.rule', '.stamp', '.check_cache', '.depend', '.list')) { $known = $true }
        if ($file.Name -in @('CTestTestfile.cmake', 'cmake_install.cmake', 'ALL_BUILD.vcxproj', 'ZERO_CHECK.vcxproj')) { $known = $true }
        if ($file.Name -eq 'CMakeCache.txt') { $null = $cacheFiles.Add($file.FullName) }
        if ($relative -eq 'CMakeFiles\TargetDirectories.txt' -or $relative -like 'CMakeFiles\*\VCTargetsPath.txt') { $known = $true }
        if ($relative -eq 'generated\build_version.h') { $known = $true }
        if ($extension -eq '.map' -and $file.Name -like 'SimpleComChart-*.map') { $known = $true }
        if ($extension -eq '.bin' -and $relative -like 'CMakeFiles\*\CMakeDetermineCompilerABI_CXX.bin') { $known = $true }
        if ($relative -like 'CMakeFiles\*\CompilerIdCXX\*' -and $extension -in @('.cpp', '.h', '.exe')) { $known = $true }
        if ($relative -like 'Testing\Temporary\*' -and $extension -in @('.log', '.txt')) { $known = $true }
        if ($relative -match '^(Debug|Release|MinSizeRel|RelWithDebInfo)\\([^\\]+)\.exe$' -and $targetNames.Contains($Matches[2])) { $known = $true }
    }
    if ($known) { $null = $remove.Add($file.FullName) }
}

# Runtime-check binaries are private, reproducible verification outputs. Keep
# compact build/verify/evidence receipts, and never infer deletion from a generic
# EXE/INI filename elsewhere.
$runtimeRoot = Join-Path $build 'verify\runtime-bin'
foreach ($configuration in @('Debug', 'Release', 'MinSizeRel', 'RelWithDebInfo')) {
    $folder = Join-Path $runtimeRoot $configuration
    foreach ($leaf in @('SimpleComChart.exe', 'SimpleComChartCli.exe', 'lang_en.ini', 'lang_tw.ini')) {
        $path = Join-Path $folder $leaf
        if (Test-Path -LiteralPath $path -PathType Leaf) { $null = $remove.Add([IO.Path]::GetFullPath($path)) }
    }
}
# Non-Release delivery configurations are reproducible build outputs. The formal
# portable delivery remains the root build\SimpleComChart*.exe and catalogs.
foreach ($configuration in @('Debug', 'MinSizeRel', 'RelWithDebInfo')) {
    $folder = Join-Path $build $configuration
    foreach ($leaf in @('SimpleComChart.exe', 'SimpleComChartCli.exe', 'lang_en.ini', 'lang_tw.ini')) {
        $path = Join-Path $folder $leaf
        if (Test-Path -LiteralPath $path -PathType Leaf) { $null = $remove.Add([IO.Path]::GetFullPath($path)) }
    }
}

$running = @(Get-CimInstance Win32_Process | Where-Object {
    $_.ExecutablePath -and $remove.Contains([IO.Path]::GetFullPath($_.ExecutablePath))
})
if ($running.Count) {
    throw "Refusing to clean running generated executable(s): $($running.ProcessId -join ', ')"
}

$removed = 0
foreach ($path in $remove) {
    Remove-Item -LiteralPath $path -Force
    $removed++
}
# Producer identity is removed last. If an earlier deletion fails, the next clean
# can still validate and resume the same generated tree.
foreach ($path in $cacheFiles) {
    Remove-Item -LiteralPath $path -Force
    $removed++
}
# Never recursively purge a named folder. Even obj/tests may hold user data.
$emptyDirectoryCount = 0
$emptyRemoved=$true
while($emptyRemoved){$emptyRemoved=$false
    $emptyCandidates = @()
    foreach ($root in @($objectRoot, $runtimeRoot)) {
        if (-not (Test-Path -LiteralPath $root -PathType Container)) { continue }
        $emptyCandidates += Get-ChildItem -LiteralPath $root -Directory -Recurse -Force
        $emptyCandidates += Get-Item -LiteralPath $root -Force
    }
    $emptyCandidates += $directories | Where-Object {
        $_.Parent.FullName -eq $build -and $_.Name -in @('Debug', 'MinSizeRel', 'RelWithDebInfo')
    }
    foreach ($directory in $emptyCandidates | Sort-Object { $_.FullName.Length } -Descending) {
        if(-not(Test-Path -LiteralPath $directory.FullName -PathType Container)){continue}
        $children = @(Get-ChildItem -LiteralPath $directory.FullName -Force)
        if ($children.Count -eq 0) {
            Remove-Item -LiteralPath $directory.FullName -Force
            $emptyDirectoryCount++
            $emptyRemoved=$true
        }
    }
}
Write-Host "Clean complete: $removed allowlisted compiler/CMake/test files removed."
Write-Host "Clean complete: $emptyDirectoryCount empty generated directories removed."
Write-Host 'Preserved EXE/INI/DLL/firmware/data/log/evidence/unknown files, including nested delivery folders.'
