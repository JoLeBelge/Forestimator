#ifndef MANIPRESSOURCE_H
#define MANIPRESSOURCE_H
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <memory>
#include <sqlite3.h>
#include <algorithm>
#include "boost/program_options.hpp"
namespace po = boost::program_options;
#include "cdicoapt.h"
#include "layerbase.h"
//#include "api/cnswresource.h"
#include "./libzippp/src/libzippp.h"
#include "../stationDescriptor/rapidxml/rapidxml.hpp"
#include <curl/curl.h>

#include "cphyto.h"

#include <Wt/WApplication.h>
#include <Wt/WEnvironment.h>

using namespace libzippp;
using namespace rapidxml;

void deepl(cDicoApt *dico);
void dicoToXml(std::shared_ptr<cDicoPhyto> dico);
std::string traduction(std::string afr, std::string target_lang="EN");
void processNCBI(std::shared_ptr<cDicoPhyto> dico);


// pour manipuler les xml avec les ressources des 2 sites web, un soft dédié

int main(int argc, char **argv);
#endif // MANIPRESSOURCE_H
