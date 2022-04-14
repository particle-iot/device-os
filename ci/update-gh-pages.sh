# Upload reports and artefacts to gh-pages
#
# For this to work GH_TOKEN needs to be defined with the OAuth2 github token.
# use `travis encrypt` to produce this and add to the .travis.yml file

# don't bother with this - it was just for demo purposes
exit

if [ "$TRAVIS_PULL_REQUEST" == "false" ]; then
  echo -e "Starting to update gh-pages\n"
  
  #copy data we're interested in to other place
  PAGES=$TRAVIS_BUILD_DIR/../gh-pages
  mkdir -p $PAGES  
  
  #go to home and setup git
  cd $PAGES
  git config --global user.email "travis@travis-ci.org"
  git config --global user.name "Travis"

  #using token clone gh-pages branch
  git clone --quiet --branch=gh-pages https://${GH_TOKEN}@github.com/spark/core-firmware.git  gh-pages > /dev/null

  #go into directory and copy data we're interested in to that directory
  cd gh-pages  
  cp -Rr $TRAVIS_BUILD_DIR/main/tests/unit/obj/report.th .
  cp -Rr $TRAVIS_BUILD_DIR/build/target/test-reports .

  #add, commit and push files
  git add -f .
  git commit -m "Travis build $TRAVIS_BUILD_NUMBER pushed to gh-pages"
  git push -fq origin gh-pages > /dev/null
  
fi
