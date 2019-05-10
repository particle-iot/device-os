
#   .d8888b.  888      .d88888b.  888     888 8888888b.      888888b.         d8888 88888888888 .d8888b.
#  d88P  Y88b 888     d88P" "Y88b 888     888 888  "Y88b     888  "88b       d88888     888    d88P  Y88b
#  888    888 888     888     888 888     888 888    888     888  .88P      d88P888     888    Y88b.
#  888        888     888     888 888     888 888    888     8888888K.     d88P 888     888     "Y888b.
#  888        888     888     888 888     888 888    888     888  "Y88b   d88P  888     888        "Y88b.
#  888    888 888     888     888 888     888 888    888     888    888  d88P   888     888          "888
#  Y88b  d88P 888     Y88b. .d88P Y88b. .d88P 888  .d88P d8b 888   d88P d8888888888     888    Y88b  d88P
#   "Y8888P"  88888888 "Y88888P"   "Y88888P"  8888888P"  Y8P 8888888P" d88P     888     888     "Y8888P"
#
# http://patorjk.com/software/taag/#p=display&f=Colossal&t=CLOUD.BATS

# Usage:
# DEVICE_ID=123412341234123412341234 DEVICE_NAME=photon_p2 PLATFORM_ID=6 API_URL=https://api.particle.io ACCESS_TOKEN=123412341234123412341234 DEVICEOS_PATH=/Users/brett/code/firmware/ bats /Users/brett/code/firmware/user/tests/app/cloudtest/cloud.bats

function die()
{
	echo "exiting: " $1
	exit -1
}

function check_defined()
{
	eval val=\$$1
	[[ $val ]] || die "$1 not defined."
}

check_defined DEVICE_ID
check_defined PLATFORM_ID
check_defined DEVICE_NAME
check_defined ACCESS_TOKEN
check_defined API_URL
check_defined DEVICEOS_PATH

id=$DEVICE_ID
name=$DEVICE_NAME

alpha="ABCDEFGHIJKLMNOPQRSTUVWXYZ"
FUNC_KEY_MIN="F"
FUNC_KEY_MAX="NULL"
FUNC_ARG_MAX="NULL"
VAR_MAX="NULL"
EVENT_NAME_MAX="1234567890123456789012345678901234567890123456789012345678901234"
EVENT_DATA_MAX="NULL"
FUNC_KEY_MAX_CORE="FUN126789012"
FUNC_ARG_MAX_CORE="1234567890123456789012345678901234567890123456789012345678901234"
EVENT_DATA_MAX_CORE="123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345"
VAR_MAX_CORE=622
FUNC_KEY_MAX_OTHER="FUN646789012345678901234567890123456789012345678901234567890ABCD"
FUNC_ARG_MAX_OTHER="12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890AB"
EVENT_DATA_MAX_OTHER="12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890AB"
# 776 for p1, 782 for electron... lets test up to 770
VAR_MAX_OTHER=770

function get_func_key_max()
{
    if [ $PLATFORM_ID == 0 ]; then
       FUNC_KEY_MAX=$(echo $FUNC_KEY_MAX_CORE)
    else
       FUNC_KEY_MAX=$(echo $FUNC_KEY_MAX_OTHER)
    fi
    echo $FUNC_KEY_MAX
}

function get_func_arg_max()
{
    if [ $PLATFORM_ID == 0 ]; then
       FUNC_ARG_MAX=$(echo $FUNC_ARG_MAX_CORE)
    else
       FUNC_ARG_MAX=$(echo $FUNC_ARG_MAX_OTHER)
    fi
    echo $FUNC_ARG_MAX
}

function get_var_max()
{
    if [ $PLATFORM_ID == 0 ]; then
       VAR_MAX=$(echo $VAR_MAX_CORE)
    else
       VAR_MAX=$(echo $VAR_MAX_OTHER)
    fi
    echo $VAR_MAX
}

function get_event_data_max()
{
    if [ $PLATFORM_ID == 0 ]; then
       EVENT_DATA_MAX=$(echo $EVENT_DATA_MAX_CORE)
    else
       EVENT_DATA_MAX=$(echo $EVENT_DATA_MAX_OTHER)
    fi
    echo $EVENT_DATA_MAX
}

function make_api_call()
{
	args="curl -H \"Authorization: Bearer $ACCESS_TOKEN\" \"$API_URL$1\""
	echo "$args"
}

function api_call()
{
	eval "$(make_api_call $*)"
}

function cmd()
{
	particle call $DEVICE_NAME cmd $1
}

function flash_app()
{
	pushd $DEVICEOS_PATH/main
	make all -s PLATFORM_ID=$PLATFORM_ID APP=$1
	popd
    if [ $PLATFORM_ID == 0 ]; then
	   output=$DEVICEOS_PATH/build/target/main/platform-$PLATFORM_ID-lto/$(basename $1).bin
    else
       output=$DEVICEOS_PATH/build/target/user-part/platform-$PLATFORM_ID-m/$(basename $1).bin
    fi
    echo -e [$output]
	[[ -f $output ]]
	particle flash $DEVICE_NAME $output
	if [ $PLATFORM_ID == 0 ]; then
        sleep 40
    else
        sleep 20
    fi
	# todo - how to check that the flash was successful?
}

function has_tinker_app()
{
    list=$(api_call "/v1/devices/$id" )
    echo $list > list.txt
    fns=$(echo $list | jq -c .functions )
    [[ $fns == '["digitalread","digitalwrite","analogread","analogwrite"]' ]]
}

function has_test_app()
{
    FUNC_KEY_MAX=$(get_func_key_max)
    list=$(api_call "/v1/devices/$id" )
    echo $list > list.txt
    fns=$(echo $list | jq -c .functions )
    vars=$(echo $list | jq -c .variables )
    echo -e $vars
    echo -e "{\"bool\":\"bool\",\"int\":\"int32\",\"double\":\"double\",\"string\":\"string\"}"
    echo -e $fns
    echo -e "[\"updateString\",\"update\",\"setString\",\"checkString\",\"$FUNC_KEY_MAX\",\"F\",\"cmd\"]"
    [[ $vars == "{\"bool\":\"bool\",\"int\":\"int32\",\"double\":\"double\",\"string\":\"string\"}" ]] &&
    [[ $fns == "[\"updateString\",\"update\",\"setString\",\"checkString\",\"$FUNC_KEY_MAX\",\"F\",\"cmd\"]" ]]
}

function wait_until_device_online_with_cloudtest_app()
{
    FUNC_KEY_MAX=$(get_func_key_max)
    list=$(api_call "/v1/devices/$id" )
    echo $list > list.txt
    fns=$(echo $list | jq -c .functions )
    vars=$(echo $list | jq -c .variables )
    if [ $vars == "{\"bool\":\"bool\",\"int\":\"int32\",\"double\":\"double\",\"string\":\"string\"}" ] &&
        [ $fns == "[\"updateString\",\"update\",\"setString\",\"checkString\",\"$FUNC_KEY_MAX\",\"F\",\"cmd\"]" ]; then
        echo "Device is online. Pass."
        return 0
    else
        echo "Device is offline. Trying again in 30 seconds!"
        sleep 30
        list=$(api_call "/v1/devices/$id" )
        echo $list > list.txt
        fns=$(echo $list | jq -c .functions )
        vars=$(echo $list | jq -c .variables )
        if [ $vars == "{\"bool\":\"bool\",\"int\":\"int32\",\"double\":\"double\",\"string\":\"string\"}" ] &&
        [ $fns == "[\"updateString\",\"update\",\"setString\",\"checkString\",\"$FUNC_KEY_MAX\",\"F\",\"cmd\"]" ]; then
            echo "Device is online. Pass."
            return 0
        else
            echo "Device is offline. Fail."
            return 1
        fi
    fi
}

function wait_until_device_online_with_tinker_app()
{
    list=$(api_call "/v1/devices/$id" )
    echo $list > list.txt
    fns=$(echo $list | jq -c .functions )
    if [ $fns == '["digitalread","digitalwrite","analogread","analogwrite"]' ]; then
        echo "Device is online. Pass."
        return 0
    else
        echo "Device is offline. Trying again in 30 seconds!"
        sleep 30
        list=$(api_call "/v1/devices/$id" )
        echo $list > list.txt
        fns=$(echo $list | jq -c .functions )
        if [ $fns == '["digitalread","digitalwrite","analogread","analogwrite"]' ]; then
            echo "Device is online. Pass."
            return 0
        else
            echo "Device is offline. Fail."
            return 1
        fi
    fi
}

#   .d8888b.  8888888888 88888888888 888     888 8888888b.
#  d88P  Y88b 888            888     888     888 888   Y88b
#  Y88b.      888            888     888     888 888    888
#   "Y888b.   8888888        888     888     888 888   d88P
#      "Y88b. 888            888     888     888 8888888P"
#        "888 888            888     888     888 888
#  Y88b  d88P 888            888     Y88b. .d88P 888
#   "Y8888P"  8888888888     888      "Y88888P"  888
#
# http://patorjk.com/software/taag/#p=display&f=Colossal&t=SETUP

@test "device is online" {
    list=$(particle list | grep $name)
    [[ $list == *online ]]
}

#   .d8888b.  888      .d88888b.  888     888 8888888b. 88888888888 8888888888 .d8888b. 88888888888
#  d88P  Y88b 888     d88P" "Y88b 888     888 888  "Y88b    888     888       d88P  Y88b    888
#  888    888 888     888     888 888     888 888    888    888     888       Y88b.         888
#  888        888     888     888 888     888 888    888    888     8888888    "Y888b.      888
#  888        888     888     888 888     888 888    888    888     888           "Y88b.    888
#  888    888 888     888     888 888     888 888    888    888     888             "888    888
#  Y88b  d88P 888     Y88b. .d88P Y88b. .d88P 888  .d88P    888     888       Y88b  d88P    888
#   "Y8888P"  88888888 "Y88888P"   "Y88888P"  8888888P"     888     8888888888 "Y8888P"     888
#
# http://patorjk.com/software/taag/#p=display&f=Colossal&t=CLOUDTEST

@test "can flash test application if needed" {
    has_test_app && skip
	flash_app ../tests/app/cloudtest
}

@test "wait until device is online with cloudtest app 1" {
    wait_until_device_online_with_cloudtest_app
}

@test "device reconnects to the cloud after dropping the PDP context" {
    if [ $PLATFORM_ID != 10 ]; then
        skip "electron only"
    fi
    cmd bounce_pdp
    sleep 30
}

@test "wait until device is online with cloudtest app 2" {
    if [ $PLATFORM_ID != 10 ]; then
        skip "electron only"
    fi
    wait_until_device_online_with_cloudtest_app
}

@test "Integer variable value can be fetched and incremented" {
    intvar=$(particle get $name int)
    particle call $name update
    intvar2=$(particle get $name int)

    [[ $intvar2 -eq $(($intvar + 1)) ]]
}

@test "Double variable value can be fetched and incremented" {
    doublevar=$(particle get $name double)
    run particle call $name update
    doublevar2=$(particle get $name double)
    # reprint doublevar2 to convert very small errors like 1.2100000000000002 => 1.21
    doublevar2=$(awk "BEGIN {print $doublevar2; exit}")
    doublevartest=$(awk "BEGIN {print $doublevar+0.11; exit}")
    echo -e $doublevar2 == $doublevartest
    [[ $doublevar2 == $doublevartest ]]
}

@test "String variable can be set" {
    particle call $name updateString abcde
    run particle get $name string

    [[ $output == "abcde" ]]
}

@test "String variable can be set again" {
    particle call $name updateString 1234
    run particle get $name string
    [[ $output == "1234" ]]
}

@test "Function call to variable can be set to a 60-character value" {
    val="012345678901234567890123456789012345678901234567890123456789"
    particle call $name updateString $val
    run particle get $name string
    [[ $output == "$val" ]]
}

@test "String variable can be set to a 1-character value" {
    run particle call $name setString 1
    [[ $output == "1" ]]

    run particle get $name string
    [[ $output == "A" ]]
}

@test "String variable can be set to a 255-character value" {
    run particle call $name setString 255
    [[ $output == "255" ]]

    maxalpha=$(echo $alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha"ABCDEFGHIJKLMNOPQRSTU")
    run particle get $name string
    echo $output $maxalpha
    [[ $output == $maxalpha ]]
}

@test "String variable can be set to a $VAR_MAX_CORE-character value for Core, or $VAR_MAX_OTHER-character value for others" {
    VAR_MAX=$(get_var_max)
    # limited by the PROTOCOL_BUFFER_SIZE - 2 byte leading length - 16 potential padding bytes
    run particle call $name setString $VAR_MAX
    [[ $output == $VAR_MAX ]]

    if [ $PLATFORM_ID == 0 ]; then
        maxalpha=$(echo $alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha"ABCDEFGHIJKLMNOPQRSTUVWX")
    else
        maxalpha=$(echo $alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha"ABCDEFGHIJKLMNOP")
    fi
    run particle get $name string
    echo $output $maxalpha
    [[ $output == $maxalpha ]]
}

@test "Test min function key size" {
    run particle call $name $FUNC_KEY_MIN 1234,5678
    [[ $output == "9" ]]

    run particle get $name string
    [[ $output == "1234,5678" ]]
}

@test "Test max function key size" {
    FUNC_KEY_MAX=$(get_func_key_max)
    run particle call $name $FUNC_KEY_MAX 1234
    [[ $output == "4" ]]

    run particle get $name string
    [[ $output == "1234" ]]
}

@test "Test beyond max function key size" {
    FUNC_KEY_MAX=$(get_func_key_max)
    func=$(echo $FUNC_KEY_MAX"1")
    run particle call $name $func 5678
    [[ $status -eq 1 ]]
    [[ $output != "4" ]]

    run particle get $name string
    [[ $output != "5678" ]]
}

@test "Test min function key AND min function argument size" {
    run particle call $name $FUNC_KEY_MIN A
    [[ $output == "1" ]]

    run particle get $name string
    [[ $output == "A" ]]
}

@test "Test min function key AND max function argument size" {
    FUNC_KEY_MAX=$(get_func_key_max)
    FUNC_ARG_MAX=$(get_func_arg_max)
    echo $FUNC_KEY_MAX $FUNC_ARG_MAX
    run particle call $name $FUNC_KEY_MIN $FUNC_ARG_MAX
    [[ $output == ${#FUNC_ARG_MAX} ]]

    run particle get $name string
    [[ $output == $FUNC_ARG_MAX ]]
}

@test "Test max function key AND max function argument size" {
    FUNC_KEY_MAX=$(get_func_key_max)
    FUNC_ARG_MAX=$(get_func_arg_max)
    echo $FUNC_KEY_MAX $FUNC_ARG_MAX
    run particle call $name $FUNC_KEY_MAX $FUNC_ARG_MAX
    [[ $output == ${#FUNC_ARG_MAX} ]]

    run particle get $name string
    [[ $output == $FUNC_ARG_MAX ]]
}

@test "Test max function key AND beyond max function argument size" {
    FUNC_KEY_MAX=$(get_func_key_max)
    FUNC_ARG_MAX=$(get_func_arg_max)
    echo $FUNC_KEY_MAX $FUNC_ARG_MAX
    run particle call $name $FUNC_KEY_MAX $FUNC_ARG_MAX"1"
    [[ $output == ${#FUNC_ARG_MAX} ]]

    run particle get $name string
    [[ $output == $FUNC_ARG_MAX ]]
}

@test "can turn on nyan" {
    particle nyan $name on
}

@test "can turn off nyan" {
    particle nyan $name off
}

@test "device can subscribe to an event" {
    run particle get $name string
    [[ $output != "happytest" ]]

    particle publish cloudtest happytest --private
    # the event changes the string as a side effect
    run particle get $name string
    [[ $output == "happytest" ]]
}

@test "device can subscribe to an event name max" {
    echo -e [$EVENT_NAME_MAX $output]
    run particle get $name string
    [[ $output != "happytest2" ]]

    particle publish $EVENT_NAME_MAX happytest2 --private
    # the event changes the string as a side effect
    run particle get $name string
    echo -e [$output]
    [[ $output == "happytest2" ]]
}

@test "device can publish event data max" {
    EVENT_DATA_MAX=$(get_event_data_max)
    run particle get $name string
    [[ $output != $EVENT_DATA_MAX ]]

    particle publish cloudtest $EVENT_DATA_MAX --private
    # the event changes the string as a side effect
    run particle get $name string
    [[ $output == $EVENT_DATA_MAX ]]
}

@test "device can publish event data max AND subscribe to event name max" {
    particle publish cloudtest happytest3 --private

    EVENT_DATA_MAX=$(get_event_data_max)
    run particle get $name string
    [[ $output != $EVENT_DATA_MAX ]]

    particle publish $EVENT_NAME_MAX $EVENT_DATA_MAX --private
    # the event changes the string as a side effect
    run particle get $name string
    [[ $output == $EVENT_DATA_MAX ]]
}

#  88888888888 8888888 888b    888 888    d8P  8888888888 8888888b.
#      888       888   8888b   888 888   d8P   888        888   Y88b
#      888       888   88888b  888 888  d8P    888        888    888
#      888       888   888Y88b 888 888d88K     8888888    888   d88P
#      888       888   888 Y88b888 8888888b    888        8888888P"
#      888       888   888  Y88888 888  Y88b   888        888 T88b
#      888       888   888   Y8888 888   Y88b  888        888  T88b
#      888     8888888 888    Y888 888    Y88b 8888888888 888   T88b
#
# http://patorjk.com/software/taag/#p=display&f=Colossal&t=TINKER

@test "can flash tinker to the device if needed" {
    has_tinker_app && skip
    flash_app tinker
}

@test "wait until device is online with tinker app" {
    wait_until_device_online_with_tinker_app
}

@test "user LED can be turned on" {
    run particle call $name digitalwrite D7,HIGH

    [[ $output == "1" ]]
}

@test "user LED output is HIGH, but digitalread switches it to INPUT mode" {
    run particle call $name digitalread D7

    [[ $output == "0" ]]
}

@test "user LED can be turned on again" {
    run particle call $name digitalwrite D7,HIGH

    [[ $output == "1" ]]
}

@test "user LED can be turned off" {
    run particle call $name digitalwrite D7,LOW

    [[ $output == "1" ]]
}

@test "user LED output is LOW" {
    run particle call $name digitalread D7

    [[ $output == "0" ]]
}


# todo - how to flash to a device and verify the flash succeeded?
