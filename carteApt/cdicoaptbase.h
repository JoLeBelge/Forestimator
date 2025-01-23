#ifndef CDICOAPTBASE_H
#define CDICOAPTBASE_H
#include <sqlite3.h>
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <boost/algorithm/string/replace.hpp>
#include <boost/range/adaptor/map.hpp>
#include "boost/filesystem.hpp"
#include <unistd.h>
#include "color.h"

#include <Wt/Dbo/Dbo.h>
#include <Wt/Dbo/backend/Sqlite3.h>
#include <Wt/WSignal.h>

#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wreorder"

namespace dbo = Wt::Dbo;

enum class FeRe {Feuillus,
                 Resineux,
                 Autre
                };

// ça c'était utilisé pour le calcul des carte apt, des tuiles et du code mapserveur. Encore utilisé par l'API forestimator
enum TypeCarte {Apt, Potentiel, Station1, Habitats,NH,NT,Topo,AE,SS,ZBIO,CSArdenne,CSLorraine,MNH,Composition,MNT16b};

enum class TypeWMS {WMS,
                    ArcGisRest
                   };

TypeWMS str2TypeWMS(const std::string& str);

std::string roundDouble(double d, int precisionVal=1);

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
  size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
  return written;
}


class cEss;
class WMSinfo;
class cdicoAptBase; // utilisé par plusieurs appli (Forestimator, phytospy)

class caracteristiqueCS{
public:
    caracteristiqueCS(int zbio,int station_id,int VCP, int SES,int SCC,int RCS, int PB);
    caracteristiqueCS():zbio(0),station_id(0),VCP(0),SES(0),SCC(0),RCS(0),PB(0),tass_sol(0),N2000(""),N2000_maj(""),Wal(""),Wal_maj(""){}
    int VCP,SES,SCC,RCS,PB,tass_sol;
    int zbio,station_id;
    std::string Wal,Wal_maj,N2000,N2000_maj;
    template<class Action>
       void persist(Action& a)
       {
           dbo::field(a, zbio,     "zbio");
           dbo::field(a, station_id, "station_id");
           dbo::field(a, VCP,     "VCP");
           dbo::field(a, SES,    "SES");
           dbo::field(a, SCC,    "SCC"); //sensibilité aux changement climatique, plus de microclimat
           dbo::field(a, RCS,    "RCS");
           dbo::field(a, PB,    "PB");
           dbo::field(a, Wal,    "Wal");
           dbo::field(a, Wal_maj,    "Wal_maj");
           dbo::field(a, N2000,    "N2000");
           dbo::field(a, N2000_maj,    "N2000_maj");
           dbo::field(a, tass_sol,    "tass_sol");

           //prod_b -> mm que PB donc je charge pas
       }
       caracteristiqueCS(const caracteristiqueCS * c):zbio(c->zbio),station_id(c->station_id),VCP(c->VCP),SES(c->SES),SCC(c->SCC),RCS(c->RCS),PB(c->PB),Wal(c->Wal),Wal_maj(c->Wal_maj),N2000(c->N2000),N2000_maj(c->N2000_maj),tass_sol(c->tass_sol){}
    private:
};


class cdicoAptBase : public std::enable_shared_from_this<cdicoAptBase>
{
public:
    void closeConnection();
    int openConnection();
    cdicoAptBase(std::string aBDFile);
    ~cdicoAptBase(){
        std::cout << "destruction du dico ; la fin d'une longue histoire (bug)" << std::endl;}

    std::map<int,std::map<std::string,int>> getFEEApt(std::string aCodeEs);
    std::map<int,int> getZBIOApt(std::string aCodeEs);
    std::map<int,std::map<int,int>> getRisqueTopo(std::string aCodeEs);
    std::map<int, std::map<std::tuple<int, std::string>, int> > getCSApt(std::string aCodeEs);
    std::map<int,std::map<std::tuple<int, std::string>,int>> getCSRisqueClim(std::string aCodeEs);


    std::map<int,std::string>  * NH(){return  &Dico_NH;}
    std::map<int,std::string>  * NT(){return  &Dico_NT;}
    std::map<int,std::string>  * code2NTNH(){return  &Dico_code2NTNH;}
    std::map<std::string,int>  * NTNH(){return  &Dico_NTNH2Code;}
    std::map<int,std::string>  * code2Apt(){return  &Dico_code2Apt;}
    std::map<int,std::string>  * code2AptFull(){return  &Dico_code2AptFull;}
    std::map<std::string,int>  * Apt(){return  &Dico_Apt;}
    std::map<int,int>  * risqueCat(){return  &Dico_risqueCategorie;}

    std::map<int,std::string>  * topo(){return  &Dico_topo;}
    // Accès sécurisé aux dictionnaire
    std::string NT(int aCode){
        std::string aRes("not found");
        if (Dico_NT.find(aCode)!=Dico_NT.end()){aRes=Dico_NT.at(aCode);}
        return aRes;
    }
    std::string NH(int aCode){
        std::string aRes("not found");
        if (Dico_NH.find(aCode)!=Dico_NH.end()){aRes=Dico_NH.at(aCode);}
        return aRes;
    }
    int posEcoNH(int aCode){
        int aRes(0);
        if (Dico_NHposEco.find(aCode)!=Dico_NHposEco.end()){aRes=Dico_NHposEco.at(aCode);}
        return aRes;
    }
    int posEcoNT(int aCode){
        int aRes(0);
        if (Dico_NTposEco.find(aCode)!=Dico_NTposEco.end()){aRes=Dico_NTposEco.at(aCode);}
        return aRes;
    }
    int rasterNH2Gr(int aCode){
        int aRes(0);
        if (Dico_rasterNH2groupe.find(aCode)!=Dico_rasterNH2groupe.end()){aRes=Dico_rasterNH2groupe.at(aCode);}
        return aRes;
    }

    std::vector<int> NHGr(){
        std::vector<int> aRes ;
        for (auto kv : Dico_rasterNH2groupe){
            int gr = kv.second;
            std::vector<int>::iterator it = std::find(aRes.begin(), aRes.end(), gr);
            if (it == aRes.end()){
                aRes.push_back(gr);
            }
        }
        return aRes;
    }
    std::string code2NTNH(int aCode){
        std::string aRes("not found");
        if (Dico_code2NTNH.find(aCode)!=Dico_code2NTNH.end()){aRes=Dico_code2NTNH.at(aCode);}
        return aRes;
    }
    std::string code2Apt(int aCode){
        std::string aRes("not found\n");
        if (aCode==20) aRes="Pas d'aptitude pour cette Zbio";
        if (aCode==0) aRes="Pas d'aptitude";
        if (Dico_code2Apt.find(aCode)!=Dico_code2Apt.end()){aRes=Dico_code2Apt.at(aCode);}
        return aRes;
    }
    std::string code2AptFull(int aCode){
        std::string aRes("not found\n");
        if (aCode==20) aRes="Pas d'aptitude pour cette Zbio";
        if (aCode==0) aRes="Pas d'aptitude";
        if (Dico_code2AptFull.find(aCode)!=Dico_code2AptFull.end()){aRes=Dico_code2AptFull.at(aCode);}
        return aRes;
    }

    std::string code2Recommandation(int aCode){
        std::string aRes("not found\n");
        if (Dico_code2Recommandation.find(aCode)!=Dico_code2Recommandation.end()){aRes=Dico_code2Recommandation.at(aCode);}
        return aRes;
    }
    int Apt(std::string aCode){
        int aRes(777);
        if (Dico_Apt.find(aCode)!=Dico_Apt.end()){aRes=Dico_Apt.at(aCode);}
        return aRes;
    }
   std::string CSClim(int aCode){
        std::string aRes("");
        if (Dico_CSClim.find(aCode)!=Dico_CSClim.end()){aRes=Dico_CSClim.at(aCode);}
        return aRes;
    }

    std::string accroEss2Nom(std::string aCode){
        std::string aRes("");
        if (Dico_codeEs2NomFR.find(aCode)!=Dico_codeEs2NomFR.end()){aRes=Dico_codeEs2NomFR.at(aCode);}
        return aRes;
    }
    std::string accroEss2prefix(std::string aCode){
        std::string aRes("");
        if (Dico_code2prefix.find(aCode)!=Dico_code2prefix.end()){aRes=Dico_code2prefix.at(aCode);}
        return aRes;
    }
    // on se débarasse des double aptitude en choisisant la plus contraignante
    int AptContraignante(int aCode){
        int aRes(0);
        if (Dico_AptDouble2AptContr.find(aCode)!=Dico_AptDouble2AptContr.end()){aRes=Dico_AptDouble2AptContr.at(aCode);}
        return aRes;
    }
    int AptNonContraignante(int aCode){
        int aRes(0);
        if (Dico_AptDouble2AptNonContr.find(aCode)!=Dico_AptDouble2AptNonContr.end()){aRes=Dico_AptDouble2AptNonContr.at(aCode);}
        return aRes;
    }

    bool isDoubleApt(int aCode){
        bool aRes(0);
        if (AptContraignante(aCode)!=AptNonContraignante(aCode)){aRes=1;}
        return aRes;
    }

    // on améliore l'aptitude car facteur de compensation
    int AptSurcote(int aCode){
        int aRes(aCode);
        if (Dico_AptSurcote.find(aCode)!=Dico_AptSurcote.end()){aRes=Dico_AptSurcote.at(aCode);}
        return aRes;
    }
    // on dégrade l'aptitude car facteur agravant
    int AptSouscote(int aCode){
        int aRes(aCode);
        if (Dico_AptSouscote.find(aCode)!=Dico_AptSouscote.end()){aRes=Dico_AptSouscote.at(aCode);}
        return aRes;
    }
    std::map<int,std::string> * ZBIO(){return  &Dico_ZBIO;}

    std::string ZBIO(int aCode){
        std::string aRes("not found");
        if (Dico_ZBIO.find(aCode)!=Dico_ZBIO.end()){aRes=Dico_ZBIO.at(aCode);}
        return aRes;
    }

    std::string ZBIO2CSlay(int aCode){
        std::string aRes("");
        if (Dico_ZBIO2layCS.find(aCode)!=Dico_ZBIO2layCS.end()){aRes=Dico_ZBIO2layCS.at(aCode);}
        return aRes;
    }
    int ZBIO2CSid(int aCode){
        int aRes(0);
        if (Dico_ZBIO2CSid.find(aCode)!=Dico_ZBIO2CSid.end()){aRes=Dico_ZBIO2CSid.at(aCode);}
        return aRes;
    }

    std::string TOPO(int aCode){
        std::string aRes("not found");
        if (Dico_topo.find(aCode)!=Dico_topo.end()){aRes=Dico_topo.at(aCode);}
        return aRes;
    }
    int Risque(std::string aStr){
        int aRes(0);
        if (Dico_risque2Code.find(aStr)!=Dico_risque2Code.end()){aRes=Dico_risque2Code.at(aStr);}
        return aRes;
    }
    std::string Risque(int aCode){
        std::string aRes("pas de risque pour ce code");
        if (Dico_risque.find(aCode)!=Dico_risque.end()){aRes=Dico_risque.at(aCode);}
        return aRes;
    }
    int risqueCat(int aCode){
        int aRes(0);
        if (Dico_risqueCategorie.find(aCode)!=Dico_risqueCategorie.end()){aRes=Dico_risqueCategorie.at(aCode);}
        return aRes;
    }

    int groupeNH2Nb(int aCode){
        int aRes(0);
        if (dico_groupeNH2Nb.find(aCode)!=dico_groupeNH2Nb.end()){aRes=dico_groupeNH2Nb.at(aCode);}
        return aRes;
    }
    int groupeNH2NHStart(int aCode){
        int aRes(0);
        if (dico_groupeNH2NHStart.find(aCode)!=dico_groupeNH2NHStart.end()){aRes=dico_groupeNH2NHStart.at(aCode);}
        return aRes;
    }
    std::string groupeNH2Label(int aCode){
        std::string  aRes("");
        if (dico_groupeNH2Label.find(aCode)!=dico_groupeNH2Label.end()){aRes=dico_groupeNH2Label.at(aCode);}
        return aRes;
    }
    std::string NTLabel(int aCode){
        std::string  aRes("");
        if (Dico_NTLabel.find(aCode)!=Dico_NTLabel.end()){aRes=Dico_NTLabel.at(aCode);}
        return aRes;
    }

    FeRe accroEss2FeRe(std::string aEss){
        FeRe aRes(FeRe::Feuillus);
        switch (Dico_F_R.at(aEss)) {
        case 1:{
            aRes=FeRe::Feuillus;
            break;}
        case 2:{
            aRes=FeRe::Resineux;
            break;}
        default:
            break;
        }
        return aRes;
    }

    std::vector<std::string> getAllAcroEss(){
        std::vector<std::string> aRes;
        for (auto es : Dico_codeEs2NomFR){
            aRes.push_back(es.first);
        }
        return aRes;
    }

    std::map<std::string,std::string>  Dico_AptFull2AptAcro;// j'en ai besoin dans les batonnetApt
    // dans l'ordre alphabétique
    std::vector<std::string> Dico_Ess;

    std::shared_ptr<cEss> getEss(std::string aCode);

    bool hasEss(std::string aCode){
        bool aRes(0);
        if (mVEss.find(aCode)!=mVEss.end()){aRes=1;}
        return aRes;
    }
    std::map<std::string,std::shared_ptr<cEss>> getAllEss(){return mVEss;std::cout << "getAllEss()" << std::endl;}

    std::string File(std::string aCode){
        std::string aRes("");
        if (Dico_GISfile.find(aCode)!=Dico_GISfile.end()){aRes=Dico_GISfile.at(aCode);}
        return aRes;
    }

    std::map<std::string,std::string>  * codeEs2Nom(){return  &Dico_codeEs2NomFR;}
    std::map<std::string,std::shared_ptr<color>> colors;

    std::shared_ptr<color> Apt2col(int aCode){
        std::shared_ptr<color> aRes=std::make_shared<color>(0,0,0);
        if (Dico_codeApt2col.find(aCode)!=Dico_codeApt2col.end()){aRes=Dico_codeApt2col.at(aCode);}
        return aRes;
    }
    std::shared_ptr<color> getColor(std::string aCode){
        std::shared_ptr<color> aRes=std::make_shared<color>(0,0,0);
        if (colors.find(aCode)!=colors.end()){aRes=colors.at(aCode);
        } else { std::cout << "color " << aCode << " not found"<< std::endl;}
        return aRes;
    }

    std::string getStationMaj(int zbio,int US){
    std::string aRes="";
    int zbioKey=ZBIO2CSid(zbio);
         if (Dico_station_varMaj.find(zbioKey)!=Dico_station_varMaj.end()){
             if (Dico_station_varMaj.at(zbioKey).find(US)!=Dico_station_varMaj.at(zbioKey).end()){
                 aRes=Dico_station_varMaj.at(zbioKey).at(US);
             }
         }
    return aRes;
    }
    bool isStationMaj(int zbio,int US, std::string var){
    bool aRes(0);
        int zbioKey=ZBIO2CSid(zbio);
         if (Dico_station_varMaj.find(zbioKey)!=Dico_station_varMaj.end()){
             if (Dico_station_varMaj.at(zbioKey).find(US)!=Dico_station_varMaj.at(zbioKey).end()){
                 if (Dico_station_varMaj.at(zbioKey).at(US)==var){aRes=1;}
             }
         }
    return aRes;
    }
    caracteristiqueCS getKKCS(int zbio, int station_id){
        caracteristiqueCS aRes;
        std::pair<int, int> key(zbio,station_id);
        if (Dico_US2KK.find(key)!=Dico_US2KK.end()){aRes=Dico_US2KK.at(key);}
        return aRes;
    }

    int rasterValHabitats(std::string aCode){
        int aRes(0);
        if (Dico_code2rasterValHabitat.find(aCode)!=Dico_code2rasterValHabitat.end()){aRes=Dico_code2rasterValHabitat.at(aCode);}
        return aRes;
    }



protected:
    std::string mBDpath;
    sqlite3 **db_;
    sqlite3 * ptDb_;

    // code essence 2 code groupe de couche pour catalogue de couches
    std::map<std::string,std::string> Dico_lay2groupe;
    // booléen expert assigné au groupe de couche
    std::map<std::string,bool> Dico_groupeExpert;
    std::map<std::string,std::string>  Dico_GISfile;
    std::map<std::string,std::string>  Dico_RasterType;
    // continu vs classe
    std::map<std::string,std::string>  Dico_RasterVar;
    std::map<std::string,bool>  Dico_RasterVisu;// les couches que l'on peux visualiser dans la partie carto
    std::map<std::string,bool>  Dico_RasterStat;// les couches sur lesquelles on peut calculer des statistiques
    std::map<std::string,bool>  Dico_RasterStatP;// les couches sur lesquelles on peut calculer des statistiques ponctuelles
    // description peuplement vs description station
    std::map<std::string,std::string>  Dico_RasterCategorie;
    std::map<std::string,std::string>  Dico_RasterNomComplet;
    std::map<std::string,std::string>  Dico_RasterNomCourt;
    std::map<std::string,bool>  Dico_RasterExpert;
    std::map<std::string,double>  Dico_RasterGain;
    // pour savoir de quelle table provient les info du raster, fichiersGIS ou layerApt, car j'ai besoin du nom de la table pour charger le dicitonnaire (pour l'instant)
    std::map<std::string,std::string>  Dico_RasterTable;
    //code ess vers nom français
    std::map<std::string,std::string> Dico_codeEs2NomFR;
    std::map<std::string,std::string> Dico_code2prefix;
    // code essence 2 code groupe "feuillus" vs "Resineux (1=feuillus, 2=Résineux)
    std::map<std::string,int> Dico_F_R;
    std::map<int,std::string>  Dico_NH;
    // c'est dans mes analyses phyto que j'ai besoin de grouper les niveaux hydriques en groupe
    std::map<int,int>  Dico_rasterNH2groupe;
    // code NH vers position Y dans l'écogramme
    std::map<int,int>  Dico_NHposEco;
    std::map<int,int>  Dico_NTposEco;
    std::map<int,std::string>  Dico_NT;
    std::map<int,std::string>  Dico_NTLabel;
    std::map<int,std::string>  Dico_code2NTNH;
    std::map<std::string,int>  Dico_NTNH2Code;
    std::map<std::string,int>  Dico_Apt;
    std::map<int,std::string>  Dico_code2Apt;// apt sous forme d'acro, O, T, TE, ect
    std::map<int,std::string> Dico_code2AptFull;
    std::map<int,int>  Dico_AptDouble2AptContr;
    std::map<int,int>  Dico_AptDouble2AptNonContr;
    std::map<int,int>  Dico_AptSouscote;
    std::map<int,int>  Dico_AptSurcote;
    std::map<int,std::string>  Dico_code2Recommandation;
    std::map<int,std::string>  Dico_CSClim;
    std::map<int,std::string>  Dico_CSClim2col;
    // les codes aptitudes sont classé dans un ordre fonction de la contrainte, permet de comparer deux aptitude
    std::map<int,int>  Dico_Apt2OrdreContr;
    std::map<int,std::string> Dico_risque;
    std::map<std::string,int> Dico_risque2Code;
    // on regroupe les 5 risques en 3 catégorie
    std::map<int,int> Dico_risqueCategorie;
    std::map<int,std::string>  Dico_topo;
    std::map<int,std::string>  Dico_ZBIO;
    std::map<int,std::string>  Dico_ZBIO2layCS;
    std::map<int,int>  Dico_ZBIO2CSid;
    std::map<int,std::string>  dico_groupeNH2Label;// pour l'écogramme avec visu prédiciton random forest
    std::map<int,int>  dico_groupeNH2Nb;//nombre de niveau NH par groupe
    std::map<int,int>  dico_groupeNH2NHStart;// code nh qui débute le groupe.
    std::map<int,std::shared_ptr<color>> Dico_codeApt2col;

    // clé 1 : zbio, clé 2: id station+variance,value ; nom de la sation cartograhique
    std::map<int,std::map<std::tuple<int, std::string>,std::string>>  Dico_station;
    // clé 1 : zbio, clé 2: id station+variance,value ; nom de la variante
    std::map<int,std::map<std::tuple<int, std::string>,std::string>>  Dico_station_varName;
    // bool pour savoir si c'est la variante majoritaire. clé 1 zbio clé 2 US val var code
    std::map<int,std::map<int,std::string>>  Dico_station_varMaj;

    // key ; code le la couche layer. value ; les infos nécessaire pour charger le wms
    std::map<std::string,WMSinfo>  Dico_WMS;

    std::map<std::string,int>  Dico_code2rasterValHabitat;
    //std::map<std::string,std::string>  Dico_code2NomHabitat; // pas besoin non?
    std::map<std::string,std::string> Dico_codeKK2Nom;
    std::map<std::pair<int,int>,caracteristiqueCS> Dico_US2KK;

    // clé ; code ess. val ; pointeur vers essence
    std::map<std::string,std::shared_ptr<cEss>> mVEss;
};


class cEss
{
public:
    cEss(std::string aCodeEs,cdicoAptBase * aDico);

    //effectue la confrontation Apt Zbio et AptHydroTrophiue si hierarchique = true, sinon renvoie l'aptitude de l'écogramme
    int getApt(int aCodeNT, int aCodeNH, int aZbio, bool hierachique=true, int aTopo=666);
    // retourne l'aptitude global de la zone bioclimatique
    int getApt(int aZbio);
    // retourne l'aptitude du catalogue de station
    int getApt(int aZbio, int US, std::string aVar="", bool withClim=true);
    int getCSClim(int zbio, int US, std::string aVar="");
    bool hasCSApt(){
        bool aRes(1);
        if (mAptCS.size()==0) {
            aRes=0;
            //std::cout << "essence " << mNomFR << " n'as pas d'aptitude pour CS" << std::endl;
        }
        return aRes;
    }
    bool hasFEEApt(){
        bool aRes(0);
        if (mEcoVal.size()>0 && mEcoVal.at(1).size()>0) {
            aRes=1;
            //std::cout << "essence " << mNomFR << " n'as pas d'aptitude pour FEE" << std::endl;
        }
        return aRes;
    }

    int getMaxAptHT(std::vector<std::tuple<int,int>> aVNtnh4Comparison, int aZbio){
        int aRes(4);
        if (aVNtnh4Comparison.size()>0){
            std::tuple<int,int> ntnh0 =aVNtnh4Comparison.at(0);
            int nt0=std::get<0>(ntnh0);
            int nh0=std::get<1>(ntnh0);
            aRes = mDico->AptNonContraignante(getApt(nt0,nh0,aZbio,false));
            for (size_t c(1); c < aVNtnh4Comparison.size();c++){
                std::tuple<int,int> ntnh = aVNtnh4Comparison.at(c);
                int nt=std::get<0>(ntnh);
                int nh=std::get<1>(ntnh);
                int aptHT2 = mDico->AptNonContraignante(getApt(nt,nh,aZbio,false));
                if (aptHT2>aRes){
                    aRes=aptHT2;}
            }
        }
        return aRes;
    }

    bool hasApt(){
        bool aRes(1);
        if (mEcoVal.size()==0 && mAptCS.size()==0) {
            aRes=0;
            std::cout << "essence " << mNomFR << " n'as pas d'aptitude pour FEE ni pour Catalogue de stations" << std::endl;
        }
        return aRes;
    }

    int getFinalApt(int aCodeNT,int aCodeNH, int aZbio, int topo){
        return getApt(aCodeNT,aCodeNH,aZbio,true,topo);
    }

    // renvoie l'apt climatique compensée par situation topographique
    int corrigAptBioRisqueTopo(int aptBio,int topo,int zbio);

    std::string Nom(){return mNomFR;}
    std::string Code(){return mCode;}

    // aptitude ecograme : clé chaine charactère ; c'est la combinaison ntxnh du genre "A2p5" ou "Mm4
    std::map<int,std::map<std::string,int>> mEcoVal;
    // aptitude pour chaque zone bioclim
    std::map<int,int> mAptZbio;
    // aptitude pour catalogue de station
    // clé ; zone bioclim/ région. Value ; une map -> clé = identifiant de la station (id + variante) Value ; aptitude
    std::map<int,std::map<std::tuple<int, std::string>,int>> mAptCS;
    // sensibilité climatique
     std::map<int,std::map<std::tuple<int, std::string>,int>> mCSClim;
    // clé ; zone bioclim/ région. Value ; une map -> clé ; id situation topo. valeur ; code risque
    std::map<int,std::map<int,int>> mRisqueTopo;

    int getRisque(int zbio,int topo){
        int aRes(0);
        if (mRisqueTopo.find(zbio)!=mRisqueTopo.end() && mRisqueTopo.at(zbio).find(topo)!=mRisqueTopo.at(zbio).end()){
            aRes=mRisqueTopo.at(zbio).at(topo);
        }
        return aRes;
    }
    // savoir si il faut utiliser la situation topo comme facteur de compensation ou d'aggravation
    bool hasRisqueComp(int zbio,int topo);

    cdicoAptBase * Dico(){return mDico;}
    std::string printRisque();

    //TypeCarte Type(){return mType;}

private:
    FeRe mFeRe;
    //TypeCarte mType;
    cdicoAptBase *mDico;
    std::string mCode, mNomFR, mF_R, mPrefix;
};

class WMSinfo
{
public:
    std::string WMSLayerName()const{return mWMSLayerName;}
    std::string WMSURL()const{return mUrl;}
    TypeWMS getTypeWMS(){return mTypeWMS;}

    WMSinfo():mUrl(""),mWMSLayerName("toto"){}
    WMSinfo(std::string url,std::string layer, std::string aTypeGeoservice,std::string attribution):mUrl(url),mWMSLayerName(layer),mWMSattribution(attribution){
        mTypeWMS=str2TypeWMS(aTypeGeoservice);
    }
    std::string mUrl, mWMSLayerName, mWMSattribution;

    TypeWMS mTypeWMS;
};


#endif // CDICOAPTBASE_H
