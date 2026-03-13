#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
RELEASE_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

flash_suffix() {
    local platform="$1"
    local size_file
    local used available pct

    size_file="$(find "${RELEASE_DIR}/${platform}" -name 'system-part1.size' | head -n 1)"
    if [ ! -f "$size_file" ]; then
        return 0
    fi

    # shellcheck disable=SC1090
    . "$size_file"
    if [ -z "${used}" ] || [ -z "${available}" ] || [ "${available}" = "0" ]; then
        return 0
    fi

    pct="$(awk -v u="$used" -v a="$available" 'BEGIN { printf "%.1f", (u * 100.0) / a }')"
    awk -v u="$used" -v a="$available" -v p="$pct" \
        'BEGIN { printf " (%d/%d KiB, %s%%)", int((u + 1023) / 1024), int((a + 1023) / 1024), p }'
}

platform_msg() {
    local label="$1"
    local platform
    platform="$(printf '%s' "$label" | tr '[:upper:]' '[:lower:]')"

    if echo -e "${failures}" | grep -q "PLATFORM=\"${platform}\""; then
        echo ":scrum_closed: ${label}$(flash_suffix "${platform}")\\n"
    else
        echo ":scrum_finished: ${label}$(flash_suffix "${platform}")\\n"
    fi
}

RESULT_STATUS="passed"
RESULT_COLOR="#00ff00"

failures=$(cat $1/* 2>/dev/null)

if [ "${CF_PULL_REQUEST_CANNOT_BE_MERGED}" == "1" ] || [ "${failures}" != "" ] || [ "${CIRCLE_ARTIFACTS_URL}" == "" ]; then
    RESULT_STATUS="failed"
    RESULT_COLOR="#ff0000"
fi

RESULT_TIME_ELAPSED=$(date -u -d @"$(expr $(date +%s) - ${CIRCLE_BUILD_TIMESTAMP})" +"%T")

RESULT_ADDITIONAL=""

if [ "${CIRCLE_PR_NUMBER}" != "" ]; then
    RESULT_ADDITIONAL=" (<${CIRCLE_PULL_REQUEST}|PR ${CIRCLE_PR_NUMBER}>)"
fi

REPOSITORY_URL="https://github.com/${CIRCLE_PROJECT_USERNAME}/${CIRCLE_PROJECT_REPONAME}"
COMMIT_URL="${REPOSITORY_URL}/commit/${CIRCLE_SHA1}"

BASE_BLOCK=$(cat <<EOF
{
    "type": "section",
    "text": {
        "type": "mrkdwn",
        "text": "Build <${CIRCLE_WORKFLOW_URL}|${CIRCLE_BUILD_NUM}> of <${COMMIT_URL}|${CIRCLE_BRANCH}>${RESULT_ADDITIONAL} by ${CIRCLE_USERNAME} ${RESULT_STATUS} in ${RESULT_TIME_ELAPSED}"
    }
}
EOF
)

ADDITIONAL_BLOCKS=""

if echo -e "${failures}" | grep -q "PLATFORM=\"unit-test\""; then
    msg=":scrum_closed: Unit tests"
else
    msg=":scrum_finished: Unit tests"
fi
ADDITIONAL_BLOCKS=$(cat <<EOF
,{
    "type": "section",
    "text": {
        "type": "mrkdwn",
        "text": "${msg}"
    }
}
EOF
)

# Do not add new platforms here, there is a limit of 10 currently, see a block below instead
fields=""
for p in Argon Boron BSoM B5SoM Tracker TrackerM ESomX P2 GCC Newhal; do
    msg="$(platform_msg "$p")"
    field=$(cat <<EOF
{
    "type": "mrkdwn",
    "text": "${msg}"
}
EOF
)
    comma=","
    if [ "${fields}" == "" ]; then
        comma=""
    fi
    fields="${fields}${comma}${field}"
done
ADDITIONAL_BLOCKS+=$(cat <<EOF
,{
    "type": "section",
    "fields": [
        ${fields}
    ]
}
EOF
)

fields=""
for p in MSoM Electron2; do
    msg="$(platform_msg "$p")"
    field=$(cat <<EOF
{
    "type": "mrkdwn",
    "text": "${msg}"
}
EOF
)
    comma=","
    if [ "${fields}" == "" ]; then
        comma=""
    fi
    fields="${fields}${comma}${field}"
done
ADDITIONAL_BLOCKS+=$(cat <<EOF
,{
    "type": "section",
    "fields": [
        ${fields}
    ]
}
EOF
)

if [ "${RESULT_STATUS}" == "failed" ]; then
    if [ "${CIRCLE_ARTIFACTS_URL}" == "" ]; then
        msg=":scrum_closed: Artifacts"
    else
        msg=":scrum_finished: Artifacts"
    fi
    ADDITIONAL_BLOCKS+=$(cat <<EOF
,{
    "type": "section",
    "text": {
        "type": "mrkdwn",
        "text": "${msg}"
    }
}
EOF
)

else
    if [ "${CIRCLE_ARTIFACTS_URL}" != "" ]; then
        ADDITIONAL_BLOCKS+=$(cat <<EOF
,{
    "type": "section",
    "text": {
        "type": "mrkdwn",
        "text": "<${CIRCLE_ARTIFACTS_URL}|Artifacts>"
    }
}
EOF
)
    fi
fi

result=$(cat <<EOF
{
    "attachments":[
        {
            "blocks": [${BASE_BLOCK}${ADDITIONAL_BLOCKS}],
            "color": "${RESULT_COLOR}"
        }
    ]
}
EOF
)

echo "${result}"
