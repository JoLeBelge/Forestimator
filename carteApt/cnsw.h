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

//enum class PEDO { DRAINAGE,
enum  PEDO {
    SIGLE,
    DRAINAGE,
    TEXTURE,
    PROFONDEUR,
    CHARGE,
    Last// pour itérer sur les enum
};

class dicoPedo
{

public:
    // deux constructeur ; un qui recoit un pointeur vers un BD déjà ouverte, l'autre qui reçcois le chemin d'accès et qui l'ouvre
    // dicoPedo(std::string aBDFile);
    dicoPedo(sqlite3 *db);

    void loadInfo();
    //void closeConnection();
    //int openConnection();
    std::string sIdToSigleStr(int aCode){
        std::string aRes("/");
        if (sigleIdToSigleStr.find(aCode)!=sigleIdToSigleStr.end()){aRes=sigleIdToSigleStr.at(aCode);}
        return aRes;
    }

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
    std::string TextureSigle(int aCode){
        std::string aRes("");
        if (sToTexture.find(aCode)!=sToTexture.end()){aRes=sToTexture.at(aCode);}
        return aRes;
    }

    std::string DrainageSigle(int aCode){
        std::string aRes("");
        if (sToDrainage.find(aCode)!=sToDrainage.end()){aRes=sToDrainage.at(aCode);}
        return aRes;
    }
    /* plus difficile car il y a plusieurs phase (1, 2, 3)
    std::string ProfondeurSigle(int aCode){
        std::string aRes("");
        if (sToPhase1.find(aCode)!=mPhase.end()){aRes=mPhase.at(aCode);}
        return aRes;
    }*/
    std::string chargeSigle(int aCode){
        std::string aRes("");
        if (sToCharge.find(aCode)!=sToCharge.end()){aRes=sToCharge.at(aCode);}
        return aRes;
    }
    std::string charge(int aCode){
        std::string aRes("/");
        if (sToCharge.find(aCode)!=sToCharge.end()){
            std::string sigleCharge=sToCharge.at(aCode);
           if (mCharge.find(sigleCharge)!=mCharge.end()){
            aRes=mCharge.at(sigleCharge);
           }
        }
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
        if (mPhase.find(aCode)!=mPhase.end()){aRes=mPhase.at(aCode);} else {
           // if (1){ std::cout << " pas de phase de profondeur pour  " << 1<< std::endl;}
        }
        return aRes;
    }

    std::string ProfondeurCourt(std::vector<std::string> aCode){
        std::string aRes("/");
        if (mPhaseCourt.find(aCode)!=mPhaseCourt.end()){aRes=mPhaseCourt.at(aCode);}
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
        //std::cout << "sigle " << sigleIdToSigleStr.at(aCode) << std::endl;
        return Profondeur(aKey);
    }
    std::string ProfondeurCourt(int aCode){
        std::string p1(""),p2(""),p3(""),p4(""),p5(""),p6(""),p7("");

        if (sToPhase1.find(aCode)!=sToPhase1.end()){p1=sToPhase1.at(aCode);}
        if (sToPhase2.find(aCode)!=sToPhase2.end()){p2=sToPhase2.at(aCode);}
        if (sToPhase3.find(aCode)!=sToPhase3.end()){p3=sToPhase3.at(aCode);}
        if (sToPhase4.find(aCode)!=sToPhase4.end()){p4=sToPhase4.at(aCode);}
        if (sToPhase5.find(aCode)!=sToPhase5.end()){p5=sToPhase5.at(aCode);}
        if (sToPhase6.find(aCode)!=sToPhase6.end()){p6=sToPhase6.at(aCode);}
        if (sToPhase7.find(aCode)!=sToPhase7.end()){p7=sToPhase7.at(aCode);}
        std::vector<std::string> aKey{p1,p2,p3,p4,p5,p6,p7};

        return ProfondeurCourt(aKey);
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

    // Phase vers description courte (juste chiffre de profondeurs)
    std::map<std::vector<std::string>,std::string> mPhaseCourt;

    // sigle vers description
    std::map<std::string,std::string> mCharge;


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

    std::map<int,std::string> sToCharge;

};

class cnsw : public std::enable_shared_from_this<cnsw>,public dicoPedo
{
public:
    cnsw(std::string aBDFile);
    cnsw(sqlite3 *db);
    std::vector<std::string> displayInfo(double x, double y,PEDO p);

    // void loadCNSW(); j'avais testé l'action d'ouvrir une seule fois la couche (dataset est membre de cnsw) pour voir l'impact sur les ressources en mémoire du soft
    // extractInfo
    std::string getValue(double x, double y, PEDO p);
    // analyse surfacique ; retourne une map avec clé=sigle pédo et valeur=pourcentage en surface pour ce sigle
    std::map<int,double> anaSurface(OGRGeometry *poGeom);
    // selectionne le polygone qui intersecte cette position et renvoi le FID
    int getIndexSol(double x, double y);

private:

};

// en fait cette classe pourrait très bien avoir comme membre uniquement le sigle pédo, puis c'est le dico pedo qui gère tout.
class ptPedo{
public:
    ptPedo(std::shared_ptr<cnsw> dico, int aIdSigle);
    ptPedo(std::shared_ptr<cnsw> dico, double x, double y);
    std::vector<std::string> displayInfo(PEDO p);

    std::string displayAllInfoInOverlay();

private:
    std::shared_ptr<cnsw> mDico;
    int idSigle;
    std::string dDrainage,dTexture,dProf,dCharge; // d pour description
};

// analyse surfacique
class surfPedo{
public:
    surfPedo( std::shared_ptr<cnsw> dico, OGRGeometry *poGeom );

    std::string getSummary(PEDO p);

    // retourne le symbole de Texture majoritaire ainsi que sa proportion en surface
    std::pair<std::string,double> getMajTexture();
    // retourne le symbole de Drainage majoritaire ainsi que sa proportion en surface
    std::pair<std::string,double> getMajDrainage();

    std::pair<std::string,double> getMajProf();
    // pour avoir uniquement les chiffres, sans le texte en détails
    std::pair<std::string,double> getMajProfCourt();

    void catPropSurf(){
        std::cout << "surfPedo details;\n";
        for (auto kv : propSurf){
            std::cout << "sigle " << mDico->sIdToSigleStr(kv.first) << ", porportion surf " << kv.second << std::endl;
        }
    }
private:
    std::shared_ptr<cnsw> mDico;
    std::map<int,double> propSurf;
    std::string dDrainage,dTexture,dProf;

};

#endif // CNSW_H
