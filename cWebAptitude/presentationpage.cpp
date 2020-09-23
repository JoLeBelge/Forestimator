#include "presentationpage.h"

int nbAds(5); // Ads = advertising
// creation d'une bannières de pub (ads banner) pour faire défiler des informations descriptives du sites
presentationPage::presentationPage()
{
    // chargement des contenus
    for (int i(1); i<nbAds+1;i++){
        Wt::WContainerWidget * cont =this->addNew<WContainerWidget>();
        cont->addNew<Wt::WText>(Wt::WText::tr("presentation-ad"+std::to_string(i)));
    }

    // j'ai testé toutes les solutions trouvées sur le net, asio de boost, plein de timer (souvent bloquant), finalement le Wtimer est juste super
   Wt::WTimer * timer = this->addChild(cpp14::make_unique<Wt::WTimer>());
   timer->setInterval(std::chrono::seconds(7));
   timer->timeout().connect(this, &presentationPage::bannerAnimation);
   timer->start();

}



void presentationPage::bannerAnimation(){

    Wt::WAnimation animation(Wt::AnimationEffect::SlideInFromLeft,
                                Wt::TimingFunction::Linear,
                                1000);
    int i(0);
    if (this->currentIndex()==nbAds-1){i=0;}else{i=this->currentIndex()+1;}
    //std::cout << "presentationPage::bannerAnimation(), current Index = " << i << std::endl;
    this->setCurrentIndex(i,animation);

}
