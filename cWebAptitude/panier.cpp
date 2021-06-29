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

    /*
    WPushButton * bvis;

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
    bvis->setToolTip(tr("panier.transparent"));
    bvis->setCheckable(true);
    bvis->setChecked(false);
    bvis->clicked().connect([=] {
        if(bvis->isChecked())
            bvis->setText("T");
        else
            bvis->setText("O");
        mcWebAptitude->doJavaScript("IGNLayer.setOpacity(IGNLayer.getOpacity()==1?0.5:1);");
    });
    */


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


void panier::addMap(std::string aCode, std::shared_ptr<Layer> l){
    std::cout << "aCode : " << aCode << std::endl;
    // vérifie qu'elle n'est pas déjà dans le panier
    bool test(1);
    for (std::shared_ptr<Layer> l : mVLs){
        if (l->Code()==aCode){
            Wt::WMessageBox * messageBox = this->addChild(Wt::cpp14::make_unique<Wt::WMessageBox>("Sélection d'une carte","<p>Cette couche est déjà dans votre sélection</p>",Wt::Icon::Critical,Wt::StandardButton::Ok));
            messageBox->setModal(true);
            messageBox->buttonClicked().connect([=] {
                this->removeChild(messageBox);
            });
            messageBox->show();
            test=0;
            break;
        }
    }

    if (test){
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



        // la première couche n'est pas en transparence. fonctionne pas, je sais pas pk
        /*if (row==0){
            this->doJavaScript("activeLayers['"+aCode+"'].setOpacity(1);");
            bvis->setChecked(false);
            bvis->setText("O");
        }*/



        bvis = mTable->elementAt(row, 3)->addWidget(cpp14::make_unique<WPushButton>("x"));
        bvis->addStyleClass("button_carto");
        bvis->setToolTip(tr("panier.delete"));
        bvis->clicked().connect([=] {
            Wt::StandardButton answer = Wt::WMessageBox::show("Confirmer","<p>Enlever cette couche de votre sélection ?</p>",Wt::StandardButton::Yes | Wt::StandardButton::No | Wt::StandardButton::Cancel);
            if (answer == Wt::StandardButton::Yes){

                if(mVLs.size()>1){
                // del in vector

                int i=0;
                for (i; i<mVLs.size(); i++){

                    if(mVLs.at(i)==l){
                        mVLs.erase(mVLs.begin()+i);
                        break;
                    }
                }


                // mTable->removeRow(row); --> cause bug car copie la valeur de row qui en fait n'est pas constante car si je supprime des couche du panier en amont avant de supprimer celle-ci, row  n'as pas la bonne valeur.

                mTable->removeRow(i); // attention dangeureux le +1 car 1 couche de base IGN
                // del layer
                mcWebAptitude->doJavaScript("map.removeLayer(activeLayers['"+aCode+"']);delete activeLayers['"+aCode+"'];");
                } else {
                    Wt::WMessageBox * messageBox = this->addChild(Wt::cpp14::make_unique<Wt::WMessageBox>("Retirer une carte","<p>Il ne reste que cette couche dans votre sélection</p>",Wt::Icon::Critical,Wt::StandardButton::Ok));
                    messageBox->setModal(true);
                    messageBox->buttonClicked().connect([=] {
                        this->removeChild(messageBox);
                    });
                    messageBox->show();
                }
            }
        });
    }

}

