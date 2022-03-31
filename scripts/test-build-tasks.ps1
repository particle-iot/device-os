param (
	[Parameter(Mandatory = $true, Position = 0)][string]$deviceOSPath,
	[Parameter(Mandatory = $false, Position = 1)][string]$taksList
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

$ErrorActionPreference = "Stop"
$deviceOSSource="source:${deviceOSPath}"
$tinkerPath="$($deviceOSPath)\user\applications\tinker\"
$toolchain = run prtcl toolchain:view $deviceOSSource --json

if ($taksList){
	$tasks = $taksList.split(' ')
} else {
	$tasks = @(
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

run prtcl toolchain:install $deviceOSSource --quiet

$json = $toolchain | ConvertFrom-Json
foreach ($platform in $json.platforms){
	echo ":::: Testing build tasks for $($platform.name)"
	echo ""

	foreach ($task in $tasks){
		run prtcl $task $deviceOSSource $platform.name $tinkerPath --quiet

		if ($LASTEXITCODE -ne 0){
			throw "Failure! Exit code is $LASTEXITCODE"
		}
	}
}

echo ":::: done!"
exit 0

