#!/bin/bash

RESULT_STATUS="passed"
RESULT_COLOR="#00ff00"

failures=$(cat ${CF_VOLUME_PATH}/failures/* 2>/dev/null)

if [ "${CF_PULL_REQUEST_CANNOT_BE_MERGED}" == "1" ] || [ "${failures}" != "" ]; then
    RESULT_STATUS="failed"
    RESULT_COLOR="#ff0000"
fi

RESULT_TIME_ELAPSED=$(date -u -d @"$(expr $(date +%s) - ${CF_BUILD_TIMESTAMP} / 1000)" +"%T")

RESULT_ADDITIONAL=""

if [ "${CF_PULL_REQUEST_NUMBER}" != "" ]; then
    RESULT_ADDITIONAL=" (<https://github.com/${CF_REPO_OWNER}/${CF_REPO_NAME}/pull/${CF_PULL_REQUEST_NUMBER}|PR ${CF_PULL_REQUEST_NUMBER}>)"
fi

CF_BUILD_URL_PUBLIC=$(echo "${CF_BUILD_URL}" | sed -e 's|/build/|/public/accounts/particle/builds/|')

BASE_BLOCK=$(cat <<EOF
{
    "type": "section",
    "text": {
        "type": "mrkdwn",
        "text": "Build <${CF_BUILD_URL_PUBLIC}|${CF_BUILD_ID}> of <${CF_COMMIT_URL}|${CF_BRANCH}>${RESULT_ADDITIONAL} by ${CF_BUILD_INITIATOR} ${RESULT_STATUS} in ${RESULT_TIME_ELAPSED}"
    }
}
EOF
)

ADDITIONAL_BLOCKS=""

if [ "${failures}" != "" ]; then
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
    for p in Argon Boron BSoM B5SoM Electron Photon P1 Tracker GCC Newhal; do
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
