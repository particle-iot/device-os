#!/bin/bash

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

# Underscores are treated as italic styling and seems to cause an error
export CIRCLE_BRANCH="${CIRCLE_BRANCH//_/\\_}"

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

if [ "${RESULT_STATUS}" == "failed" ]; then
    # No more than 10 fields, unit tests separately
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

    fields=""
    for p in Argon Boron BSoM B5SoM Tracker TrackerM ESomX P2 GCC Newhal P2; do
        if echo -e "${failures}" | grep -q "PLATFORM=\"${p,,}\""; then
            msg=":scrum_closed: $p\\n"
        else
            msg=":scrum_finished: $p\\n"
        fi
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
