#! /bin/bash
set -eu

# utils
hasCmd(){
	command -v "$1" > /dev/null
}

stashBins(){
	local dest=$1
	local name root f
	for name in deviceos app; do
		if [ "${name}" = "deviceos" ]; then
			root="${deviceOSPath}/build/target"
		else
			root="${tinkerPath}target"
		fi
		[ -d "${root}" ] || continue
		while IFS= read -r f; do
			mkdir -p "${dest}/${name}/$(dirname "${f}")"
			mv "${root}/${f}" "${dest}/${name}/${f}"
		done < <(cd "${root}" && find . -type f -name '*.bin')
	done
	if [ -z "$(find "${dest}" -type f -name '*.bin' 2>/dev/null)" ]; then
		echo ":::: Error: No binaries found after the build!"
		exit 1
	fi
}

# check args and prompt if missing
if [ $# -lt 1 ]; then
	echo ":::: Error: Missing script arguments!"
	echo ":::: Fix: \`deviceOSPath\` is required - e.g. \`~/path/to/device-os\`"
	echo ":::: Usage: <script> <deviceOSPath> [<platform>] [<taskpairs>] [<jobs>]"
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

# setup variables
deviceOSPath=$1
deviceOSSource="source:${deviceOSPath}"
tinkerPath="${deviceOSPath}/user/applications/tinker/"
platforms=()
toolchain="$(prtcl toolchain:view ${deviceOSSource} --json)"

if [ $# -gt 1 ]; then
	declare -a platforms=($2)
else
	platformsStr=$(echo $toolchain | jq '.platforms' | jq 'map(.name)')

	while read p; do
		platforms+=($(echo ${p} | tr -d '"'))
	done < <(echo $platformsStr | jq -c '.[]')
fi

if [ $# -gt 2 ]; then
	declare -a taskpairs=($3)
else
	taskpairs=(
		'compile:user'
		'clean:user'
		'compile:all'
		'clean:all'
		'compile:debug'
		'clean:debug'
	)
fi

# every compile task runs once per entry, the binaries must match the first run
if [ $# -gt 3 ]; then
	declare -a jobsList=($4)
else
	jobsList=(1 8)
fi

scriptsID="$(echo $toolchain | jq -r '.dependencies[] | select(.type == "scripts") | .id')"
toolsID="$(echo $toolchain | jq -r '.dependencies[] | select(.type == "tools") | .id')"

echo ":::: Using prtcl $(prtcl version)"
echo ":::: Using Device OS at: ${deviceOSPath}"
echo ":::: Using toolchain dependencies: ${scriptsID} ${toolsID}"
echo ":::: Targeting platforms: ${platforms[@]}"
echo ":::: Running task pairs: ${taskpairs[@]}"
echo ":::: Running compile tasks with --jobs: ${jobsList[@]}"

stashDir="$(mktemp -d)"
trap 'rm -rf "${stashDir}"' EXIT

# install toolchain and run specified tasks
prtcl toolchain:install ${deviceOSSource} --quiet

for platform in "${platforms[@]}"; do
	echo ":::: Testing build tasks for ${platform}"
	echo

	for taskpair in "${taskpairs[@]}"; do
		subcmd=$(echo $taskpair | cut -d ':' -f1)
		task=$(echo $taskpair | cut -d ':' -f2)

		if [ "${subcmd}" != "compile" ]; then
			prtcl project:$subcmd ${tinkerPath} --toolchain ${deviceOSSource} --platform ${platform} --task $task --quiet
			continue
		fi

		outDir="${stashDir}/${platform}-${task}"

		for i in "${!jobsList[@]}"; do
			jobs=${jobsList[$i]}

			if [ $i -gt 0 ]; then
				prtcl project:clean ${tinkerPath} --toolchain ${deviceOSSource} --platform ${platform} --task $task --quiet
			fi

			echo ":::: Running compile:${task} for ${platform} with --jobs ${jobs}"
			prtcl project:compile ${tinkerPath} --toolchain ${deviceOSSource} --platform ${platform} --task $task --jobs $jobs --quiet
			stashBins "${outDir}/${i}"

			if [ $i -gt 0 ] && ! diff -rq "${outDir}/0" "${outDir}/${i}"; then
				echo ":::: Error: compile:${task} for ${platform} with --jobs ${jobs} produced different binaries than with --jobs ${jobsList[0]}"
				exit 1
			fi
		done
	done
done

echo ":::: done!"
