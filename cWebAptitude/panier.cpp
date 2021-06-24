#include "panier.h"


panier::panier(AuthApplication *app, cWebAptitude * cWebApt): WContainerWidget() ,
    mDico(app->mDico),m_app(app),mcWebAptitude(cWebApt),mMap(cWebApt->mMap),mGroupL(cWebApt->mGroupL)
{
    this->setMaximumSize(500,700);


    // create table et layer nodes
    mTable = this->addWidget(cpp14::make_unique<WTable>());
    mTable->setHeaderCount(0);
    mTable->setWidth(Wt::WLength("90%"));
    mTable->toggleStyleClass("table-striped",true);

    // couche de base Ortho
    mTable->elementAt(0, 0)->setContentAlignment(AlignmentFlag::Top | AlignmentFlag::Left);
    mTable->elementAt(0, 0)->setPadding(5);
    mTable->elementAt(0, 0)->addWidget(cpp14::make_unique<WText>("Carte aérienne 2020"));
    bOrtho = mTable->elementAt(0, 1)->addWidget(cpp14::make_unique<WPushButton>(""));
    bOrtho->setIcon("resources/eye_notvisible.png");
    bOrtho->addStyleClass("button_carto");
    bOrtho->setCheckable(true);
    bOrtho->clicked().connect([=] {
        if(bOrtho->isChecked())
            bOrtho->setIcon("resources/eye_visible.png");
        else
            bOrtho->setIcon("resources/eye_notvisible.png");
        cWebApt->doJavaScript("orthoLayer.setVisible(!orthoLayer.values_.visible);");
    });
    mTable->elementAt(0, 2)->setColumnSpan(2);
    WPushButton * bvis = mTable->elementAt(0, 2)->addWidget(cpp14::make_unique<WPushButton>("T"));
    bvis->addStyleClass("button_carto");
    bvis->setToolTip("panier.transparent");
    bvis->setCheckable(true);
    bvis->setChecked(false);
    bvis->clicked().connect([=] {
        if(bvis->isChecked())
            bvis->setText("T");
        else
            bvis->setText("O");
        mcWebAptitude->doJavaScript("orthoLayer.setOpacity(orthoLayer.getOpacity()==1?0.5:1);");
    });

    // couche de base IGN
    mTable->elementAt(1, 0)->setContentAlignment(AlignmentFlag::Top | AlignmentFlag::Left);
    mTable->elementAt(1, 0)->setPadding(5);
    mTable->elementAt(1, 0)->addWidget(cpp14::make_unique<WText>("Carte IGN"));
    bIGN = mTable->elementAt(1, 1)->addWidget(cpp14::make_unique<WPushButton>(""));
    bIGN->setIcon("resources/eye_visible.png");
    bIGN->addStyleClass("button_carto");
    bIGN->setCheckable(true);
    bIGN->setChecked(true);
    bIGN->clicked().connect([=] {
        if(bIGN->isChecked())
            bIGN->setIcon("resources/eye_visible.png");
        else
            bIGN->setIcon("resources/eye_notvisible.png");
        cWebApt->doJavaScript("IGNLayer.setVisible(!IGNLayer.values_.visible);");
    });
    mTable->elementAt(1, 2)->setColumnSpan(2);
    bvis = mTable->elementAt(1, 2)->addWidget(cpp14::make_unique<WPushButton>("T"));
    bvis->addStyleClass("button_carto");
    bvis->setToolTip("panier.transparent");
    bvis->setCheckable(true);
    bvis->setChecked(false);
    bvis->clicked().connect([=] {
        if(bvis->isChecked())
            bvis->setText("T");
        else
            bvis->setText("O");
        mcWebAptitude->doJavaScript("IGNLayer.setOpacity(IGNLayer.getOpacity()==1?0.5:1);");
    });


    // extent div si user connecté
    this->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());
    Wt::WContainerWidget * mExtentDivGlob = this->addWidget(cpp14::make_unique<WContainerWidget>());
    Wt::WContainerWidget * mExtentDiv;
    WPushButton * button_e = mExtentDivGlob->addWidget(cpp14::make_unique<WPushButton>(tr("afficher_extent")));
    button_e->setToolTip(tr("afficher_extent_tooltip"));
    button_e->clicked().connect([=] {
        if(mGroupL->mExtentDiv->isVisible())
            mGroupL->mExtentDiv->hide();
        else
            mGroupL->mExtentDiv->show();
    });
    button_e->addStyleClass("btn btn-info");
    printf("mextentdiv\n");
    mExtentDiv = mExtentDivGlob->addWidget(cpp14::make_unique<WContainerWidget>());
    mExtentDiv->setMargin(15,Wt::Side::Left);
    mExtentDiv->setMargin(15,Wt::Side::Right);
    mExtentDiv->addStyleClass("div_extent");
    mExtentDiv->hide();

    this->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());
    this->addWidget(cpp14::make_unique<WText>(tr("coucheStep3")));
    WPushButton * bExportTiff = this->addWidget(cpp14::make_unique<WPushButton>("Télécharger"));

    bExportTiff->clicked().connect(mGroupL->slotMapExport);
    //bExportTiff->clicked().connect(this,&groupLayers::updateMapExtentAndCropIm);

    mcWebAptitude->mGroupL->mExtentDivGlob=mExtentDivGlob;
    mcWebAptitude->mGroupL->mExtentDiv=mExtentDiv;



}


void panier::addMap(std::string aCode, TypeLayer type, std::shared_ptr<Layer> l){
    std::cout << "aCode : " << aCode << std::endl;
    mVLs.push_back(l);
    int row=mTable->rowCount();
    mTable->elementAt(row, 0)->setContentAlignment(AlignmentFlag::Top | AlignmentFlag::Left);
    mTable->elementAt(row, 0)->setPadding(5);
    mTable->elementAt(row, 0)->addWidget(cpp14::make_unique<WText>(l->Nom()));
    WPushButton * bvis = mTable->elementAt(row, 1)->addWidget(cpp14::make_unique<WPushButton>(""));
    bvis->addStyleClass("button_carto");
    bvis->setIcon("resources/eye_visible.png");
    bvis->setCheckable(true);
    bvis->setChecked(true);
    bvis->clicked().connect([=] {
        if(bvis->isChecked())
            bvis->setIcon("resources/eye_visible.png");
        else
            bvis->setIcon("resources/eye_notvisible.png");
        mcWebAptitude->doJavaScript("activeLayers['"+aCode+"'].setVisible(!activeLayers['"+aCode+"'].values_.visible);");
    });

    bvis = mTable->elementAt(row, 2)->addWidget(cpp14::make_unique<WPushButton>("T"));
    bvis->addStyleClass("button_carto");
    bvis->setToolTip(tr("panier.transparent"));
    bvis->setCheckable(true);
    bvis->setChecked(true);
    bvis->clicked().connect([=] {
        if(bvis->isChecked())
            bvis->setText("T");
        else
            bvis->setText("O");
        mcWebAptitude->doJavaScript("activeLayers['"+aCode+"'].setOpacity(activeLayers['"+aCode+"'].getOpacity()==1?0.5:1);");
    });

    bvis = mTable->elementAt(row, 3)->addWidget(cpp14::make_unique<WPushButton>("x"));
    bvis->addStyleClass("button_carto");
    bvis->setToolTip(tr("panier.delete"));
    bvis->clicked().connect([=] {
        Wt::StandardButton answer = Wt::WMessageBox::show("Confirmer","<p>Enlever cette couche de votre sélection ?</p>",Wt::StandardButton::Yes | Wt::StandardButton::No | Wt::StandardButton::Cancel);
        if (answer == Wt::StandardButton::Yes){
            // del in vector
            for (int i=0; i<mVLs.size(); i++){
                if(mVLs.at(i)==l){
                    mVLs.erase(mVLs.begin()+i);
                    break;
                }
            }
            // del table row
            mTable->removeRow(row);
            // del layer
            mcWebAptitude->doJavaScript("map.removeLayer(activeLayers['"+aCode+"']);delete activeLayers['"+aCode+"'];");
        }
    });

}

