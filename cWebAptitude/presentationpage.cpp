#include "presentationpage.h"

int nbAds(5); // Ads = advertising
// creation d'une bannières de pub (ads banner) pour faire défiler des informations descriptives du sites
presentationPage::presentationPage()
{

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

   setContentAlignment(AlignmentFlag::Left);
   setOverflow(Wt::Overflow::Auto);
   addWidget(cpp14::make_unique<Wt::WTemplate>(WString::tr("page_presentation")));

}



void presentationPage::bannerAnimation(){

    Wt::WAnimation animation(Wt::AnimationEffect::SlideInFromLeft,
                                Wt::TimingFunction::Linear,
                                1000);
    int i(0);
    if (adsBanner->currentIndex()==nbAds-1){i=0;}else{i=adsBanner->currentIndex()+1;}
    //std::cout << "presentationPage::bannerAnimation(), current Index = " << i << std::endl;
    adsBanner->setCurrentIndex(i,animation);
}
