
function die() {
   [[ -z "$1" ]] || echo "Error: $1"    
   exit 1
}

# executes script in this shell passed as stdin
function executeScript() {
   while read line; do
      eval $line
   done
}

function checkDefined() {   
   [ "${!1}" ] || die "\$$1 variable is awol."
}