param (
	[Parameter(Mandatory = $true, Position = 0)][string]$deviceOSPath,
	[Parameter(Mandatory = $false, Position = 1)][string]$platformList,
	[Parameter(Mandatory = $false, Position = 2)][string]$taksList,
	[Parameter(Mandatory = $false, Position = 3)][string]$jobsList
)

function run(){
	if ($args.Count -eq 0) {
		throw "Must supply some arguments."
	}

	$command = $args[0]
	$commandArgs = @()
	if ($args.Count -gt 1) {
		$commandArgs = $args[1..($args.Count - 1)]
	}

	& $command $commandArgs
	$result = $LASTEXITCODE

	if ($result -ne 0) {
		throw "$command $commandArgs exited with code $result."
	}
}

function stashBins($dest){
	$roots = @(
		@{ name = 'deviceos'; path = Join-Path $deviceOSPath 'build\target' },
		@{ name = 'app'; path = Join-Path $tinkerPath 'target' }
	)
	$count = 0

	foreach ($root in $roots){
		if (!(Test-Path $root.path)){
			continue
		}

		$base = (Resolve-Path $root.path).Path
		foreach ($f in Get-ChildItem -LiteralPath $base -Recurse -File -Filter '*.bin'){
			$rel = $f.FullName.Substring($base.Length).TrimStart('\')
			$target = Join-Path (Join-Path $dest $root.name) $rel
			New-Item -ItemType Directory -Force -Path (Split-Path $target) | Out-Null
			Move-Item -LiteralPath $f.FullName -Destination $target
			$count++
		}
	}

	if ($count -eq 0){
		throw "No binaries found after the build!"
	}
}

function hashBins($dir){
	$base = (Resolve-Path $dir).Path
	$hashes = @{}

	foreach ($f in Get-ChildItem -LiteralPath $base -Recurse -File){
		$hashes[$f.FullName.Substring($base.Length)] = (Get-FileHash -Algorithm SHA256 -LiteralPath $f.FullName).Hash
	}

	return $hashes
}

function compareBins($expectedDir, $actualDir){
	$expected = hashBins $expectedDir
	$actual = hashBins $actualDir
	$mismatches = @()

	foreach ($k in $expected.Keys){
		if ($actual[$k] -ne $expected[$k]){
			$mismatches += $k
		}
	}

	foreach ($k in $actual.Keys){
		if (!$expected.ContainsKey($k)){
			$mismatches += $k
		}
	}

	foreach ($k in $mismatches){
		Write-Host ":::: Mismatch: $($k)"
	}

	return $mismatches.Count -eq 0
}

# setup variables
$ErrorActionPreference = "Stop"
$deviceOSSource = "source:${deviceOSPath}"
$tinkerPath = "$($deviceOSPath)\user\applications\tinker\"
$platforms = @()
$taskPairs = @()
$jobs = @()
$toolchain = run prtcl toolchain:view $deviceOSSource --json
$json = $toolchain | ConvertFrom-Json

if ($platformList){
	$platforms = $platformList.split(' ')
} else {
	foreach ($p in $json.platforms){
		$platforms += $p.name
	}
}

if ($taksList){
	$taskPairs = $taksList.split(' ')
} else {
	$taskPairs = @(
		'compile:user'
		'clean:user'
		'compile:all'
		'clean:all'
		'compile:debug'
		'clean:debug'
	)
}

# every compile task runs once per entry, the binaries must match the first run
if ($jobsList){
	$jobs = $jobsList.split(' ')
} else {
	$jobs = @('1', '8')
}

$scriptsID = ($json.dependencies | Where-Object { $_.type -eq 'scripts' }).id
$toolsID = ($json.dependencies | Where-Object { $_.type -eq 'tools' }).id

echo ":::: Using prtcl $(prtcl version)"
echo ":::: Using Device OS at $($deviceOSPath)"
echo ":::: Using toolchain dependencies: $($scriptsID) $($toolsID)"
echo ":::: Targeting platforms: $($platforms)"
echo ":::: Running tasks: $($taskPairs)"
echo ":::: Running compile tasks with --jobs: $($jobs)"

$stashDir = Join-Path ([System.IO.Path]::GetTempPath()) ("test-build-tasks-" + [guid]::NewGuid())

# install toolchain and run specified tasks
run prtcl toolchain:install $deviceOSSource --quiet

try {
	foreach ($platform in $platforms){
		echo ":::: Testing build tasks for $($platform)"
		echo ""

		foreach ($taskPair in $taskPairs){
			$subcmd, $task = $taskPair.Split(":");

			if ($subcmd -ne 'compile'){
				run prtcl project:$subcmd $tinkerPath --toolchain $deviceOSSource --platform $platform --task $task --quiet
				continue
			}

			$outDir = Join-Path $stashDir "$($platform)-$($task)"

			for ($i = 0; $i -lt $jobs.Count; $i++){
				$jobCount = $jobs[$i]

				if ($i -gt 0){
					run prtcl project:clean $tinkerPath --toolchain $deviceOSSource --platform $platform --task $task --quiet
				}

				echo ":::: Running compile:$($task) for $($platform) with --jobs $($jobCount)"
				run prtcl project:compile $tinkerPath --toolchain $deviceOSSource --platform $platform --task $task --jobs $jobCount --quiet
				stashBins (Join-Path $outDir "$i")

				if ($i -gt 0 -and !(compareBins (Join-Path $outDir '0') (Join-Path $outDir "$i"))){
					throw "compile:$($task) for $($platform) with --jobs $($jobCount) produced different binaries than with --jobs $($jobs[0])"
				}
			}
		}
	}
} finally {
	Remove-Item -Recurse -Force -ErrorAction SilentlyContinue $stashDir
}

echo ":::: done!"
exit 0
