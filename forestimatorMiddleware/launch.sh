sudo sh activeWebsite/launch.sh &
#sudo sh development/launch.sh &

sudo docker start nominatim6666 &

sudo /usr/local/go/bin/go run /home/carto/app/Forestimator/forestimatorMiddleware/middleware.go