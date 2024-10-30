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

    auto menu = std::make_unique<Wt::WMenu>(subStack.get());
    menu_ = menu.get();
    menu_->addStyleClass("flex-column");
    menu_->setInternalPathEnabled("/documentation");

    // forestimator
    auto subMenuPtr = std::make_unique<Wt::WMenu>(subStack.get());
    auto subMenu = subMenuPtr.get();
    std::unique_ptr<Wt::WMenuItem>  item = std::make_unique<Wt::WMenuItem>("Forestimator");
    item->setMenu(std::move(subMenuPtr));
    auto   item_ = menu_->addItem(std::move(item));
    subMenu->addStyleClass("nav-stacked submenu");
    subMenu->setInternalPathEnabled("/documentation/"+item_->pathComponent());
    item = std::make_unique<Wt::WMenuItem>("Forestimator : présentation");
    item->setContents(std::make_unique<WText>(WString::tr("page_presentation")));
    item->setPathComponent("");
    subMenu->addItem(std::move(item));

    item = std::make_unique<Wt::WMenuItem>("Crédit et contact", std::make_unique<Wt::WText>(WString::tr("page_presentation.credit")));
    subMenu->addItem(std::move(item));

    item = std::make_unique<Wt::WMenuItem>("Forestimator API", std::make_unique<Wt::WText>(WString::tr("docu.api")));
    subMenu->addItem(std::move(item));
    subMenu->addItem(std::move(downloadPage()));

    subMenuPtr = std::make_unique<Wt::WMenu>(subStack.get());
    subMenu = subMenuPtr.get();
    item = std::make_unique<Wt::WMenuItem>("Fichier Ecologique des Essences");
    item->setMenu(std::move(subMenuPtr));
    item_ = menu_->addItem(std::move(item));
    subMenu->addStyleClass("nav-stacked submenu");
    subMenu->setInternalPathEnabled("/documentation/" + item_->pathComponent());
    int i(0);
    for (std::string c : {"Aptitude","ECO","ZBIO","NH", "NT","TOPOetSS", "AE" }){
    LayerMTD lMTD =mDico->getLayerMTD(c);
    std::unique_ptr<Wt::WMenuItem> item2 = std::make_unique<Wt::WMenuItem>(lMTD.Label(), cpp14::make_unique<Wt::WText>(getHtml(&lMTD)));
    if (i==0){item2->setPathComponent("");}
    subMenu->addItem(std::move(item2));
    i++;
    }

    subMenuPtr = std::make_unique<Wt::WMenu>(subStack.get());
    subMenu = subMenuPtr.get();
    item = std::make_unique<Wt::WMenuItem>("Peuplements forestiers");
    item->setMenu(std::move(subMenuPtr));
    item_ = menu_->addItem(std::move(item));
    subMenu->addStyleClass("nav-stacked submenu");
    subMenu->setInternalPathEnabled("/documentation/" + item_->pathComponent());
    i=0;
    for (std::string c : {"MF","COMPO","MNH","dendro"}){
    LayerMTD lMTD =mDico->getLayerMTD(c);
    std::unique_ptr<Wt::WMenuItem> item2 = std::make_unique<Wt::WMenuItem>(lMTD.Label(), cpp14::make_unique<Wt::WText>(getHtml(&lMTD)));
     if (i==0){item2->setPathComponent("");}
    subMenu->addItem(std::move(item2));
    }
    subMenu->addItem(std::move(scolytePage()));

    subMenuPtr = std::make_unique<Wt::WMenu>(subStack.get());
    subMenu = subMenuPtr.get();
    item = std::make_unique<Wt::WMenuItem>("Guide des Stations");
    item->setMenu(std::move(subMenuPtr));
    item_ = menu_->addItem(std::move(item));
    subMenu->addStyleClass("nav-stacked submenu");
    subMenu->setInternalPathEnabled("/documentation/" + item_->pathComponent());
    //item_->setContents(std::make_unique<WText>(tr("CS.intro")));

    std::unique_ptr<Wt::WMenuItem> item2 = std::make_unique<Wt::WMenuItem>("Présentation");
    item2->setContents(std::make_unique<WText>(tr("CS.intro")));
    item2->setPathComponent("");
    subMenu->addItem(std::move(item2));
    item2 = std::make_unique<Wt::WMenuItem>("Fiches stations");
    item2->setContents(std::make_unique<matAptCS>(mDico));
    subMenu->addItem(std::move(item2));
    /*****************************************************************/


    // dernière page est différente, car pas de sous-menu

    item = std::make_unique<Wt::WMenuItem>("Confidentialité");
    item->setContents(std::make_unique<WText>(Wt::WString::tr("confidentialite")));
    menu_->addItem(std::move(item));
    //subMenu->setInternalPathEnabled("/documentation/" + item_->pathComponent());

    hLayout->addWidget(std::move(menu));
    stack_ =hLayout->addWidget(std::move(subStack),1);
}

std::unique_ptr<WMenuItem> presentationPage::downloadPage(){
    std::unique_ptr<Wt::WMenuItem> item = std::make_unique<Wt::WMenuItem>("Téléchargement");
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
    item->setContents(std::unique_ptr<Wt::WContainerWidget>(c));

    return item;
}

std::unique_ptr<Wt::WMenuItem> presentationPage::scolytePage(){

    LayerMTD lMTD=mDico->getLayerMTD("ES_EP");
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
    //menu_->addItem(std::move(mi));
    return mi;
}

void presentationPage::handlePathChanged(std::string path){
    if (globTest){ std::cout << "presentationPage::handlepathChange " << path << std::endl;}

    if (path.find("forestimator")!=std::string::npos){
        std::cout << "fo" << std::endl;
    //menu_->itemAt(0)->select();
    menu_->itemAt(0)->menu()->select(0);
    }else  if (path.find("fichier-ecologique-des-essences")!=std::string::npos){
     std::cout << "fee"  << std::endl;
    //menu_->itemAt(1)->select();
    menu_->itemAt(1)->menu()->select(0);
    }else  if (path.find("peuplements-forestiers")!=std::string::npos){
        std::cout << "pe f"  << std::endl;
    //menu_->itemAt(2)->select();
    menu_->itemAt(2)->menu()->select(0);
    }else  if (path.find("guide-des-stations")!=std::string::npos ){
        std::cout << "gsa"  << std::endl;
    //menu_->itemAt(3)->select();
    menu_->itemAt(3)->menu()->select(0);
    }
}
