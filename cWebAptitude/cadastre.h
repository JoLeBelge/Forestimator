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
/*
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>

#include <boost/serialization/unique_ptr.hpp>
*/
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/binary_object.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/map.hpp>
#include <boost/archive/detail/common_oarchive.hpp>

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
    friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {

            ar & CaSecKey;
                        ar &CaPaKey;
                        ar & comINS;
                        ar & divCode;
                        ar & mPID;
                        ar & section;
            // manque divisionName
        }
    void writeArchive(std::string aArchive);

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

    /*friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        //ar & BOOST_SERIALIZATION_NVP(mShpCommunePath);
        ar & BOOST_SERIALIZATION_NVP(mTmpDir);
        ar & BOOST_SERIALIZATION_NVP(mVDiv);
        ar & BOOST_SERIALIZATION_NVP(mVCaPa);
        ar & BOOST_SERIALIZATION_NVP(mVCom);
    }

    */

    friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & BOOST_SERIALIZATION_NVP(mVCaPa);
        }
         void writeArchive(std::string aArchive);

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

/*
Copyright 2011 Christopher Allen Ogden. All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are
permitted provided that the following conditions are met:
   1. Redistributions of source code must retain the above copyright notice, this list of
      conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright notice, this list
      of conditions and the following disclaimer in the documentation and/or other materials
      provided with the distribution.
THIS SOFTWARE IS PROVIDED BY CHRISTOPHER ALLEN OGDEN ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL CHRISTOPHER ALLEN OGDEN OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
The views and conclusions contained in the software and documentation are those of the
authors and should not be interpreted as representing official policies, either expressed
or implied, of Christopher Allen Ogden.


namespace boost {
namespace serialization {

template<uint N>
struct Serialize
{
    template<class Archive, typename... Args>
    static void serialize(Archive & ar, std::tuple<Args...> & t, const unsigned int version)
    {
        ar & std::get<N-1>(t);
        Serialize<N-1>::serialize(ar, t, version);
    }
};

template<>
struct Serialize<0>
{
    template<class Archive, typename... Args>
    static void serialize(Archive & ar, std::tuple<Args...> & t, const unsigned int version)
    {
        (void) ar;
        (void) t;
        (void) version;
    }
};

template<class Archive, typename... Args>
void serialize(Archive & ar, std::tuple<Args...> & t, const unsigned int version)
{
    Serialize<sizeof...(Args)>::serialize(ar, t, version);
}

}
}
*/

#endif // CADASTRE_H
