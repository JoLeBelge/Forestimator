#!/bin/bash

#pushd /data1/build-phytoTool
#/data1/Forestimator/build-WebAptitude/WebAptitude --docroot /data1/Forestimator/build-WebAptitude --http-port 82 --http-addr 0.0.0.0
/data1/Forestimator/build-WebAptitude/WebAptitude --deploy-path=/ --docroot "/data1/Forestimator/data/;/favicon.ico,/google52ee6b8ebe0b4b19.html,/sitemap.xml,/resources,/style,/tmp,/data,/js,/jslib,/img" --http-port 82 --http-addr 0.0.0.0 -c /data1/Forestimator/data/wt_config.xml
#popd
