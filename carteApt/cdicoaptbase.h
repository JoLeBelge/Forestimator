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

enum class FeRe {Feuillus,
                 Resineux,
                 Autre
                   };

// octobre 2021 ; j'aimerai utiliser dans phytospy le dico Apt, entre autre pour concevoir une mise en page des matrices d'aptitudes.
// j'aimerai partager le dico Apt entre forestimator et phytospy, mais celui-ci est trop spécialisé forestimator. Je crée une classe mère avec les membres que je souhaite partager entre les deux applis

std::string roundDouble(double d, int precisionVal=1);
std::string loadBDpath();

class cdicoAptBase : public std::enable_shared_from_this<cdicoAptBase>
{
public:
    void closeConnection();
    int openConnection();
    cdicoAptBase(std::string aBDFile);

    std::map<int,std::map<std::string,int>> getFEEApt(std::string aCodeEs);
    std::map<int,int> getZBIOApt(std::string aCodeEs);
    std::map<int,std::map<int,int>> getRisqueTopo(std::string aCodeEs);


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
    int Apt(std::string aCode){
        int aRes(777);
        if (Dico_Apt.find(aCode)!=Dico_Apt.end()){aRes=Dico_Apt.at(aCode);}
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

    std::map<std::string,std::string>  Dico_AptFull2AptAcro;// j'en ai besoin dans les batonnetApt
protected:
    std::string mBDpath;
    sqlite3 **db_;
    sqlite3 * ptDb_;

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
    // les codes aptitudes sont classé dans un ordre fonction de la contrainte, permet de comparer deux aptitude
    std::map<int,int>  Dico_Apt2OrdreContr;
    std::map<int,std::string> Dico_risque;
    std::map<std::string,int> Dico_risque2Code;
    // on regroupe les 5 risques en 3 catégorie
    std::map<int,int> Dico_risqueCategorie;
    std::map<int,std::string>  Dico_topo;
    std::map<int,std::string>  Dico_ZBIO;
    std::map<int,std::string>  dico_groupeNH2Label;// pour l'écogramme avec visu prédiciton random forest
    std::map<int,int>  dico_groupeNH2Nb;//nombre de niveau NH par groupe
    std::map<int,int>  dico_groupeNH2NHStart;// code nh qui débute le groupe.

};

#endif // CDICOAPTBASE_H
