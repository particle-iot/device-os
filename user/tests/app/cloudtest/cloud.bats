
id=3e0040000e47343432313031
name=photon4



@test "device is online" {
    skip
    list=$(particle list | grep $name)
    [[ $list == *online ]]
}

@test "device has functions and variables" {

}

@test "Integer variable value can be fetched and incremented" {
    intvar=$(particle variable get photon4 int)
    particle function call $name update
    intvar2=$(particle variable get photon4 int)

    [[ $intvar2 -eq $(($intvar + 1)) ]]
}

@test "Double variable value can be fetched and incremented" {
    doublevar=$(particle variable get photon4 double)
    particle function call $name update
    doublevar2=$(particle variable get photon4 double)

    [[ $double2 -eq $(($doublevar + 0.5)) ]]
}

@test "String variable can be set" {
    particle function call $name updateString abcde
    stringvar2=$(particle variable get photon4 string)

    [[ $stringvar2 == "abcde" ]]
}


@test "String variable can be set again" {
    particle function call $name updateString 1234
    stringvar2=$(particle variable get photon4 string)
    [[ $stringvar2 == "1234" ]]
}


@test "Function call to variable can be set to a 60-character value" {
    # function call has a maximum length of 64 characters
    onehundred="012345678901234567890123456789012345678901234567890123456789"
    particle function call $name updateString $onehundred
    stringvar2=$(particle variable get photon4 string)
    [[ $stringvar2 == "$onehundred" ]]
}


alpha="ABCDEFGHIJKLMNOPQRSTUVWXYZ"

@test "String variable can be set to a 26-character value" {
    particle function call $name setString 26
    [[ $output == 26 ]]

    stringvar2=$(particle variable get photon4 string)
    [[ $stringvar2 == "$alpha" ]]
}


@test "String variable can be set to a 260-character value" {
    particle function call $name setString 260
    [[ $output == 260 ]]

    stringvar2=$(particle variable get photon4 string)
    [[ $stringvar2 == "$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha" ]]
}

@test "String variable can be set to a 624-character value" {
    particle function call $name setString 624
    [[ $output == 624 ]]

    stringvar2=$(particle variable get photon4 string)
    [[ $stringvar2 == "$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha$alpha" ]]
}

@test "can turn on nyan" {
    particle nyan $name
}

@test "can turn off nyan" {
    particle nyan $name off
}



# todo - how to flash to a device and verify the flash succeeded?
