#include "presentationpage.h"

int globMaxDownloadFileS(1000);

const std::string showMenuText = "<i class='fa fa-bars' aria-hidden='true'></i> Afficher le menu";
const std::string closeMenuText = "<i class='fa fa-bars' aria-hidden='true'></i> Fermer le menu";

presentationPage::presentationPage(cDicoApt *aDico, cWebAptitude *app) : mDico(aDico), m_app(app), openMenuButton_(nullptr), menuOpen_(false)
{
  Wt::WTemplate *tpl = addWidget(cpp14::make_unique<Wt::WTemplate>(WString::tr("tpl:widget-gallery")));
  contentsStack_ = contentsStack_ = tpl->bindWidget("contents", std::make_unique<WStackedWidget>());

  Wt::WAnimation animation(Wt::AnimationEffect::Fade,
                           Wt::TimingFunction::Linear,
                           200);
  contentsStack_->setTransitionAnimation(animation, true);

  Wt::WMenu *menu = tpl->bindWidget("menu", std::make_unique<WMenu>(contentsStack_));
  menu->addStyleClass("flex-column");
  menu->setInternalPathEnabled("/documentation");

  openMenuButton_ = tpl->bindWidget("open-menu", std::make_unique<WPushButton>());
  openMenuButton_->setTextFormat(Wt::TextFormat::UnsafeXHTML);
  openMenuButton_->setText(showMenuText);

  openMenuButton_->clicked().connect(this, &presentationPage::toggleMenu);
  auto contentsCover = tpl->bindWidget("contents-cover", std::make_unique<Wt::WContainerWidget>());
  contentsCover->clicked().connect(this, &presentationPage::closeMenu);

  // forestimator
  auto subMenuPtr = std::make_unique<Wt::WMenu>(contentsStack_);
  auto subMenu = subMenuPtr.get();
  std::unique_ptr<Wt::WMenuItem> item = std::make_unique<Wt::WMenuItem>("Forestimator");
  item->setMenu(std::move(subMenuPtr));
  auto item_ = menu->addItem(std::move(item));
  subMenu->addStyleClass("nav-stacked submenu");
  subMenu->setInternalPathEnabled("/documentation/" + item_->pathComponent());
  item = std::make_unique<Wt::WMenuItem>("Forestimator : présentation");
  item->setContents(std::make_unique<WText>(WString::tr("page_presentation")));
  item->setPathComponent("");
  subMenu->addItem(std::move(item));

  item = std::make_unique<Wt::WMenuItem>("Crédit et contact", std::make_unique<Wt::WText>(WString::tr("page_presentation.credit")));
  subMenu->addItem(std::move(item));

  item = std::make_unique<Wt::WMenuItem>("Forestimator API", std::make_unique<Wt::WText>(WString::tr("docu.api")));
  subMenu->addItem(std::move(item));
  subMenu->addItem(std::move(downloadPage()));

  subMenuPtr = std::make_unique<Wt::WMenu>(contentsStack_);
  subMenu = subMenuPtr.get();
  subMenu->addStyleClass("nav-stacked submenu");
  item = std::make_unique<Wt::WMenuItem>("Fichier Ecologique des Essences");
  subMenu->setInternalPathEnabled("/documentation/" + item->pathComponent());
  item->setMenu(std::move(subMenuPtr));
  menu->addItem(std::move(item));
  int i(0);
  for (std::string c : {"Aptitude", "ECO", "ZBIO", "NH", "NT", "TOPOetSS", "AE"})
  {
    std::string label = WString::tr(c + ".label").toUTF8();
    if (!isValidXmlIdentifier(label) || !isValidHtml(label))
    {
        label = "";
        if (globTest){cout << "Warning: Label not found in FILE: forestimator-documentation.xml for TAG: " << c << ".label" << endl;}
    }
    std::unique_ptr<Wt::WMenuItem> item2 = std::make_unique<Wt::WMenuItem>(label, cpp14::make_unique<Wt::WText>(getHtml(c)));
    if (i == 0)
    {
      item2->setPathComponent("");
    }
    subMenu->addItem(std::move(item2));
    i++;
  }

  subMenuPtr = std::make_unique<Wt::WMenu>(contentsStack_);
  subMenu = subMenuPtr.get();
  item = std::make_unique<Wt::WMenuItem>("Peuplements forestiers");
  item->setMenu(std::move(subMenuPtr));
  item_ = menu->addItem(std::move(item));
  subMenu->addStyleClass("nav-stacked submenu");
  subMenu->setInternalPathEnabled("/documentation/" + item_->pathComponent());
  i = 0;
  for (std::string c : {"MF", "COMPO", "MNH", "dendro"})
  {
    std::string label = WString::tr(c + ".label").toUTF8();
    if (!isValidXmlIdentifier(label) || !isValidHtml(label))
    {
        label = "";
        if (globTest){cout << "Warning: Label not found in FILE: forestimator-documentation.xml for TAG: " << c << ".label" << endl;}
    }
    std::unique_ptr<Wt::WMenuItem> item2 = std::make_unique<Wt::WMenuItem>(label, cpp14::make_unique<Wt::WText>(getHtml(c)));
    if (i == 0)
    {
      item2->setPathComponent("");
    }
    subMenu->addItem(std::move(item2));
    i++;
  }
  subMenu->addItem(std::move(scolytePage()));

  subMenuPtr = std::make_unique<Wt::WMenu>(contentsStack_);
  subMenu = subMenuPtr.get();
  item = std::make_unique<Wt::WMenuItem>("Guide des Stations");
  item->setMenu(std::move(subMenuPtr));
  item_ = menu->addItem(std::move(item));
  subMenu->addStyleClass("nav-stacked submenu");
  subMenu->setInternalPathEnabled("/documentation/" + item_->pathComponent());

  std::unique_ptr<Wt::WMenuItem> item2 = std::make_unique<Wt::WMenuItem>("Présentation");
  item2->setContents(std::make_unique<WText>(tr("CS.intro")));
  item2->setPathComponent("");
  subMenu->addItem(std::move(item2));
  item2 = std::make_unique<Wt::WMenuItem>("Fiches stations");
  item2->setContents(std::make_unique<matAptCS>(mDico));
  subMenu->addItem(std::move(item2));

  for (std::string c : {"IGN", "MNT", "SWC"})
  {
    std::string label = WString::tr(c + ".label").toUTF8();
    if (!isValidXmlIdentifier(label) || !isValidHtml(label))
    {
        label = "";
        if (globTest){cout << "Warning: Label not found in FILE: forestimator-documentation.xml for TAG: " << c << ".label" << endl;}
    }
    std::unique_ptr<Wt::WMenuItem> item2 = std::make_unique<Wt::WMenuItem>(label, cpp14::make_unique<Wt::WText>(getHtml(c)));
    menu->addItem(std::move(item2));
  }

  item = std::make_unique<Wt::WMenuItem>("Confidentialité");
  item->setContents(std::make_unique<WText>(Wt::WString::tr("confidentialite")));
  menu->addItem(std::move(item));
}

std::unique_ptr<WMenuItem> presentationPage::downloadPage()
{
  std::unique_ptr<Wt::WMenuItem> item = std::make_unique<Wt::WMenuItem>("Téléchargement");
  Wt::WContainerWidget *c = new Wt::WContainerWidget();
  c->addNew<WText>(WString::tr("intro_telechargement"));
  Wt::WTable *t = c->addNew<Wt::WTable>();
  t->setHeaderCount(1);
  t->setWidth(Wt::WLength("90%"));
  t->toggleStyleClass("table-striped", true);
  t->elementAt(0, 0)->setColumnSpan(4);
  t->elementAt(0, 0)->setContentAlignment(AlignmentFlag::Top | AlignmentFlag::Center);
  t->elementAt(0, 0)->setPadding(10);
  t->elementAt(0, 0)->addWidget(std::make_unique<WText>(tr("titre.tab.download")));

  // on les présente par groupe de couches
  for (std::string gr : mDico->Dico_groupe)
  {
    // test si au moins une couche pour le groupe
    bool test(0);
    for (std::shared_ptr<layerBase> l : mDico->VlayersForGroupe(gr))
    {
      if (l->getCatLayer() != TypeLayer::Externe & !l->Expert() & mDico->lay4Visu(l->Code()))
      {
        test = 1;
        break;
      }
    }
    if (test)
    {

      int r = t->rowCount();
      t->elementAt(r, 0)->setColumnSpan(4);
      t->elementAt(r, 0)->addWidget(std::make_unique<WText>(WString::fromUTF8("<h4>" + mDico->groupeLabel(gr) + "</h4>")));
      t->elementAt(r, 0)->addStyleClass("bold");
      t->elementAt(r + 1, 1)->addWidget(std::make_unique<WText>(tr("colWMS.tab.download")));
      t->elementAt(r + 1, 2)->addWidget(std::make_unique<WText>(tr("colWMSname.tab.download")));
      t->elementAt(r + 1, 3)->addWidget(std::make_unique<WText>(tr("colSize.tab.download")));

      for (std::shared_ptr<layerBase> l : mDico->VlayersForGroupe(gr))
      {
        if (l->getCatLayer() != TypeLayer::Externe & !l->Expert() & mDico->lay4Visu(l->Code()) & l->getFilesize() < globMaxDownloadFileS)
        {
          int row = t->rowCount();
          t->elementAt(row, 0)->addWidget(std::make_unique<WText>(WString::fromUTF8(l->Nom())));
          WText *url = t->elementAt(row, 1)->addWidget(std::make_unique<WText>(WString::fromUTF8(l->WMSURL())));
          url->addStyleClass("mya");
          t->elementAt(row, 2)->addWidget(std::make_unique<WText>(WString::fromUTF8(l->WMSLayerName())));
          t->elementAt(row, 3)->addWidget(std::make_unique<WText>(WString::fromUTF8(roundDouble(l->getFilesize(), 1) + " Mo")));

          Wt::WPushButton *b = t->elementAt(row, 4)->addWidget(std::make_unique<Wt::WPushButton>("télécharger"));
          t->elementAt(row, 4)->setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Middle);
          Wt::WLink loadLink = Wt::WLink("/telechargement/" + l->Code());
          b->clicked().connect([=]
                               {m_app->addLog(l->Code(),typeLog::dsingleRW);
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

                        } });
          b->setLink(loadLink); // le lien pointe vers une ressource qui est générée dans main.cpp
          // qml
          if (l->hasSymbology())
          {
            Wt::WPushButton *b2 = t->elementAt(row, 5)->addWidget(std::make_unique<Wt::WPushButton>("télécharger le qml"));
            t->elementAt(row, 5)->setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Middle);
            Wt::WLink loadLink2 = Wt::WLink("/telechargement/" + l->Code() + "qml");
            b2->setLink(loadLink2);
          }
        }
      }
    }
  }
  item->setContents(std::unique_ptr<Wt::WContainerWidget>(c));

  return item;
}

std::unique_ptr<Wt::WMenuItem> presentationPage::scolytePage()
{

  std::string label = WString::tr("ES_EP.label").toUTF8();
  if (!isValidXmlIdentifier(label) || !isValidHtml(label))
  {
      label = "";
      cout << "Warning: Label not found in FILE: forestimator-documentation.xml for TAG: " << "ES_EP" << ".label" << endl;
  }
  std::unique_ptr<Wt::WMenuItem> mi = std::make_unique<Wt::WMenuItem>(label);
  Wt::WContainerWidget *ac = new Wt::WContainerWidget();
  ac->addNew<WText>(getHtml("ES_EP"));
  // ajout d'un média video via librairie wt
  ac->addNew<WText>(tr("ES_EP.video"));
  std::string mp4Video = "video/Argonne-illuCriseSco.mp4";
  // std::string ogvVideo = "https://www.webtoolkit.eu/videos/sintel_trailer.ogv";
  //  Define poster image location
  std::string poster = "img/scoMM.png";
  Wt::WVideo *video = ac->addNew<Wt::WVideo>();
  video->addSource(Wt::WLink(mp4Video), "");
  video->setPoster(poster);
  video->setAlternativeContent(std::make_unique<Wt::WImage>(Wt::WLink(poster)));
  video->resize(640, 360);
  mi->setContents(std::unique_ptr<Wt::WContainerWidget>(ac));
  return mi;
}

void presentationPage::toggleMenu()
{
  if (menuOpen_)
  {
    closeMenu();
  }
  else
  {
    openMenu();
  }
}

void presentationPage::openMenu()
{
  if (menuOpen_)
    return;

  openMenuButton_->setText(closeMenuText);
  addStyleClass("menu-open");

  menuOpen_ = true;
}

void presentationPage::closeMenu()
{
  if (!menuOpen_)
    return;

  openMenuButton_->setText(showMenuText);
  removeStyleClass("menu-open");

  menuOpen_ = false;
}
