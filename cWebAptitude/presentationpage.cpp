#include "presentationpage.h"

int nbAds(5); // Ads = advertising
// creation d'une bannières de pub (ads banner) pour faire défiler des informations descriptives du sites
presentationPage::presentationPage(cDicoApt *aDico):mDico(aDico)
{
    /*
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
   timer->start();
   */

    // création d'un menu à gauche, co dans wt widget gallery
   Wt::WHBoxLayout * hLayout = setLayout(std::make_unique<Wt::WHBoxLayout>());
   hLayout->setContentsMargins(0, 0, 0, 0);

   std::unique_ptr<Wt::WStackedWidget> subStack = std::make_unique<Wt::WStackedWidget>();
   subStack->addStyleClass("contents");
   subStack->setContentAlignment(AlignmentFlag::Left);
   subStack->setOverflow(Wt::Overflow::Auto);

   auto subMenu = std::make_unique<Wt::WMenu>(subStack.get());
   auto subMenu_ = subMenu.get();
   subMenu_->addStyleClass("nav-pills nav-stacked submenu");
   subMenu_->setWidth(200);

   subMenu_->setInternalPathEnabled();
   subMenu_->setInternalBasePath("/presentation");

   //addWidget(cpp14::make_unique<Wt::WTemplate>(WString::tr("page_presentation")));
   // introduction forestimator
   auto item = std::make_unique<Wt::WMenuItem>("Forestimator : présentation", cpp14::make_unique<Wt::WText>(WString::tr("page_presentation")));
   auto item_ = subMenu_->addItem(std::move(item));

   for( auto kv : *mDico->layerMTD()){
       LayerMTD lMTD=kv.second;
       //std::cout << "ajout lMTD dans sous menu présentation " << lMTD.Nom() << std::endl;
       auto item = std::make_unique<Wt::WMenuItem>(lMTD.Nom(), cpp14::make_unique<Wt::WText>(getHtml(&lMTD)));
       auto item_ = subMenu_->addItem(std::move(item));
   }

   hLayout->addWidget(std::move(subMenu));
   hLayout->addWidget(std::move(subStack),1);

}


/*
void presentationPage::bannerAnimation(){

    Wt::WAnimation animation(Wt::AnimationEffect::SlideInFromLeft,
                                Wt::TimingFunction::Linear,
                                1000);
    int i(0);
    if (adsBanner->currentIndex()==nbAds-1){i=0;}else{i=adsBanner->currentIndex()+1;}
    //std::cout << "presentationPage::bannerAnimation(), current Index = " << i << std::endl;
    adsBanner->setCurrentIndex(i,animation);
}
*/
