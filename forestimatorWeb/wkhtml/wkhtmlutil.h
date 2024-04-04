#ifndef WKHTMLUTIL_H
#define WKHTMLUTIL_H

#include "wkhtmltox/image.h"
#include <string>
#include <iostream>
#include <fstream>
int wkhtml(std::string html);
void error(wkhtmltoimage_converter * c, const char * msg);

#endif // WKHTMLUTIL_H
