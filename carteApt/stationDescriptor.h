#include <iostream>

using namespace std;

#include "cdicoaptbase.h"
#include "cnsw.h"
#include "cdicoapt.h"

#include "boost/program_options.hpp"
#include "layerbase.h"
#include <string>
#include <random>
#include <omp.h>
#include <iomanip>
namespace po = boost::program_options;
namespace bf =boost::filesystem;


void statDendro(std::string aShp);

extern bool globTest;
void descriptionStation(std::string aShp);

OGRPoint * getCentroid(OGRPolygon * hex);

void echantillonTuiles(std::string aShp);

// découpe de polygone en tuile
void tuilage(std::string aShp);

// renvoie une grille de point, centre de tuile
std::vector<OGRPoint> squarebin(OGRLayer * lay, int rectSize);
// renvoie les tuiles carrées
std::vector<OGRGeometry*> squarebinPol(OGRLayer * lay, int rectSize);

// générer une grille depuis un raster masque, on garde les tuiles qui ont plus de x pct d'occurence de 1 dans le masque
void tuilageFromRaster(rasterFiles * rasterMask, int rectSize, double prop, double seuilRasterIn);

// stat sur un raster scolyte et ajout d'un champ dans shp tuile
void anaScolyteOnShp(rasterFiles * raster, std::string aShp);
