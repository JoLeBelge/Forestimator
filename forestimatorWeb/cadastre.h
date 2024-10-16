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
#include <algorithm>

#include <unistd.h>
#include <cmath>
#include <sqlite3.h>
#include <tuple>

#include <Wt/Dbo/Dbo.h>
#include <Wt/Dbo/backend/Sqlite3.h>
#include <Wt/WSignal.h>
namespace fs = boost::filesystem ;
namespace dbo = Wt::Dbo;
// une classe qui ressemble un peu dans sa structure à celle de cnsw, qui sera membre du dicoApt
// sert à renseigner les chemins d'accès vers les shp, à lire les dbf, à retourner les polygones des communes, division et parcelles cadastrales

// tentative de serialize ; J'ai des tuple, ma classe capa, et des pointeurs uniques à sérialize. Ca ne compile pas, c'est peut-être un peu difficile pour moi. Par ailleur, pas besoin de sérialisé en xml, binaire c'est bon aussi et c'est plus simple

extern bool globTest;

std::string featureToGeoJSON(OGRFeature *f);

class capa;
class ptCadastre;

class capa{

public:
    capa(std::string aCaSecKey, std::string aCaPaKey,int aPID, std::map<int,std::tuple<int,std::string>> * aVDiv);
    capa(){};

    std::string CaSecKey, CaPaKey;
    int comINS, divCode, mPID;
    std::string section;

    template<class Action>
       void persist(Action& a)
       {
           dbo::field(a, CaSecKey,     "CaSecKey");
           dbo::field(a, CaPaKey, "CaPaKey");
           dbo::field(a, comINS,     "comINS");
           dbo::field(a, divCode,    "divCode");
           dbo::field(a, mPID,    "mPID");
           dbo::field(a, section,    "section");
       }
    private:

};

// mais je ne met pas encore de Wt dans cette classe.
class cadastre
{
public:
    cadastre(sqlite3 *db);
    void loadInfo();

    // retourne le chemin d'accès vers le fichier json qui contient le polygone au format json
    std::string createPolygonDiv(int aDivCode);
    std::string createPolygonCommune(int aINScode);
    std::string createPolygonPaCa(std::string aCaPaKey, int divCode);// trop lent
    std::string createPolygonPaCa(int aFID);

    // info nécessitant la lecture des objets capa mappé dans la bd sqlite
    std::vector<std::string> getSectionForDiv(int aDivCode, Wt::Dbo::Session * session);
    std::vector<dbo::ptr<capa> > getCaPaPtrVector(int aDivCode,std::string aSection,dbo::Session * session);
    dbo::ptr<capa> getCaPaPtr(std::string aCaPaKey,dbo::Session * session);

    void getCaPa4pt(double x, double y, ptCadastre * aPt);
    std::string Commune(int aComINS){
        std::string aRes("");
        if (mVCom.find(aComINS)!=mVCom.end()){
            aRes=mVCom.at(aComINS);
        }
        return aRes;
    }

    std::vector<std::pair<int,std::string>> getCommuneLabel(){
        std::map<int,std::string> aRes;
        std::vector<std::pair<int,std::string>> vec; // vecteur temp pour ordre alpha

        // copy key-value pairs from the map to the vector
        std::copy(mVCom.begin(), mVCom.end(), std::back_inserter<std::vector<std::pair<int,std::string>>>(vec));
        // sort the vector by increasing the order of its pair's second value
        // if the second value is equal, order by the pair's first value
        std::sort(vec.begin(), vec.end(),
                [](const std::pair<int,std::string> &l, const std::pair<int,std::string> &r)
                {
                        return l.second < r.second;
                });
        for (auto & kv : vec){
            aRes.emplace(std::make_pair(kv.first,kv.second+" "+std::to_string(kv.first)));
        }
        //return aRes;
        return vec;
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
    std::string mDirBDCadastre;

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
      //std::map<int,std::vector<std::unique_ptr<capa>>> mVCaPa;

      sqlite3 *db_;
};

class ptCadastre{
public:
    ptCadastre(std::shared_ptr<cadastre> aCadastre, double x, double y);
    std::string displayAllInfoInOverlay();
    Wt::Signal<std::string,std::string>& sendPolygone(){ return geoJson_; }
    Wt::Signal<std::string,std::string> geoJson_;
    void usePolyg4Stat();

    void setCaPaKey(std::string aKey){mCaPaKey=aKey;}
    void setCaPaFID(int aFID){mFID=aFID;}

private:
    dbo::Session session;
    std::shared_ptr<cadastre> mCad;
    std::string mCaPaKey,mCommune;
    int mFID;
};


#endif // CADASTRE_H
