# According to https://askubuntu.com/questions/492033/fontconfig-error-cannot-load-default-config-file
# the next line solves an issiue with pango
export FONTCONFIG_PATH=/etc/fonts

sudo /usr/local/go/bin/go run /home/carto/app/Forestimator/forestimatorMiddleware/forestimatorMiddleware.go

sudo docker start nominatim6666
