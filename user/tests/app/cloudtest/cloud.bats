
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

id=$DEVICE_ID
name=$DEVICE_NAME

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
	particle function call $DEVICE_NAME cmd $1
}

function flash_app()
{
	pushd ../../../../main	
	make PLATFORM_ID=$PLATFORM_ID APP=$1 all DEBUG_BUILD=y DEBUG=1
	popd
	output=../../../../build/target/user-part/platform-$PLATFORM_ID-m/$(basename $1).bin
	[[ -f $output ]] 
	particle flash $DEVICE_NAME $output
	sleep 20s
	# todo - how to check that the flash was successful?
}


@test "device is online" {
    list=$(particle list | grep $name)
    [[ $list == *online ]]
}

function has_test_app()
{
    list=$(api_call "/v1/devices/$id" )
	echo $list > list.txt
    fns=$(echo $list | jq -c .functions )
	vars=$(echo $list | jq -c .variables )
	[[ $vars == '{"bool":"int32","int":"int32","double":"double","string":"string"}' ]] &&
	[[ $fns == '["updateString","update","setString","checkString","cmd"]' ]]  	
}


@test "can flash tinker to the device" {
	flash_app tinker
}

@test "tinker functions are visible" {
    list=$(api_call "/v1/devices/$id" )
    echo $list > list.txt
    fns=$(echo $list | jq -c .functions )
	[[ $fns == '["digitalread","digitalwrite","analogread","analogwrite"]' ]]  
}



@test "can flash test application if needed" {
	has_test_app && skip
	flash_app ../tests/app/cloudtest
}

@test "device has test app functions and variables" {
	has_test_app	
}

@test "device reconnects to the cloud after dropping the PDP context" {	
	cmd bounce_pdp
	sleep 20s
	# wait for the device to come back online	
}

@test "Integer variable value can be fetched and incremented" {
    intvar=$(particle variable get $name int)
    particle function call $name update
    intvar2=$(particle variable get $name int)

    [[ $intvar2 -eq $(($intvar + 1)) ]]
}

@test "Double variable value can be fetched and incremented" {
	skip # a problem with adding 0.5 to the value?
    doublevar=$(particle variable get $name double)
 	   particle function call $name update
    doublevar2=$(particle variable get $name double)

    [[ $double2 -eq $(($doublevar + 0.5)) ]]
}

@test "String variable can be set" {
    particle function call $name updateString abcde
    stringvar2=$(particle variable get $name string)

    [[ $stringvar2 == "abcde" ]]
}


@test "String variable can be set again" {
    particle function call $name updateString 1234
    stringvar2=$(particle variable get $name string)
    [[ $stringvar2 == "1234" ]]
}


@test "Function call to variable can be set to a 60-character value" {
    # function call has a maximum length of 64 characters
    onehundred="012345678901234567890123456789012345678901234567890123456789"
    particle function call $name updateString $onehundred
    stringvar2=$(particle variable get $name string)
    [[ $stringvar2 == "$onehundred" ]]
}


alpha="ABCDEFGHIJKLMNOPQRSTUVWXYZ"

@test "String variable can be set to a 26-character value" {
    particle function call $name setString 26
    [[ $output == 26 ]]

    stringvar2=$(particle variable get $name string)
    [[ $stringvar2 == "$alpha" ]]
}


@test "String variable can be set to a 260-character value" {
    particle function call $name setString 260
    [[ $output == 260 ]]

    stringvar2=$(particle variable get $name string)
    [[ $stringvar2 == "$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha" ]]
}

@test "String variable can be set to a 624-character value" {
    particle function call $name setString 624
    [[ $output == 624 ]]

    stringvar2=$(particle variable get $name string)
    [[ $stringvar2 == "$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha" ]]
}

@test "can turn on nyan" {
    particle nyan $name
}

@test "can turn off nyan" {
    particle nyan $name off
}

@test "device can subscribe to an event" {

    stringvar2=$(particle variable get $name string)
    [[ $stringvar2 != "happytest" ]]

	particle publish cloudtest happytest
	# the event changes the string as a side effect
    stringvar2=$(particle variable get $name string)
    [[ $stringvar2 == "happytest" ]]	
}

@test "can flash tinker to the device" {
	flash_app tinker_electron
}

@test "tinker functions are visible" {
    list=$(api_call "/v1/devices/$id" )
    echo $list > list.txt
    fns=$(echo $list | jq -c .functions )
	[[ $fns == '["digitalread","digitalwrite","analogread","analogwrite"]' ]]  
}




# todo - how to flash to a device and verify the flash succeeded?
