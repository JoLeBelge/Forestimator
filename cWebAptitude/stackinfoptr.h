#ifndef STACKINFOPTR_H
#define STACKINFOPTR_H

#include <Wt/WContainerWidget.h>
#include <Wt/WTabWidget.h>
#include <Wt/WStackedWidget.h>

using namespace Wt;

// une classe qui regroupe les différents pointeurs vers les conteneurs Légende et GroupGL
// ainsi que le pointeur vers le stack qui contient ces conteneur et les menuItem de celui-ci
class stackInfoPtr
{
public:
    stackInfoPtr():stack_info(NULL),mSimplepointW(NULL),mGroupLayerW(NULL),menuitem_analyse(NULL),menuitem_cartes(NULL),menuitem_legend(NULL){}

    WStackedWidget * stack_info; // cause que je dois changer de current index après avoir mis à jour la légende que je clique sur une station
    WContainerWidget * mSimplepointW;
    WContainerWidget * mGroupLayerW;
    WMenuItem * menuitem_analyse,* menuitem_cartes,*menuitem_legend,*menuitem_presentation,*menuitem_simplepoint;
};

#endif // STACKINFOPTR_H
