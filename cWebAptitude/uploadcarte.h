#ifndef UPLOADCARTE_H
#define UPLOADCARTE_H
#include "parcellaire.h"
#include "Wt/WResource.h"
#include "Wt/WFileResource.h"
#include "Wt/Http/Response.h"

// sert a préparer le téléchargement par l'utilisateur de cartes, soit en plein, soit croppées avec un shp parcellaire.
// il faut donc que l'utilisateur puisse choisir tout cela (listing via groupLayers ; même genre de tableau que grouplayers avec des checkbox en plus)

class uploadCarte;

class uploadCarte : public WContainerWidget
{
public:
    // structure similaire à parcellaire sauf que pas de nouvelle fenetre à ouvrir
    uploadCarte(WContainerWidget *parent, groupLayers * aGL, parcellaire * aPA, Wt::WApplication* app);

    void uploadShp();
    void uploadRaster();

private:
    Wt::WContainerWidget     * mParent;
    Wt::WPushButton *uploadBtShp, *uploadBtRaster;
    Wt::WCheckBox * mCB_clip;
    Wt::WApplication* m_app;
    Wt::WText * msg;
    groupLayers * mGL;
    parcellaire * mPA;
};

#endif // UPLOADCARTE_H
