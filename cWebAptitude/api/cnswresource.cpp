#include "cnswresource.h"

// avec mon écran forestimator a une emprise au sol de minimum +-250 m
extern bool globTest;

cnswresource::cnswresource(std::string aTmpDir):mTmpDir(aTmpDir){
}
cnswresource::~cnswresource(){
}

void cnswresource::handleRequest(const Request& request, Response& response) {

    std::string pathInfo = request.pathInfo();
    //std::cout << " path query " <<  request.queryString() <<  std::endl;
    // il faut enlever le / du pathInfo
    pathInfo=pathInfo.substr(1,pathInfo.size()-1);

    std::string BBOXold=*request.getParameterValues("BBOX").data();
    std::string BBOX=BBOXold;
    if (globTest){std::cout << " BBOX =  " <<   BBOX <<  std::endl;}
    double Xmin,Xmax,Ymin,Ymax;
    Xmin = std::stod(BBOX.substr(0, BBOX.find(",")));
    BBOX.erase(0, BBOX.find(",") + 1);
    Ymin = std::stod(BBOX.substr(0, BBOX.find(",")));
    BBOX.erase(0, BBOX.find(",") + 1);
    Xmax = std::stod(BBOX.substr(0, BBOX.find(",")));
    BBOX.erase(0, BBOX.find(",") + 1);
    Ymax = std::stod(BBOX);
    //std::cout << " Xmin =  " <<   Xmin << " , Xmax " << Xmax << " , " << Ymin << " , " << Ymax <<  std::endl;

    // largeur de l'emprise au sol
    double W=(Xmax-Xmin);
    std::string url="https://geoservices.wallonie.be/arcgis/rest/services/SOL_SOUS_SOL/CNSW/MapServer/export?";

    response.setMimeType("image/png");
    std::ostream& out = response.out();

    // si emprise trop petite, le serveur refuse de servir une image
   /* if (W>260){
        url+=request.queryString();
        //std::cout << "url : " << url << std::endl;
        // https://gist.github.com/alghanmi/c5d7b761b2c9ab199157
        CURL *curl;
        CURLcode res;
        std::string readBuffer;
        curl = curl_easy_init();
        if(curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            out << readBuffer << std::endl;
        }
    } else {
    */

        std::string sRequest=request.queryString();
        double aMinSize(260);
        //double aMinSize(400);
        double aTargetSize(500);
        if ( W<aMinSize){
        // changer l'extend pour chopper une image
        Xmax+=(aTargetSize-(Xmax-Xmin));
        Ymin-=(aTargetSize-(Ymax-Ymin));
         //std::cout << "\n échelle max dépassée " << std::endl;
        }
        std::string newBBOX= std::to_string(Xmin)+","+std::to_string(Ymin)+","+std::to_string(Xmax)+","+std::to_string(Ymax);
        // virgule encode en %2C dans sRequest
        boost::replace_all(BBOXold,",","%2C");
        boost::replace_all(sRequest,"&BBOX="+BBOXold,"&BBOX="+newBBOX);
        url+=sRequest;

        if ( W<aMinSize){
        if (globTest){
            std::cout << "\n échelle max dépassée, je change la old bounding Box " <<BBOXold << " par celle-ci " <<newBBOX<< "\n" << std::endl;
            std::cout << "url : " << url << std::endl;}
        }

        // image 1: CNSW
        std::string name0 = std::tmpnam(nullptr);
        std::string name = mTmpDir+name0.substr(5,name0.size()-5)+".png";

        CURL *curl_handle;
        curl_global_init(CURL_GLOBAL_ALL);
        curl_handle = curl_easy_init();
        curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);
        FILE* fp = fopen(name.c_str(), "wb");
        if(fp) {
            curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, fp);
            curl_easy_perform(curl_handle);
            fclose(fp);
        }

        // image 2: PTS (couleur parlante)
        url="https://geoservices.wallonie.be/arcgis/rest/services/SOL_SOUS_SOL/CNSW__PRINC_TYPES_SOLS/MapServer/export?"+sRequest;
        if (globTest){std::cout << "url 2 : " << url << std::endl;}
        name0 = std::tmpnam(nullptr);
        std::string name2 = mTmpDir+name0.substr(5,name0.size()-5)+".png";
                if (globTest){std::cout << "name2 tmp : " <<  name2 << std::endl;}
        // curl_easy_cleanup(curl_handle);
        curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);
        fp = fopen(name2.c_str(), "wb");
        if(fp) {
            curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, fp);
            curl_easy_perform(curl_handle);
            fclose(fp);
        }
            // lecture de la partie de l'image qui nous intéresse dans un WRaster puis envoie comme réponse
            int imSz(256);// ça c'est dans la requete que la taille est indiquée.
            Wt::WRasterImage pngImage("png", imSz, imSz);
            Wt::WPainter painter(&pngImage);
            Wt::WRectF destinationRect = Wt::WRectF(0.0,0.0,imSz, imSz);
            int wadj=imSz*(W/aTargetSize);
            Wt::WRectF sourceRect = Wt::WRectF(0.0,0.0,wadj,wadj);
            // 1) dessiner les couleurs de PTS
           try{
            Wt::WPainter::Image imagePTS(name2,name2);
              painter.drawImage(destinationRect, imagePTS, sourceRect);
            } catch (std::exception& e) { std::cout << "image PTS not valid " << std::endl;}
             try{
            Wt::WPainter::Image image(name,name);
            painter.drawImage(destinationRect, image, sourceRect);
            } catch (std::exception& e) { std::cout << "image cnsw not valid " << std::endl;}


            pngImage.done();
            pngImage.write(out);

        // suppression du fichier temporaire
        //boost::filesystem::remove(name);
        curl_easy_cleanup(curl_handle);
        curl_global_cleanup();
    //}

}
