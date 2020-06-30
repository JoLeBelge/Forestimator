#ifndef CDICOCARTEPH_H
#define CDICOCARTEPH_H
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

class cDicoCartepH
{
public:
    cDicoCartepH(std::string aBDFile);
    void closeConnection();

    double getpH(int aPTS,int aZBIO);
    double getpH(int aPTS){
        double aRes(1.0);
        if (Dico_PTS2pH.find(aPTS)!=Dico_PTS2pH.end()){aRes=Dico_PTS2pH.at(aPTS);}
        return aRes;
    }
    std::string File(std::string aCode){
        std::string aRes("");
        if (Dico_GISfile.find(aCode)!=Dico_GISfile.end()){aRes=Dico_GISfile.at(aCode);}
        return aRes;
    }
    double toDouble(std::string aStr);

    int getPTS(int aSolIndex){
        int aRes(0);
        if (Dico_IndexSiglePed2PTS.find(aSolIndex)!=Dico_IndexSiglePed2PTS.end()){aRes=Dico_IndexSiglePed2PTS.at(aSolIndex);}
        return aRes;
    }

private:
    std::string mBDpath;
    sqlite3 *db_;

    std::map<std::string,int> Dico_NomRN2Code;
    std::map<int,std::string> Dico_codeRN2Nom;
    std::map<int,std::string> Dico_PTS;
    std::map<int,int> Dico_Zbio2RN;
    std::map<int,int> Dico_IndexSiglePed2PTS;
    std::map<int,double> Dico_PTS2pH;
    std::map<std::vector<int>,double> Dico_PTSetRN2pH;
    std::map<std::string,std::string>  Dico_GISfile;
};



#endif // CDICOCARTEPH_H
