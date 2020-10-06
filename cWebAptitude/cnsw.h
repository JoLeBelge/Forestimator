#ifndef CNSW_H
#define CNSW_H

/* une classe qui contient les chemins d'accès vers le shp de la carte numérique des sols de wallonie, des dictionnaires pour les sigles et les champs
, des méthodes pour les analyse ponctuelles et pour les analyses surfaciques
*/

#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <boost/algorithm/string/replace.hpp>
#include <boost/range/adaptor/map.hpp>
#include "boost/filesystem.hpp"
#include <unistd.h>
#include <cmath>

class cnsw;
// toutes les map des dictionnaires pédo et les accesseurs
class dicoPedo;

enum class PEDO { DRAINAGE,
                  TEXTURE,
                  PROFONDEUR
                     };

class cnsw
{
public:
    cnsw();
    std::vector<std::string> displayInfo(double x, double y,PEDO p);

    // raster value
    std::string getValue(double x, double y, PEDO p);

};

//
class dicoPedo
{

public:
    dicoPedo();
private:
    boost::filesystem::path mShpPath;


};

#endif // CNSW_H
