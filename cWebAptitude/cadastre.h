#ifndef CADASTRE_H
#define CADASTRE_H
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
#include <tuple>
namespace fs = boost::filesystem ;
// une classe qui ressemble un peu dans sa structure à celle de cnsw, qui sera membre du dicoApt
// sert à renseigner les chemins d'accès vers les shp, à lire les dbf, à retourner les polygones des communes, division et parcelles cadastrales

// tentative de serialize ; J'ai des tuple, ma classe capa, et des pointeurs uniques à sérialize. Ca ne compile pas, c'est peut-être un peu difficile pour moi. Par ailleur, pas besoin de sérialisé en xml, binaire c'est bon aussi et c'est plus simple

std::string featureToGeoJSON(OGRFeature *f);

class capa;

class capa{

public:
    capa(std::string aCaSecKey, std::string aCaPaKey,int aPID, std::map<int,std::tuple<int,std::string>> * aVDiv);

    std::string CaSecKey, CaPaKey;
    int comINS, divCode, mPID;
    std::string section;
    std::string * divisionName;
    private:

};

// mais je ne met pas encore de Wt dans cette classe.
class cadastre
{
public:
    cadastre(sqlite3 *db);
    //cadastre(std::string xmlCadastre);// pour rendre plus rapide le démarrage de l'appli, je sauve cet objet au format xml
    void loadInfo();

    // retourne le chemin d'accès vers le fichier json qui contient le polygone au format json
    std::string createPolygonDiv(int aDivCode);
    std::string createPolygonCommune(int aINScode);
    std::string createPolygonPaCa(std::string aCaPaKey, int divCode);// trop lent
    std::string createPolygonPaCa(int aFID);

    std::vector<std::string> getSectionForDiv(int aDivCode);
    std::vector<capa*> getCaPaPtrVector(int aDivCode,std::string aSection);

    std::map<int,std::string> getCommuneLabel(){
        std::map<int,std::string> aRes;
        for (auto & kv : mVCom){
            aRes.emplace(std::make_pair(kv.first,kv.second+" "+std::to_string(kv.first)));
        }
        // problème ; pas dans l'ordre alphabétique... aie aie
        return aRes;
    }
    std::map<int,std::string> getDivisionLabel(int aCodeCommune){
        std::map<int,std::string> aRes;
        for (auto & kv : mVDiv){
            if (std::get<0>(kv.second)==aCodeCommune){
                aRes.emplace(std::make_pair(kv.first,std::get<1>(kv.second)));
            }
        }
        return aRes;
    }

    std::string saveFeatAsGEOJSON(OGRFeature *f);


    private:
      boost::filesystem::path mShpCommunePath;
      boost::filesystem::path mShpDivisionPath;
      boost::filesystem::path mShpParcellePath;

      std::string mTmpDir;

      // clé ; code INS commune. valeur ; nom français
      std::map<int,std::string> mVCom;
      // ça peut pas être une map avec comme clé le code postal communal car il y a plusieurs division par commune
      // clé :: code division. valeur ; paire code commune et nom français
      std::map<int,std::tuple<int,std::string>> mVDiv;
      //std::vector<std::unique_ptr<capa>> mVCaPa;
      //clé ; le code de la division.
      std::map<int,std::vector<std::unique_ptr<capa>>> mVCaPa;

      sqlite3 *db_;
};

#endif // CADASTRE_H
