#include "presentationpage.h"

//int nbAds(5); // Ads = advertising
int globMaxDownloadFileS(1000);
// creation d'une bannières de pub (ads banner) pour faire défiler des informations descriptives du sites
presentationPage::presentationPage(cDicoApt *aDico, AuthApplication *app):mDico(aDico),m_app(app)
{
    /* OLD mettre la bannière dans un stacked widget, je pense que c'est pas la super solution..
    adsBanner = addNew<WStackedWidget>();
    // chargement des contenus de la bannière
    for (int i(1); i<nbAds+1;i++){
        Wt::WContainerWidget * cont =adsBanner->addNew<WContainerWidget>();
        //cont->addStyleClass("carto_menu"); non car contient l'image de fond
        cont->addNew<Wt::WText>(Wt::WText::tr("presentation-ad"+std::to_string(i)));
    }
    // j'ai testé toutes les solutions trouvées sur le net, asio de boost, plein de timer (souvent bloquant), finalement le Wtimer est juste super
   Wt::WTimer * timer = this->addChild(cpp14::make_unique<Wt::WTimer>());
   timer->setInterval(std::chrono::seconds(7));
   timer->timeout().connect(this, &presentationPage::bannerAnimation);
   timer->start();*/


    // création d'un menu à gauche, co dans wt widget gallery
    Wt::WHBoxLayout * hLayout = setLayout(std::make_unique<Wt::WHBoxLayout>());
    hLayout->setContentsMargins(0, 0, 0, 0);

    std::unique_ptr<Wt::WStackedWidget> subStack = std::make_unique<Wt::WStackedWidget>();
    subStack->addStyleClass("contents");
    subStack->setContentAlignment(AlignmentFlag::Left);
    subStack->setOverflow(Wt::Overflow::Auto);

    auto subMenu = std::make_unique<Wt::WMenu>(subStack.get());
    auto subMenu_ = subMenu.get();
    subMenu_->addStyleClass("nav-pills nav-stacked submenu submenuPresentation");
    subMenu_->setWidth(200);

    subMenu_->setInternalPathEnabled("/documentation");

    // probleme https://redmine.webtoolkit.eu/boards/2/topics/1206, j'ai plein de session qui se lancent quand je veux accèder à l'internal path d'une documentation

    // introduction forestimator
    std::unique_ptr<Wt::WMenuItem> item = std::make_unique<Wt::WMenuItem>("Forestimator : présentation", cpp14::make_unique<Wt::WText>(WString::tr("page_presentation")));
    subMenu_->addItem(std::move(item));
    std::unique_ptr<Wt::WMenuItem> item2 = std::make_unique<Wt::WMenuItem>("Crédit et contact", cpp14::make_unique<Wt::WText>(WString::tr("page_presentation.credit")));
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
    t->elementAt(0, 0)->addWidget(cpp14::make_unique<WText>(tr("titre.tab.download")));

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
            t->elementAt(r, 0)->addWidget(cpp14::make_unique<WText>(WString::fromUTF8("<h4>"+mDico->groupeLabel(gr)+"</h4>")));
            t->elementAt(r, 0)->addStyleClass("bold");
            t->elementAt(r+1, 1)->addWidget(cpp14::make_unique<WText>(tr("colWMS.tab.download")));
            t->elementAt(r+1, 2)->addWidget(cpp14::make_unique<WText>(tr("colWMSname.tab.download")));
            t->elementAt(r+1, 3)->addWidget(cpp14::make_unique<WText>(tr("colSize.tab.download")));

            for (std::shared_ptr<layerBase> l : mDico->VlayersForGroupe(gr)){
                if (l->getCatLayer()!=TypeLayer::Externe & !l->Expert() & mDico->lay4Visu(l->Code()) & l->getFilesize()<globMaxDownloadFileS){
                    int row=t->rowCount();
                    t->elementAt(row, 0)->addWidget(cpp14::make_unique<WText>(WString::fromUTF8(l->Nom())));
                    WText * url =t->elementAt(row, 1)->addWidget(cpp14::make_unique<WText>(WString::fromUTF8(l->WMSURL())));
                    url->addStyleClass("mya");
                    t->elementAt(row, 2)->addWidget(cpp14::make_unique<WText>(WString::fromUTF8(l->WMSLayerName())));
                    t->elementAt(row, 3)->addWidget(cpp14::make_unique<WText>(WString::fromUTF8(roundDouble(l->getFilesize(),1)+ " Mo")));

                    Wt::WPushButton * b = t->elementAt(row, 4)->addWidget(cpp14::make_unique<Wt::WPushButton>("télécharger"));
                    t->elementAt(row, 4)->setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Middle);
                    Wt::WLink loadLink = Wt::WLink("/telechargement/"+l->Code());
                    b->clicked().connect([=]{m_app->addLog(l->Code(),typeLog::dsingleRW);});
                    b->setLink(loadLink);
                    // qml
                    if (l->hasSymbology()){
                        Wt::WPushButton * b2 = t->elementAt(row, 5)->addWidget(cpp14::make_unique<Wt::WPushButton>("télécharger le qml"));
                        t->elementAt(row, 5)->setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Middle);
                        Wt::WLink loadLink2 = Wt::WLink("/telechargement/"+l->Code()+"qml");
                        //b->clicked().connect([=]{m_app->addLog(l->Code(),typeLog::dsingleRW);});
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
        //std::cout << "ajout lMTD dans sous menu présentation " << lMTD.Nom() << std::endl;
        std::unique_ptr<Wt::WMenuItem> item = std::make_unique<Wt::WMenuItem>(lMTD.Label(), cpp14::make_unique<Wt::WText>(getHtml(&lMTD)));
        subMenu_->addItem(std::move(item));

    }

    std::unique_ptr<Wt::WMenuItem> item4 = std::make_unique<Wt::WMenuItem>("Forestimator API", cpp14::make_unique<Wt::WText>(WString::tr("docu.api")));
    subMenu_->addItem(std::move(item4));

    hLayout->addWidget(std::move(subMenu));
    hLayout->addWidget(std::move(subStack),1);   

}
