#ifndef MAIN_H
#define MAIN_H

#include "auth.h"
#include "Session.h"
#include <Wt/WApplication.h>
#include <Wt/WEnvironment.h>
#include "boost/program_options.hpp"
namespace po = boost::program_options;
#include <Wt/WServer.h>
#include <Wt/Auth/AuthWidget.h>
#include <Wt/Auth/PasswordService.h>
#include "api/stationdescresource.h"
#include "analytics.h"

std::unique_ptr<Wt::WApplication> createAuthApplication(const Wt::WEnvironment &env, cDicoApt * dico);

int main(int argc, char **argv);

#endif // MAIN_H
