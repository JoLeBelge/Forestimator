#include "presentationpage.h"

int globMaxDownloadFileS(1000);

presentationPage::presentationPage(cDicoApt *aDico, cWebAptitude *app):mDico(aDico),m_app(app)
{

    // création d'un menu à gauche, co dans wt widget gallery
    Wt::WHBoxLayout * hLayout = setLayout(std::make_unique<Wt::WHBoxLayout>());
    
    hLayout->setContentsMargins(0, 0, 0, 0);
    setContentAlignment(Wt::AlignmentFlag::Top);

    std::unique_ptr<Wt::WStackedWidget> subStack = std::make_unique<Wt::WStackedWidget>();
    subStack->addStyleClass("contents");
    subStack->setContentAlignment(AlignmentFlag::Left);
    subStack->setOverflow(Wt::Overflow::Auto);

    auto subMenu = std::make_unique<Wt::WMenu>(subStack.get());
    auto subMenu_ = subMenu.get();
    subMenu_->addStyleClass("nav-pills nav-stacked submenu submenuPresentation");
    subMenu_->setWidth(200);

    subMenu_->setInternalPathEnabled("/documentation");

    // introduction forestimator
    std::unique_ptr<Wt::WMenuItem> item = std::make_unique<Wt::WMenuItem>("Forestimator : présentation");
    Wt::WContainerWidget * c0 = new Wt::WContainerWidget();
    c0->addNew<WText>(WString::tr("ref.article.forestimator"));
    c0->addNew<WText>(WString::tr("page_presentation"));
    item->setContents(std::unique_ptr<Wt::WContainerWidget>(c0));
    subMenu_->addItem(std::move(item));

    std::unique_ptr<Wt::WMenuItem> item2 = std::make_unique<Wt::WMenuItem>("Crédit et contact", std::make_unique<Wt::WText>(WString::tr("page_presentation.credit")));
    subMenu_->addItem(std::move(item2));

    std::unique_ptr<Wt::WMenuItem> item3 = std::make_unique<Wt::WMenuItem>("Téléchargement");
    Wt::WContainerWidget * c = new Wt::WContainerWidget();
    c->addNew<WText>(WString::tr("intro_telechargement"));
    Wt::WTable * t = c->addNew<Wt::WTable>();
    t->setHeaderCount(1);
    t->setWidth(Wt::WLength("90%"));
    t->toggleStyleClass("table-striped",true);
    t->elementAt(0, 0)->setColumnSpan(4);
    t->elementAt(0, 0)->setContentAlignment(AlignmentFlag::Top | AlignmentFlag::Center);
    t->elementAt(0, 0)->setPadding(10);
    t->elementAt(0, 0)->addWidget(std::make_unique<WText>(tr("titre.tab.download")));

    // on les présente par groupe de couches
    for (std::string gr : mDico->Dico_groupe){
        // test si au moins une couche pour le groupe
        bool test(0);
        for (std::shared_ptr<layerBase> l : mDico->VlayersForGroupe(gr)){
            if (l->getCatLayer()!=TypeLayer::Externe & !l->Expert() & mDico->lay4Visu(l->Code())){
                test=1;break;
            }
        }
        if (test){

            int r=t->rowCount();
            t->elementAt(r, 0)->setColumnSpan(4);
            t->elementAt(r, 0)->addWidget(std::make_unique<WText>(WString::fromUTF8("<h4>"+mDico->groupeLabel(gr)+"</h4>")));
            t->elementAt(r, 0)->addStyleClass("bold");
            t->elementAt(r+1, 1)->addWidget(std::make_unique<WText>(tr("colWMS.tab.download")));
            t->elementAt(r+1, 2)->addWidget(std::make_unique<WText>(tr("colWMSname.tab.download")));
            t->elementAt(r+1, 3)->addWidget(std::make_unique<WText>(tr("colSize.tab.download")));

            for (std::shared_ptr<layerBase> l : mDico->VlayersForGroupe(gr)){
                if (l->getCatLayer()!=TypeLayer::Externe & !l->Expert() & mDico->lay4Visu(l->Code()) & l->getFilesize()<globMaxDownloadFileS){
                    int row=t->rowCount();
                    t->elementAt(row, 0)->addWidget(std::make_unique<WText>(WString::fromUTF8(l->Nom())));
                    WText * url =t->elementAt(row, 1)->addWidget(std::make_unique<WText>(WString::fromUTF8(l->WMSURL())));
                    url->addStyleClass("mya");
                    t->elementAt(row, 2)->addWidget(std::make_unique<WText>(WString::fromUTF8(l->WMSLayerName())));
                    t->elementAt(row, 3)->addWidget(std::make_unique<WText>(WString::fromUTF8(roundDouble(l->getFilesize(),1)+ " Mo")));

                    Wt::WPushButton * b = t->elementAt(row, 4)->addWidget(std::make_unique<Wt::WPushButton>("télécharger"));
                    t->elementAt(row, 4)->setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Middle);
                    Wt::WLink loadLink = Wt::WLink("/telechargement/"+l->Code());
                    b->clicked().connect([=]{m_app->addLog(l->Code(),typeLog::dsingleRW);
                        // si la couche est un raster de valeur continue avec gain et offset, prévenir l'utilisateur avec une boite de dialogue
                        if (l->getTypeVar()==TypeVar::Continu && l->Gain()!=1.0){
                            Wt::WMessageBox * messageBox = this->addChild(std::make_unique<Wt::WMessageBox>(
                                                                              "Attention",
                                                                             tr("msg.Gain.info").arg(l->Gain()),
                                                                              Wt::Icon::Information,
                                                                              Wt::StandardButton::Ok));
                            messageBox->setModal(true);
                            messageBox->buttonClicked().connect([=] {
                                this->removeChild(messageBox);
                            });
                            messageBox->show();

                        }
                    });
                    b->setLink(loadLink);// le lien pointe vers une ressource qui est générée dans main.cpp
                    // qml
                    if (l->hasSymbology()){
                        Wt::WPushButton * b2 = t->elementAt(row, 5)->addWidget(std::make_unique<Wt::WPushButton>("télécharger le qml"));
                        t->elementAt(row, 5)->setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Middle);
                        Wt::WLink loadLink2 = Wt::WLink("/telechargement/"+l->Code()+"qml");
                        b2->setLink(loadLink2);
                    }
                }
            }
        }
    }
    item3->setContents(std::unique_ptr<Wt::WContainerWidget>(c));

    subMenu_->addItem(std::move(item3));

    for( auto kv : *mDico->layerMTD()){
        LayerMTD lMTD=kv.second;
        if (lMTD.code()!="ES_EP"){
        std::unique_ptr<Wt::WMenuItem> item = std::make_unique<Wt::WMenuItem>(lMTD.Label(), cpp14::make_unique<Wt::WText>(getHtml(&lMTD)));


        subMenu_->addItem(std::move(item));
        } else {
            std::unique_ptr<Wt::WMenuItem> mi = std::make_unique<Wt::WMenuItem>(lMTD.Label());
            Wt::WContainerWidget * ac = new Wt::WContainerWidget();
            ac->addNew<WText>(getHtml(&lMTD));
            // ajout d'un média video via librairie wt

            ac->addNew<WText>(tr("ES_EP.video"));
            std::string mp4Video = "video/Argonne-illuCriseSco.mp4";
            //std::string ogvVideo = "https://www.webtoolkit.eu/videos/sintel_trailer.ogv";
            // Define poster image location
            std::string poster = "img/scoMM.png";
            Wt::WVideo * video = ac->addNew<Wt::WVideo>();
            video->addSource(Wt::WLink(mp4Video),"");
            video->setPoster(poster);
            video->setAlternativeContent(std::make_unique<Wt::WImage>(Wt::WLink(poster)));
            video->resize(640, 360);
            mi->setContents(std::unique_ptr<Wt::WContainerWidget>(ac));
            subMenu_->addItem(std::move(mi));
        }
    }

    std::unique_ptr<Wt::WMenuItem> item4 = std::make_unique<Wt::WMenuItem>("Forestimator API", std::make_unique<Wt::WText>(WString::tr("docu.api")));
    subMenu_->addItem(std::move(item4));


    std::unique_ptr<Wt::WMenuItem> item5 = std::make_unique<Wt::WMenuItem>("Guide des Stations");
    item5->setContents(std::make_unique<matAptCS>(mDico));
    item5->contents()->setMaximumSize("100%","5000px");
    subMenu_->addItem(std::move(item5));

    std::unique_ptr<Wt::WMenuItem> item6 = std::make_unique<Wt::WMenuItem>("Confidentialité");
    Wt::WContainerWidget * c6 = new Wt::WContainerWidget();
    c6->addNew<WText>(Wt::WString::tr("confidentialite"));
    item6->setContents(std::unique_ptr<Wt::WContainerWidget>(c6));
    subMenu_->addItem(std::move(item6));

    hLayout->addWidget(std::move(subMenu));
    hLayout->addWidget(std::move(subStack),1);

}
