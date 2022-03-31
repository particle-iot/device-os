#! /bin/bash
set -eu

# utils
hasCmd(){
	command -v "$1" > /dev/null
}

# check args and prompt if missing
if [ $# -lt 1 ]; then
	echo ":::: Error: Missing script arguments!"
	echo ":::: Fix: \`deviceOSPath\` is required - e.g. \`~/path/to/device-os\`"
	echo ":::: Usage: <script> <deviceOSPath> [<tasks>]"
	exit 1
fi

# check for required dependencies
if ! hasCmd prtcl; then
	echo ':::: Your system does not have the `prtcl` cli'
	echo ':::: To install, see: https://github.com/particle-iot/cli/tree/main/packages/cli#installation'
	exit 1
fi

if ! hasCmd jq; then
	echo ':::: Your system does not have the `jq` command'
	echo ':::: To install, see: https://stedolan.github.io/jq/download/'
	exit 1
fi

# run the required local compilation tasks
deviceOSPath=$1
deviceOSSource="source:${deviceOSPath}"
tinkerPath="${deviceOSPath}/user/applications/tinker/"
toolchain="$(prtcl toolchain:view ${deviceOSSource} --json)"
platforms=$(echo $toolchain | jq '.platforms' | jq 'map(.name)')

if [ $# -gt 1 ]; then
	declare -a tasks=($2)
else
	tasks=(
		'compile:user'
		'clean:user'
		'compile:all'
		'clean:all'
		'compile:debug'
		'clean:debug'
	)
fi

echo ":::: Using prtcl $(prtcl version)"
echo ":::: Using Device OS at ${deviceOSPath}"

prtcl toolchain:install ${deviceOSSource} --quiet

echo $platforms | jq -c '.[]' | while read p; do
	platform=$(echo ${p} | tr -d '"')
	echo ":::: Testing build tasks for ${platform}"
	echo

	for task in "${tasks[@]}"; do
		prtcl ${task} ${deviceOSSource} ${platform} ${tinkerPath} --quiet
	done
done

echo ":::: done!"

