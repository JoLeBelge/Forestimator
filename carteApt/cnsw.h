#ifndef CNSW_H
#define CNSW_H

/* une classe qui contient les chemins d'accès vers le shp de la carte numérique des sols de wallonie, des dictionnaires pour les sigles et les champs
, des méthodes pour les analyse ponctuelles et pour les analyses surfaciques
*/

#include "ogrsf_frmts.h"
#include "gdal_utils.h"
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/map.hpp>
#include "boost/filesystem.hpp"
#include <unistd.h>
#include <cmath>
#include <sqlite3.h>


// toutes les map des dictionnaires pédo et les accesseurs
class dicoPedo;
// les fonctions liées à l'application Forestimator
class cnsw;
// j'ai fait deux classes pour pouvoir le cas échant réutiliser la partie dicoPedo dans d'autre appli

// analyse ponctuelle
class ptPedo;
// analyse surfacique
class surfPedo;

namespace fs = boost::filesystem ;

enum class PEDO { DRAINAGE,
                  TEXTURE,
                  PROFONDEUR
                };

class dicoPedo
{

public:
    // deux constructeur ; un qui recoit un pointeur vers un BD déjà ouverte, l'autre qui reçcois le chemin d'accès et qui l'ouvre
    dicoPedo(std::string aBDFile);
    dicoPedo(sqlite3 * db);

    void loadInfo();
    void closeConnection();
    int openConnection();

    std::string Texture(std::string aCode){
        std::string aRes("/");
        if (mTexture.find(aCode)!=mTexture.end()){aRes=mTexture.at(aCode);}
        return aRes;
    }

    std::string Texture(int aCode){
        std::string aRes("/");
        if (sToTexture.find(aCode)!=sToTexture.end()){aRes=Texture(sToTexture.at(aCode));}
        return aRes;
    }

    std::string Drainage(std::string aText,std::string aDrainage){
        std::string aRes("/");
        std::pair<std::string,std::string> aCode=std::make_pair(aText,aDrainage);
        if (mDrainage.find(aCode)!=mDrainage.end()){aRes=mDrainage.at(aCode);}
        return aRes;
    }

    std::string Drainage(int aCode){
        std::string aRes("/");
        if (sToDrainage.find(aCode)!=sToDrainage.end()){aRes=Drainage(sToTexture.at(aCode),sToDrainage.at(aCode));}
        return aRes;
    }
    std::string Profondeur(std::vector<std::string> aCode){
        std::string aRes("/");
        if (mPhase.find(aCode)!=mPhase.end()){aRes=mPhase.at(aCode);}
        return aRes;
    }

    std::string roundDouble(double d, int precisionVal);

    // la description de la profondeur dépend des valeurs de toutes les phase
    std::string Profondeur(int aCode){

        std::string p1(""),p2(""),p3(""),p4(""),p5(""),p6(""),p7("");

        if (sToPhase1.find(aCode)!=sToPhase1.end()){p1=sToPhase1.at(aCode);}
        if (sToPhase2.find(aCode)!=sToPhase2.end()){p2=sToPhase2.at(aCode);}
        if (sToPhase3.find(aCode)!=sToPhase3.end()){p3=sToPhase3.at(aCode);}
        if (sToPhase4.find(aCode)!=sToPhase4.end()){p4=sToPhase4.at(aCode);}
        if (sToPhase5.find(aCode)!=sToPhase5.end()){p5=sToPhase5.at(aCode);}
        if (sToPhase6.find(aCode)!=sToPhase6.end()){p6=sToPhase6.at(aCode);}
        if (sToPhase7.find(aCode)!=sToPhase7.end()){p7=sToPhase7.at(aCode);}
        std::vector<std::string> aKey{p1,p2,p3,p4,p5,p6,p7};

        return Profondeur(aKey);
    }
protected:
    boost::filesystem::path mShpPath;
private:

    std::string mBDpath;
    sqlite3 *db_;

    // clé ; code texture + code drainage . Valeur ; description drainage
    std::map<std::pair<std::string,std::string>, std::string> mDrainage;

    // sigle vers description
    std::map<std::string,std::string> mTexture;
    // Phase vers description
    std::map<std::vector<std::string>,std::string> mPhase;


    std::map<int,std::string> sigleIdToSigleStr;
    // index du sigle vers code Texture, drainage, ect
    std::map<int,std::string> sToTexture;
    std::map<int,std::string> sToDrainage;
    std::map<int,std::string> sToPhase1;
    std::map<int,std::string> sToPhase2;
    std::map<int,std::string> sToPhase3;
    std::map<int,std::string> sToPhase4;
    std::map<int,std::string> sToPhase5;
    std::map<int,std::string> sToPhase6;
    std::map<int,std::string> sToPhase7;

};

class cnsw : public dicoPedo
{
public:
    cnsw(std::string aBDFile);
    cnsw(sqlite3 * db);
    std::vector<std::string> displayInfo(double x, double y,PEDO p);

    // extractInfo
    std::string getValue(double x, double y, PEDO p);
    // analyse surfacique ; retourne une map avec clé=sigle pédo et valeur=pourcentage en surface pour ce sigle
    std::map<int,double> anaSurface(OGRGeometry *poGeom);
    // selectionne le polygone qui intersecte cette position et renvoi le FID
    int getIndexSol(double x, double y);
private:

};


class ptPedo{
public:
    ptPedo(dicoPedo * dico, int aIdSigle);
    ptPedo(cnsw *dico, double x, double y);
    std::vector<std::string> displayInfo(PEDO p);

private:
    dicoPedo * mDico;
    int idSigle;
    std::string dDrainage,dTexture,dProf; // d pour description
};

// analyse surfacique
class surfPedo{
public:
    surfPedo(cnsw *dico, OGRGeometry *poGeom );

    std::string getSummary(PEDO p);


private:
    dicoPedo * mDico;
    std::map<int,double> propSurf;
    std::string dDrainage,dTexture,dProf;
};

#endif // CNSW_H
