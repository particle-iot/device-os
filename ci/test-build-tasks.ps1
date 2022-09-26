param (
	[Parameter(Mandatory = $true, Position = 0)][string]$deviceOSPath,
	[Parameter(Mandatory = $false, Position = 1)][string]$platformList,
	[Parameter(Mandatory = $false, Position = 2)][string]$taksList
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

# setup variables
$ErrorActionPreference = "Stop"
$deviceOSSource = "source:${deviceOSPath}"
$tinkerPath = "$($deviceOSPath)\user\applications\tinker\"
$platforms = @()
$taskPairs = @()

if ($platformList){
	$platforms = $platformList.split(' ')
} else {
	$toolchain = run prtcl toolchain:view $deviceOSSource --json
	$json = $toolchain | ConvertFrom-Json
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

echo ":::: Using prtcl $(prtcl version)"
echo ":::: Using Device OS at $($deviceOSPath)"
echo ":::: Targeting platforms: $($platforms)"
echo ":::: Running tasks: $($taskPairs)"

# install toolchain and run specified tasks
run prtcl toolchain:install $deviceOSSource --quiet

foreach ($platform in $platforms){
	echo ":::: Testing build tasks for $($platform)"
	echo ""

	foreach ($taskPair in $taskPairs){
		$subcmd, $task = $taskPair.Split(":");
		run prtcl project:$subcmd $tinkerPath --toolchain $deviceOSSource --platform $platform --task $task --quiet

		if ($LASTEXITCODE -ne 0){
			throw "Failure! Exit code is $LASTEXITCODE"
		}
	}
}

echo ":::: done!"
exit 0

