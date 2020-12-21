#include "uploadcarte.h"

uploadCarte::uploadCarte(WContainerWidget *parent, groupLayers * aGL, parcellaire * aPA, Wt::WApplication* app):mParent(parent),mGL(aGL),m_app(app),mPA(aPA)
{
    mParent->setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Left);
    mParent->setMargin(20,Wt::Side::Bottom | Wt::Side::Top);
    mParent->setInline(0);

    mParent->addWidget(cpp14::make_unique<Wt::WText>(tr("infoTelechargement")));

    mParent->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());
    //mParent->addWidget(std::unique_ptr<Wt::WContainerWidget>(mGL->afficheSelect4Stat()));
    mParent->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());

    mParent->addWidget(cpp14::make_unique<WText>("<h4>Télécharger le parcellaire</h4>"));
    mParent->addWidget(cpp14::make_unique<Wt::WText>(tr("infoTelechargementShp")));
    mParent->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());
    uploadBtShp = mParent->addWidget(cpp14::make_unique<Wt::WPushButton>("Télécharger"));
    uploadBtShp->setInline(0);
    mParent->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());
    mParent->addWidget(cpp14::make_unique<WText>("<h4>Télécharger les cartes raster</h4>"));
    mParent->addWidget(cpp14::make_unique<Wt::WText>(tr("infoTelechargementRaster")));
    mParent->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());
    mCB_clip= mParent->addWidget(Wt::cpp14::make_unique<Wt::WCheckBox>(tr("cb_decoupeShp")));
    mCB_clip->setInline(0);
    // il faut lier cette checkbox avec signaux en provenance de parcellaire. Si shp valide ; checkbox true
    uploadBtRaster = mParent->addWidget(cpp14::make_unique<Wt::WPushButton>("Télécharger"));
    uploadBtRaster->setInline(0);

    uploadBtShp->clicked().connect(this ,&uploadCarte::uploadShp);


    uploadBtRaster->clicked().connect(this ,&uploadCarte::uploadRaster);
}


void uploadCarte::uploadShp(){
    // 1 contrôler que les stat sont bien calculées

    // 2 ajouter stat au shp

    // 3 compress et upload

}
void uploadCarte::uploadRaster(){
    // la liste des raster
    std::vector<rasterFiles> r=mGL->getSelect4Download();
    rasterFiles raster=r.at(0);

    std::cout << "fichier  "<<raster.getPathTif()<< std::endl;
     WFileResource *fileResource = new Wt::WFileResource("plain/text", raster.getPathTif());
     //std::unique_ptr<WFileResource> fileResource = std::make_unique<Wt::WFileResource>("plain/text", raster.getPathTif());
    fileResource->suggestFileName("tata.tif");
    m_app->redirect(fileResource->url());
}
