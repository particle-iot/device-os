#!/bin/sh

set -x

if [ "$CF_PULL_REQUEST_NUMBER" != "" ]; then
  GITHUB_PR_MERGE_COMMIT_SHA=$(cat /codefresh/volume/event.json | jq -re .pull_request.merge_commit_sha)
  if [ "$GITHUB_PR_MERGE_COMMIT_SHA" != "null" ]; then
    cf_export CF_PULL_REQUEST_MERGED_COMMIT_SHA="$GITHUB_PR_MERGE_COMMIT_SHA"
    git fetch origin "$GITHUB_PR_MERGE_COMMIT_SHA"
    branch_name="cf/$CF_PULL_REQUEST_NUMBER"
    git branch -D ${branch_name} || true
    git checkout -b ${branch_name} FETCH_HEAD
  else
    cf_export CF_PULL_REQUEST_CANNOT_BE_MERGED=1
    exit 1
  fi
fi

