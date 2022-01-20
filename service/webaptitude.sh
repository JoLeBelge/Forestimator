#!/bin/bash

#pushd /data1/build-phytoTool
#/data1/Forestimator/build-WebAptitude/WebAptitude --docroot /data1/Forestimator/build-WebAptitude --http-port 82 --http-addr 0.0.0.0
#/data1/Forestimator/build-WebAptitude/WebAptitude --deploy-path=/ --docroot "/data1/Forestimator/data/;/favicon.ico,/google52ee6b8ebe0b4b19.html,/sitemap.xml,/resources,/style,/tmp,/data,/js,/jslib,/img,/pdf" --http-port 82 --http-addr 0.0.0.0 -c /data1/Forestimator/data/wt_config.xml

/home/carto/app/Forestimator/build/WebAptitude --deploy-path=/ --docroot "/home/carto/app/Forestimator/data/;/favicon.ico,/google52ee6b8ebe0b4b19.html,/sitemap.xml,/resources,/style,/tmp,/data,/js,/jslib,/img,/pdf" --http-port 85 --http-addr 0.0.0.0 -c /home/carto/app/Forestimator/data/wt_config.xml --BD "/home/carto/app/Forestimator/carteApt/data/aptitudeEssDB.db"


#popd
