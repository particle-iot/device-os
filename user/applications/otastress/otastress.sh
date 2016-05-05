device=$1
file=$2

function wait_for_device()
{
    while [[ `particle variable get $device ready` != '1' ]]; do
        sleep 5
    done
}


echo "Runing OTA stress test on $device with file $file"

for i in `seq 1 100`;
do
       echo "waiting for device to come online " $i:
       wait_for_device
       echo "starting ota"
       particle flash $device $file
done
