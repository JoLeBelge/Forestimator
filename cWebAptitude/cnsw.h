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

namespace fs = boost::filesystem ;

enum class PEDO { DRAINAGE,
                  TEXTURE,
                  PROFONDEUR
                     };

class dicoPedo
{

public:
    dicoPedo(std::string aBDFile);
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
   std::string Profondeur(std::string aCode){
      std::string aRes("/");
      if (mPhase.find(aCode)!=mPhase.end()){aRes=mPhase.at(aCode);}
      return aRes;
   }

  std::string Profondeur(int aCode){
      std::string aRes("/");
      if (sToPhase.find(aCode)!=sToPhase.end()){aRes=Profondeur(sToPhase.at(aCode));}
      return aRes;
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
    // Phase 1 vers description, pour l'instant seulement phase 1
    std::map<std::string,std::string> mPhase;

    std::map<int,std::string> sigleIdToSigleStr;
    // index du sigle vers code Texture, drainage, ect
    std::map<int,std::string> sToTexture;
    std::map<int,std::string> sToDrainage;
    std::map<int,std::string> sToPhase;


};

class cnsw : public dicoPedo
{
public:
    cnsw(std::string aBDFile);
    std::vector<std::string> displayInfo(double x, double y,PEDO p);

    // extractInfo
    std::string getValue(double x, double y, PEDO p);
    // selectionne le polygone qui intersecte cette position et renvoi le FID
    int getIndexSol(double x, double y);
private:

};

#endif // CNSW_H
