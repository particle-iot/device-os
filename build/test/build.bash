load assert

make_args=

function setup()
{
    # get in the project root
    pushd ../..
}

function teardown()
{
   popd
}

function force_clean() {
   rm -rf build/target
   true
}

function trim() {
    trimmed=$(echo $1 | tr -s [:space:] " " | xargs)
    echo $(echo $trimmed)
}

function file_size_range() {
    file=$1
    scale=${4:=1}
    if [ "$scale" == "K" ]; then
    scale=1024
    fi
    minimumsize=$(($2*$scale))
    maximumsize=$(($3*$scale))

    actualsize=$(wc -c <"$file")
    if [ "$actualsize" -le "$minimumsize" ]; then
        echo "file '$file' size is $actualsize which is less than $minimumsize"
        return -1
    else
        if [ "$actualsize" -ge "$maximumsize" ]; then
            echo "file '$file' size is $actualsize which is greater than $maximumsize"
            return 1
        fi
    fi
    return 0
}
