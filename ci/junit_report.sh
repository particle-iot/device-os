#!/bin/bash
# creates a junit report from the test reports

reports=$1

declare -A totals

totals[passed]=0
totals[failed]=0
totals[skipped]=0
totals[count]=0

export totals

# Sums the totals from all of the 
function calc_totals() {
    local passed
    local failed
    local skipped
    local count
    for f in ($reports/test_*_result.txt) do
        . $f
        totals[passed] += $passed
    done
}

calc_totals

echo $totals[@]