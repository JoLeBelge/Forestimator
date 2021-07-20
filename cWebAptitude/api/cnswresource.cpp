#include "cnswresource.h"

//double cmPerInch(0.0254);// le DPI est exprimé en pix par inch
//double max_scale(5000.0); // échelle m
//double wMin(225.5); // largeur de l'emprise minimum pour laquelle le serveur accepte de délivrer un image
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
    //std::cout << " pathInfo " << pathInfo <<  std::endl;

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
    //int DPI=std::stoi(*request.getParameterValues("DPI").data());
    //double scaleX=(Xmax-Xmin)*((double (DPI))/cmPerInch)/256.0;

    // largeur de l'emprise au sol
    double W=(Xmax-Xmin);
    std::string url="https://geoservices.wallonie.be/arcgis/rest/services/SOL_SOUS_SOL/CNSW/MapServer/export?";
    response.setMimeType("image/png");
    std::ostream& out = response.out();

    //if (scaleX>max_scale){
    // si emprise trop petite, le serveur refuse de servir une image
    if (W>260){
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
        // calcul d'un dpi qui nous permet de passer sous l'échelle maximum - OLD fonctionne pas
        /*
        DPI = (max_scale-10.0)*256.0*cmPerInch/(Xmax-Xmin);
        std::string oldDPI=*request.getParameterValues("DPI").data();
        boost::replace_all(sRequest,"&DPI="+oldDPI,"&DPI="+std::to_string(DPI));
        //std::cout << " échelle max dépassée, " << scaleX << ", je change le DPI pour mettre valeur " << DPI << std::endl;
        */
        std::string sRequest=request.queryString();
        // changer l'extend pour chopper une image
        Xmax+=(500-(Xmax-Xmin));
        Ymin-=(500-(Ymax-Ymin));
        std::string newBBOX= std::to_string(Xmin)+","+std::to_string(Ymin)+","+std::to_string(Xmax)+","+std::to_string(Ymax);
        // virgule encode en %2C dans sRequest
        boost::replace_all(BBOXold,",","%2C");
        boost::replace_all(sRequest,"&BBOX="+BBOXold,"&BBOX="+newBBOX);
        url+=sRequest;

        if (globTest){
            std::cout << "\n échelle max dépassée, je change la old bounding Box " <<BBOXold << " par celle-ci " <<newBBOX<< "\n" << std::endl;
            std::cout << "url : " << url << std::endl;}

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

            // lecture de la partie de l'image qui nous intéresse dans un WRaster puis envoie comme réponse
            Wt::WRasterImage pngImage("png", 256, 256);
            Wt::WPainter painter(&pngImage);
            Wt::WRectF destinationRect = Wt::WRectF(0.0,0.0,256, 256);
            int wadj=256.0*(W/500.0);
            Wt::WRectF sourceRect = Wt::WRectF(0.0,0.0,wadj,wadj);
            Wt::WPainter::Image image(name,name);
            painter.drawImage(destinationRect, image, sourceRect);
            pngImage.done();
            pngImage.write(out);
        }

        // suppression du fichier temporaire
        boost::filesystem::remove(name);
        curl_easy_cleanup(curl_handle);
        curl_global_cleanup();
    }

}
