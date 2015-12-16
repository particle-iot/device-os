
# : ${ID:="3f004c000e51343035353132"}
# : ${NAME:="electron1"}

function die()
{
	echo "exiting: " $1
	exit -1
}


[[ "$DEVICE_ID" ]] || die "DEVICE_ID not defined"
[[ "$DEVICE_NAME" ]] || die "DEVICE_NAME not defined"

id=$DEVICE_ID
name=$DEVICE_NAME


@test "device is online" {
    
    list=$(particle list | grep $name)
    [[ $list == *online ]]
}

@test "device has functions and variables" {

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


# todo - how to flash to a device and verify the flash succeeded?
