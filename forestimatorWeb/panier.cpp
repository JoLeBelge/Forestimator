#include "panier.h"

extern bool globTest;
panier::panier(cWebAptitude * cWebApt): WContainerWidget() ,
    mDico(cWebApt->mDico),m_app(cWebApt),mMap(cWebApt->mMap),mGroupL(cWebApt->mGroupL)
{
    this->setMaximumSize(500,700);

    // create table et layer nodes
    mTable = this->addWidget(std::make_unique<WTable>());
    mTable->setHeaderCount(0);
    mTable->addStyleClass("panier_table");
    //mTable->setWidth(Wt::WLength("90%"));
    mTable->toggleStyleClass("table-striped",true);

    // extent div si user connecté
    this->addWidget(std::make_unique<Wt::WBreak>());
    Wt::WContainerWidget * mExtentDivGlob = this->addWidget(std::make_unique<WContainerWidget>());
    Wt::WContainerWidget * mExtentDiv;
    WPushButton * button_e = mExtentDivGlob->addWidget(std::make_unique<WPushButton>(tr("afficher_extent")));
    button_e->setToolTip(tr("afficher_extent_tooltip"));
    button_e->clicked().connect([=] {
        if(mGroupL->mExtentDiv->isVisible())
            mGroupL->mExtentDiv->hide();
        else
            mGroupL->mExtentDiv->show();
    });
    button_e->addStyleClass("btn btn-info");
    if (globTest){printf("mextentdiv\n");}
    mExtentDiv = mExtentDivGlob->addWidget(std::make_unique<WContainerWidget>());
    mExtentDiv->setMargin(15,Wt::Side::Left);
    mExtentDiv->setMargin(15,Wt::Side::Right);
    mExtentDiv->addStyleClass("div_extent");
    mExtentDiv->hide();

    this->addWidget(std::make_unique<Wt::WBreak>());
    this->addWidget(std::make_unique<WText>(tr("coucheStep3")));
    WPushButton * bExportTiff = this->addWidget(std::make_unique<WPushButton>("Télécharger"));
    bExportTiff->setToolTip(tr("panier.download_tooltip"));
    bExportTiff->clicked().connect(mGroupL->slotMapExport);
    //bExportTiff->clicked().connect(this,&groupLayers::updateMapExtentAndCropIm);

    m_app->mGroupL->mExtentDivGlob=mExtentDivGlob;
    m_app->mGroupL->mExtentDiv=mExtentDiv;
}


void panier::addMap(std::string aCode, std::shared_ptr<Layer> l){
    if (globTest) {std::cout << "aCode : " << aCode << std::endl;}

    // vérifie qu'elle n'est pas déjà dans le panier
    for (std::shared_ptr<Layer> l : mVLs){
        if (l->Code()==aCode){
            Wt::WMessageBox * messageBox = this->addChild(std::make_unique<Wt::WMessageBox>("Sélection d'une carte","<p>Cette couche est déjà dans votre sélection</p>",Wt::Icon::Critical,Wt::StandardButton::Ok));
            messageBox->setModal(true);
            messageBox->buttonClicked().connect([=] {
                this->removeChild(messageBox);
            });
            messageBox->show();
            return;
        }
    }

    // je la met au début du vecteur
    mVLs.insert(mVLs.begin(),l);
    // je met la nouvelle couche en haut du tableau
    Wt::WTableRow * r =mTable->insertRow(0);
    r->elementAt(0)->setContentAlignment(AlignmentFlag::Top | AlignmentFlag::Left);
    r->elementAt(0)->setPadding(5);
    r->elementAt(0)->addWidget(std::make_unique<WText>(l->Nom()));
            /* bouton visible/invisible */
    WPushButton * bvis = r->elementAt(1)->addWidget(std::make_unique<WPushButton>(""));
    bvis->addStyleClass("button_carto");
    bvis->setIcon("resources/eye_visible.png");
    bvis->setCheckable(true);
    bvis->setChecked(true);
    bvis->clicked().connect([=] {
        if(bvis->isChecked())
            bvis->setIcon("resources/eye_visible.png");
        else
            bvis->setIcon("resources/eye_notvisible.png");
        m_app->doJavaScript("activeLayers['"+aCode+"']?.setVisible(!activeLayers['"+aCode+"']?.values_.visible);");
    });
    /* bouton transparent/opaque */
    bvis = r->elementAt(2)->addWidget(std::make_unique<WPushButton>("T"));
    bvis->addStyleClass("button_carto");
    bvis->setToolTip(tr("panier.transparent"));
    bvis->setCheckable(true);
    bvis->setChecked(true);
    bvis->clicked().connect([=] {
        if(bvis->isChecked())
            bvis->setText("T");
        else
            bvis->setText("O");
        m_app->doJavaScript("activeLayers['"+aCode+"']?.setOpacity(activeLayers['"+aCode+"']?.getOpacity()==1?0.5:1);");
    });

    int row=mTable->rowCount();

    if (row==0){
        bvis->setChecked(false);
        bvis->setText("O");
    }
    /* bouton delete la couche */
    bvis =r->elementAt(3)->addWidget(std::make_unique<WPushButton>("x"));
    bvis->addStyleClass("button_carto");
    bvis->setToolTip(tr("panier.delete"));
    bvis->clicked().connect([=] {
        Wt::StandardButton answer = Wt::WMessageBox::show("Confirmer","<p>Enlever cette couche de votre sélection ?</p>",Wt::StandardButton::Yes | Wt::StandardButton::No | Wt::StandardButton::Cancel);
        if (answer == Wt::StandardButton::Yes){
            if(mVLs.size()>1){
                // del in vector
                size_t i;
                for (i = 0; i < mVLs.size(); i++){

                    if(mVLs.at(i) == l){
                        mVLs.erase(mVLs.begin() + i);
                        break;
                    }
                }
                // del row in table
                mTable->removeRow(i);
                // del layer
                m_app->doJavaScript("map.removeLayer(activeLayers['"+aCode+"']);delete activeLayers['"+aCode+"'];");
                mGroupL->updateLegendeDiv(mVLs);
                mGroupL->updateActiveLay(mVLs.at(0)->Code());
            } else {
                Wt::WMessageBox * messageBox = this->addChild(std::make_unique<Wt::WMessageBox>("Retirer une carte","<p>Il ne reste que cette couche dans votre sélection</p>",Wt::Icon::Critical,Wt::StandardButton::Ok));
                messageBox->setModal(true);
                messageBox->buttonClicked().connect([=] {
                    this->removeChild(messageBox);
                });
                messageBox->show();
            }
        }
    });
    /* boutons deplacer la couche */
    bvis = r->elementAt(4)->addWidget(std::make_unique<WPushButton>(""));
    bvis->addStyleClass("button_carto movedown");
    bvis->setToolTip(tr("panier.movedown"));
    bvis->clicked().connect([=] {
        if (mVLs.size()==1) return; // skipt 1 element
        size_t i;
        for (i = 0; i<mVLs.size(); i++){
            if(mVLs.at(i)==l){break;}
        }
        if (i == mVLs.size()-1) return; // skipt last element
        // move in vector
        iter_swap(mVLs.begin() + i, mVLs.begin() + i + 1);
        // move row in table
        mTable->moveRow(i,i+1);
        // move layer
        m_app->doJavaScript("moveLayerUp('"+aCode+"');");
        mGroupL->updateActiveLay(mVLs.at(0)->Code());
    });
    bvis = r->elementAt(5)->addWidget(std::make_unique<WPushButton>(""));
    bvis->addStyleClass("button_carto moveup");
    bvis->setToolTip(tr("panier.moveup"));
    bvis->clicked().connect([=] {
        if (mVLs.size()==1) return; // skipt 1 element
        int i=0;
        for (i; i<mVLs.size(); i++){
            //std::cout << "i" << i << ", lay = " << l << std::endl;
            if(mVLs.at(i)==l){break;}
        }
        if (i==0) return; // skipt first element
        // move in vector
        iter_swap(mVLs.begin() + i, mVLs.begin() + i - 1);
        // move row in table
        mTable->moveRow(i,i-1);
        // move layer
         m_app->doJavaScript("moveLayerDown('"+aCode+"');");
         mGroupL->updateActiveLay(mVLs.at(0)->Code());
    });
}

