#include "terrainviellecouperase.h"

extern bool globTest;


encodageRelTerrain::encodageRelTerrain(const Wt::WEnvironment& env, std::string aFileDB) : Wt::WApplication(env),
    session()
{
    messageResourceBundle().use(docRoot() + "/encodageVCR");
    std::shared_ptr<Wt::WBootstrap5Theme> theme = std::make_shared<Wt::WBootstrap5Theme>();
    setTheme(theme);
    useStyleSheet("style/style.css");
    useStyleSheet("resources/themes/default/wt.css");
    auto sqlite3 = std::make_unique<dbo::backend::Sqlite3>(aFileDB);
    sqlite3->setProperty("show-queries", "false");
    session.setConnection(std::move(sqlite3));
    session.mapClass<arbre>("arbres");
    session.mapClass<ue>("UE");
    session.mapClass<ACR_ter>("ACR_ter");
    std::cout << "encodageRelTerrain " << std::endl;

    try {
        session.createTables();
        std::cout << "Created analytics database." << std::endl;
    } catch (Wt::Dbo::Exception e){
        std::cout << "table creation failed"<< e.code() << std::endl;
    }

    setTitle("Encodage relevé terrain anciennes coupes rases");
    root()->setMargin(10);
    root()->setPadding(10);

    Wt::WContainerWidget * content = root()->addWidget(std::make_unique<Wt::WContainerWidget>());
    content->setOverflow(Wt::Overflow::Scroll);
    content->addNew<WText>("ACR id");
    ACR_id=content->addNew<WLineEdit>("");
    ACR_id->setValidator(std::make_shared<WIntValidator>(0,500));

    content->addNew<WText>("date des mesures");
    date=content->addNew<WLineEdit>("");

    content->addNew<WText>("opérateurs");
    ope=content->addNew<WLineEdit>("");

    content->addNew<WText>("gps(X5 ou RS2+)");
    gps=content->addNew<WLineEdit>("");

    content->addNew<WText>("Remarque Générale");
    ACRrmq=content->addNew<WTextArea>("");

    saveACRbut=content->addNew<WPushButton>("sauver les infos ACR");
    saveACRbut->clicked().connect(this,&encodageRelTerrain::saveACR);

    loadUE=content->addNew<WPushButton>("afficher arbres ACR");
    loadUE->clicked().connect([=] {
        if (ACR_id->valueText().toUTF8()!=""){
            int acr = std::stoi(ACR_id->valueText().toUTF8());
            displayACR(acr);
        }
    });

    Wt::WContainerWidget * delim = content->addNew<WContainerWidget>();
    delim->addStyleClass("delim");

    content->addNew<WText>("UE id");
    UE_id=content->addNew<WLineEdit>("");
    UE_id->setValidator(std::make_shared<WIntValidator>(0,50));
    content->addNew<WText>("GPS label (facultatif!!)");
    gpsLabel=content->addNew<WLineEdit>("");
    content->addNew<WText>("GPS remarque");
    gpsRmq=content->addNew<WLineEdit>("");
    content->addNew<WText>("Orientation (sens d'avancée du transect)");
    ori=content->addNew<WLineEdit>("");
    content->addNew<WText>("Composition (code essence séparé par -)");
    compo=content->addNew<WLineEdit>("");
    content->addNew<WText>("Remarques");
    ueRmq=content->addNew<WLineEdit>("");

    ACR_id->enterPressed().connect(date, &WWidget::setFocus);
    date->enterPressed().connect(ope, &WWidget::setFocus);
    ope->enterPressed().connect(gps, &WWidget::setFocus);
    gps->enterPressed().connect(ACRrmq, &WWidget::setFocus);
    ACRrmq->enterPressed().connect(UE_id, &WWidget::setFocus);

    UE_id->enterPressed().connect(gpsLabel, &WWidget::setFocus);
    gpsLabel->enterPressed().connect(gpsRmq, &WWidget::setFocus);
    gpsRmq->enterPressed().connect(ori, &WWidget::setFocus);
    ori->enterPressed().connect(compo, &WWidget::setFocus);
    compo->enterPressed().connect(ueRmq, &WWidget::setFocus);

    delim = content->addNew<WContainerWidget>();
    delim->addStyleClass("delim");

    tabNewEU = content->addWidget(std::make_unique<Wt::WTable>());
    tabNewEU->addStyleClass("table");
    tabNewEU->toggleStyleClass("table-striped",true);
    tabNewEU->setHeaderCount(1);
    tabNewEU->setWidth(Wt::WLength("100%"));
    tabNewEU->elementAt(0,0)->addNew<WText>("Type");
    tabNewEU->elementAt(0,1)->addNew<WText>("Arbres");
    tabNewEU->elementAt(0,2)->addNew<WText>("Ess");
    tabNewEU->elementAt(0,3)->addNew<WText>("Distance");
    tabNewEU->elementAt(0,4)->addNew<WText>("Circ");
    tabNewEU->elementAt(0,5)->addNew<WText>("Azim");
    tabNewEU->elementAt(0,6)->addNew<WText>("Hauteur");
    tabNewEU->elementAt(0,7)->addNew<WText>("Statut");
    tabNewEU->elementAt(0,8)->addNew<WText>("Régé");
    tabNewEU->elementAt(0,9)->addNew<WText>("Défaut");
    tabNewEU->elementAt(0,10)->addNew<WText>("Remarque");

    for (int i(0) ; i<10 ; i++){

        std::unique_ptr<arbreGUI>  tree = std::make_unique<arbreGUI>(&session,tabNewEU->rowAt(i+1));
        if (i>3 && i<8){
            tree->quadrat->setText(WString("Q{1}").arg(i-3));
            tree->type->setText("Arbustif");
        } else if (i<4){
            tree->quadrat->setText(WString("Q{1}").arg(i+1));
            tree->type->setText("Arbore");
        } else {
            tree->type->setText("Avenir");
            tree->quadrat->setText(WString("A{1}").arg(i-7));
        }

        //tabNewEU->elementAt(i+1,10)->enterPressed().connect(tabNewEU->elementAt(i+2,2), &WWidget::setFocus);
        //tabNewEU->elementAt(i+1,1)->keyWentDown().connect(tabNewEU->elementAt(i+2,1), &WWidget::setFocus);
        // c'est l'objet dans la cellulle qui doit avoir le connect
        vArbres.push_back(std::move(tree));
    }

    comboA1 = tabNewEU->elementAt(9,11)->addNew<WComboBox>();
    comboA2 = tabNewEU->elementAt(10,11)->addNew<WComboBox>();

    comboA1->addItem("-");
    comboA1->addItem("Q1");
    comboA1->addItem("Q2");
    comboA1->addItem("Q3");
    comboA1->addItem("Q4");

    comboA2->addItem("-");
    comboA2->addItem("Q1");
    comboA2->addItem("Q2");
    comboA2->addItem("Q3");
    comboA2->addItem("Q4");

    comboA1->changed().connect([=] {
        if(comboA1->valueText().toUTF8()== "Q1"){
            vArbres.at(8)->setLike(vArbres.at(0).get());
        } else if(comboA1->valueText().toUTF8()== "Q2"){
            vArbres.at(8)->setLike(vArbres.at(1).get());
        } else if(comboA1->valueText().toUTF8()== "Q3"){
            vArbres.at(8)->setLike(vArbres.at(2).get());
        } else if(comboA1->valueText().toUTF8()== "Q4"){
            vArbres.at(8)->setLike(vArbres.at(3).get());
        }
        vArbres.at(8)->rmq->setText(comboA1->valueText());
    });

    comboA2->changed().connect([=] {
        if(comboA2->valueText().toUTF8()== "Q1"){
            vArbres.at(9)->setLike(vArbres.at(0).get());
        } else if(comboA2->valueText().toUTF8()== "Q2"){
            vArbres.at(9)->setLike(vArbres.at(1).get());
        } else if(comboA2->valueText().toUTF8()== "Q3"){
            vArbres.at(9)->setLike(vArbres.at(2).get());
        } else if(comboA2->valueText().toUTF8()== "Q4"){
            vArbres.at(9)->setLike(vArbres.at(3).get());
        }
        vArbres.at(9)->rmq->setText(comboA2->valueText());
    });


    submitUE=content->addNew<WPushButton>("ajouter l'UE");
    submitUE->clicked().connect(this,&encodageRelTerrain::ajoutUE);

    delim = content->addNew<WContainerWidget>();
    delim->addStyleClass("delim");

    tabAllEU = content->addWidget(std::make_unique<Wt::WTable>());
    tabAllEU->setHeaderCount(1);
    tabAllEU->setWidth(Wt::WLength("100%"));
    tabAllEU->addStyleClass("table");
    tabAllEU->addStyleClass("tabEncod");
    tabAllEU->toggleStyleClass("table-striped",true);

}

void encodageRelTerrain::saveACR(){
    if (ACR_id->valueText().toUTF8()!=""){
        int acr = std::stoi(ACR_id->valueText().toUTF8());

        std::unique_ptr<ACR_ter> a = std::make_unique<ACR_ter>();
        a->id_ACR = acr;
        a->gps = gps->valueText().toUTF8();
        a->date = date->valueText().toUTF8();
        a->ope = ope->valueText().toUTF8();
        a->rmq = ACRrmq->valueText().toUTF8();
        dbo::Transaction transaction(session);
        dbo::ptr<ACR_ter> anACR = session.add(std::move(a));

        Wt::WMessageBox * messageBox = this->addChild(std::make_unique<Wt::WMessageBox>(
                                                          "Ancienne coupe rase",
                                                          "les infos de l'ancienne coupe rase ont été enregistrée. Vous pouvez à présent encoder les UE",
                                                          Wt::Icon::Information,
                                                          Wt::StandardButton::Ok));
        messageBox->setModal(true);
        messageBox->buttonClicked().connect([=] {
            this->removeChild(messageBox);
        });
        messageBox->show();
    }
}


void encodageRelTerrain::ajoutUE(){

    // d'abord un premier check
    if (ACR_id->valueText().toUTF8()==""){
        Wt::WMessageBox * messageBox = this->addChild(std::make_unique<Wt::WMessageBox>(
                                                          "Identifiant de l'ancienne coupe rase",
                                                          "Renseignez l'identifiant de l'ancienne coupe rase, svp",
                                                          Wt::Icon::Information,
                                                          Wt::StandardButton::Ok));
        messageBox->setModal(true);
        messageBox->buttonClicked().connect([=] {
            this->removeChild(messageBox);
        });
        messageBox->show();
    }else if (UE_id->valueText().toUTF8()==""){
        Wt::WMessageBox * messageBox = this->addChild(std::make_unique<Wt::WMessageBox>(
                                                          "Identifiant de l'unité d'échantillonnage",
                                                          "Renseignez l'identifiant de l'UE, svp",
                                                          Wt::Icon::Information,
                                                          Wt::StandardButton::Ok));
        messageBox->setModal(true);
        messageBox->buttonClicked().connect([=] {
            this->removeChild(messageBox);
        });
        messageBox->show();

    }else{
        int acr = std::stoi(ACR_id->valueText().toUTF8());
        int ue_id = std::stoi(UE_id->valueText().toUTF8());

        // vérfie qu'il n'y ai pas déjà un enregistrement pour cette ACR et cette UE
        typedef dbo::collection< dbo::ptr<arbre> > arbres;
        dbo::Transaction transaction(session);
        arbres vt = session.find<arbre>().where("id_ACR = ?").bind(acr).where("UE = ?").bind(ue_id);

        if (vt.size()>0){
            Wt::WMessageBox * messageBox = this->addChild(std::make_unique<Wt::WMessageBox>(
                                                              "Il existe déjà des arbres pour cette UE et cette ancienne coupe rase",
                                                              "Vous allez ajouter des arbres à une placette (UE) déjà existante",
                                                              Wt::Icon::Information,
                                                              Wt::StandardButton::Ok));
            messageBox->setModal(true);
            messageBox->buttonClicked().connect([=] {
                this->removeChild(messageBox);
            });
            messageBox->show();

        }


        bool allValid(1);
        for (std::unique_ptr<arbreGUI> &  tree : vArbres ){
            if (!tree->isValid()){allValid=0;}
        }


        if (!allValid){
            Wt::WMessageBox * messageBox = this->addChild(std::make_unique<Wt::WMessageBox>(
                                                              "Vérifiez l'encodage des arbres",
                                                              "Vous avez renseigné certaines valeurs de distance ou de hauteur qui ne sont pas valides",
                                                              Wt::Icon::Critical,
                                                              Wt::StandardButton::Ok));
            messageBox->setModal(true);
            messageBox->buttonClicked().connect([=] {
                this->removeChild(messageBox);
            });
            messageBox->show();
        } else {

            if (globTest) {std::cout << "ue add()"<<std::endl;}

            std::unique_ptr<ue> u = std::make_unique<ue>();
            u->id_ACR = acr;
            u->id = ue_id;
            u->gps = gpsLabel->valueText().toUTF8();
            u->rmqGPS = gpsRmq->valueText().toUTF8();
            u->compo = compo->valueText().toUTF8();
            if(ori->valueText().toUTF8()!=""){u->ori = std::stoi(ori->valueText().toUTF8());}
            u->rmq = ueRmq->valueText().toUTF8();

            //dbo::Transaction transaction(session);
            dbo::ptr<ue> anUE = session.add(std::move(u));

            if (globTest) {std::cout << " ue ajoutée à BD " << std::endl;}

            // sauver les arbres
            for (std::unique_ptr<arbreGUI> &  tree : vArbres ){
                tree->add(acr,ue_id);
                tree->clear();
            }

            UE_id->setValueText(std::to_string(ue_id+1));
            gpsLabel->setValueText("");
            gpsRmq->setValueText("");
            compo->setValueText("");
            //ori->setValueText("");
            ueRmq->setValueText("");

            comboA1->setCurrentIndex(0);
            comboA2->setCurrentIndex(0);
            displayACR(acr);

            Wt::WMessageBox * messageBox = this->addChild(std::make_unique<Wt::WMessageBox>(
                                                              "bravo",
                                                              "L'ue a correctement été enregistrée",
                                                              Wt::Icon::Information,
                                                              Wt::StandardButton::Ok));
            messageBox->setModal(true);
            messageBox->buttonClicked().connect([=] {
                this->removeChild(messageBox);
            });
            messageBox->show();
        }
    }
}

void encodageRelTerrain::displayACR(int acr_id){
    if (globTest) {std::cout << " displayACR " << std::endl;}
    tabAllEU->clear();
    tabAllEU->elementAt(0,0)->addNew<WText>("ACR_id");
    tabAllEU->elementAt(0,1)->addNew<WText>("UE");
    tabAllEU->elementAt(0,2)->addNew<WText>("Type");
    tabAllEU->elementAt(0,3)->addNew<WText>("Arbres");
    tabAllEU->elementAt(0,4)->addNew<WText>("Ess");
    tabAllEU->elementAt(0,5)->addNew<WText>("Circ");
    tabAllEU->elementAt(0,6)->addNew<WText>("Statut");
    tabAllEU->elementAt(0,7)->addNew<WText>("Régé");
    tabAllEU->elementAt(0,8)->addNew<WText>("Hauteur");
    tabAllEU->elementAt(0,9)->addNew<WText>("Distance");
    tabAllEU->elementAt(0,10)->addNew<WText>("Azimut");
    tabAllEU->elementAt(0,11)->addNew<WText>("Défaut");
    tabAllEU->elementAt(0,12)->addNew<WText>("Remarque");
    typedef dbo::collection< dbo::ptr<arbre> > arbres;
    dbo::Transaction transaction(session);
    arbres vt = session.find<arbre>().where("id_ACR = ?").bind(acr_id);
    int i=1;
    if (globTest) {std::cout << "found " << vt.size() << " tree for ACR " << acr_id << std::endl;}
    for (dbo::ptr<arbre> &t : vt){
        // je crée des wLineEdit pour pouvoir modifier les encodages? ce serait judicieux, flexible.
        tabAllEU->elementAt(i,0)->addNew<Wt::WLineEdit>(std::to_string(t->ACR));
        tabAllEU->elementAt(i,1)->addNew<Wt::WLineEdit>(std::to_string(t->UE));
        Wt::WLineEdit * type=tabAllEU->elementAt(i,2)->addNew<Wt::WLineEdit>(t->type);
        Wt::WLineEdit * quadrat=tabAllEU->elementAt(i,3)->addNew<Wt::WLineEdit>(t->quadrat);
        Wt::WLineEdit * ess=tabAllEU->elementAt(i,4)->addNew<Wt::WLineEdit>(t->ess);
        Wt::WLineEdit * circ=tabAllEU->elementAt(i,5)->addNew<Wt::WLineEdit>(std::to_string(t->circ));
        Wt::WLineEdit * statut=tabAllEU->elementAt(i,6)->addNew<Wt::WLineEdit>(std::to_string(t->statut));
        Wt::WLineEdit * rege=tabAllEU->elementAt(i,7)->addNew<Wt::WLineEdit>(std::to_string(t->rege));
        Wt::WLineEdit * Ht=tabAllEU->elementAt(i,8)->addNew<Wt::WLineEdit>(t->Ht);
        Wt::WLineEdit * dist=tabAllEU->elementAt(i,9)->addNew<Wt::WLineEdit>(t->dist);
        Wt::WLineEdit * azim=tabAllEU->elementAt(i,10)->addNew<Wt::WLineEdit>(std::to_string(t->azim));
        Wt::WLineEdit * defaut=tabAllEU->elementAt(i,11)->addNew<Wt::WLineEdit>(std::to_string(t->defaut));
        Wt::WLineEdit * rmq=tabAllEU->elementAt(i,12)->addNew<Wt::WLineEdit>(t->rmq);
        WPushButton * mod  =tabAllEU->elementAt(i,13)->addNew<Wt::WPushButton>("Modifier");
        //WPushButton * supprimer  =tabAllEU->elementAt(i,14)->addNew<Wt::WPushButton>("supprimer");
        circ->setValidator(std::make_shared<WIntValidator>(0,1000));
        statut->setValidator(std::make_shared<WIntValidator>(0,20));
        rege->setValidator(std::make_shared<WIntValidator>(0,20));
        azim->setValidator(std::make_shared<WIntValidator>(0,360));
        dist->setValidator(std::make_shared<WDoubleValidator>(0.0,11.0));
        Ht->setValidator(std::make_shared<WDoubleValidator>(0.0,50.0));

        i++;
        mod->clicked().connect([=] {

            if (dist->validate()==ValidationState::Valid && Ht->validate()==ValidationState::Valid){
                t.modify()->type = type->valueText().toUTF8();
                t.modify()->quadrat = quadrat->valueText().toUTF8();
                t.modify()->ess = ess->valueText().toUTF8();
                // if(Ht->valueText().toUTF8()!=""){t.modify()->Ht = std::stod( Ht->valueText().toUTF8());}
                t.modify()->Ht =  Ht->valueText().toUTF8();
                if(circ->valueText().toUTF8()!=""){t.modify()->circ = std::stoi(statut->valueText().toUTF8());}
                if(statut->valueText().toUTF8()!=""){t.modify()->statut = std::stoi(statut->valueText().toUTF8());}
                if(rege->valueText().toUTF8()!=""){t.modify()->rege = std::stoi( rege->valueText().toUTF8());}
                //if(dist->valueText().toUTF8()!=""){t.modify()->dist = std::stod( dist->valueText().toUTF8());}
                t.modify()->dist =  dist->valueText().toUTF8();
                if(azim->valueText().toUTF8()!=""){t.modify()->azim = std::stoi( azim->valueText().toUTF8());}
                if(defaut->valueText().toUTF8()!=""){t.modify()->defaut = std::stoi(defaut->valueText().toUTF8());}
                t.modify()->rmq = rmq->valueText().toUTF8();

                if (t->ess!=""){
                    dbo::Transaction transaction(session);
                    t.flush();

                    Wt::WMessageBox * messageBox = this->addChild(std::make_unique<Wt::WMessageBox>(
                                                                      "Modification d'un Arbre",
                                                                      "l'arbre a correctement été modifié",
                                                                      Wt::Icon::Information,
                                                                      Wt::StandardButton::Ok));
                    messageBox->setModal(true);
                    messageBox->buttonClicked().connect([=] {
                        this->removeChild(messageBox);
                    });
                    messageBox->show();
                }
            }
        });

        /*        supprimer->clicked().connect([=] {
            dbo::Transaction transaction(session);
            t.remove();
            //displayACR(acr_id);
        });*/

    }

}
