#include "widgetcadastre.h"

widgetCadastre::widgetCadastre(cadastre * aCad):mCad(aCad),WTable()
{
    int row(0);
    elementAt(row,0)->addWidget(cpp14::make_unique<WLabel>(tr("cadastre.commune")));
    commune_ = elementAt(0,1)->addWidget(cpp14::make_unique<WComboBox>());
    ++row;
    elementAt(row,0)->addWidget(cpp14::make_unique<WLabel>(tr("cadastre.division")));
    division_ = elementAt(row,1)->addWidget(cpp14::make_unique<WComboBox>());
    ++row;
    // cette partie est grisée tant que commune et division ne sont pas rempli
    elementAt(row,0)->addWidget(cpp14::make_unique<WLabel>(tr("cadastre.section")));
    section_ = elementAt(row,1)->addWidget(cpp14::make_unique<WComboBox>());
    ++row;
    elementAt(row,0)->addWidget(cpp14::make_unique<WLabel>(tr("cadastre.paca")));
    paCa_ = elementAt(row,1)->addWidget(cpp14::make_unique<WComboBox>());
    ++row;
    submit_ = elementAt(row,0)->addWidget(cpp14::make_unique<WPushButton>(tr("cadastre.submit")));
    submit_->clicked().connect(this, &widgetCadastre::submit);


    aMLabelCom.emplace(std::make_pair(0,0));
    commune_->addItem(tr("cadastre.choisir"));
    int c(1);
    for (auto & l : mCad->getCommuneLabel()){
        // sert à avoir directement le lien entre l'index de la combobox commune et le code postal
        aMLabelCom.emplace(std::make_pair(c,l.first));
        commune_->addItem(l.second);
        c++;
    }
    commune_->changed().connect(std::bind(&widgetCadastre::refreshDivision, this));
    division_->changed().connect(std::bind(&widgetCadastre::refreshSection, this));
    section_->changed().connect(std::bind(&widgetCadastre::refreshPaCa, this));
}

void widgetCadastre::refreshDivision(){
    //std::cout << "refresh Division  Combobox" << std::endl;
    if (commune_->currentIndex()!=0 && aMLabelCom.find(commune_->currentIndex())!=aMLabelCom.end()){
        division_->clear();
        //division_->setNoSelectionEnabled(1);
        //division_->setCurrentIndex(-1);
        aMLabelDiv.clear();        
        aMLabelDiv.emplace(std::make_pair(0,0));
        division_->addItem(tr("cadastre.choisir"));

        section_->clear();
        section_->setNoSelectionEnabled(1);
        //section_->setCurrentIndex(-1);
        paCa_->clear();
        //paCa_->setNoSelectionEnabled(1);
        paCa_->setCurrentIndex(-1);
        int c(1);
        for (auto & l : mCad->getDivisionLabel(aMLabelCom.at(commune_->currentIndex()))){
            // sert à avoir directement le lien entre l'index de la combobox div et le code
            aMLabelDiv.emplace(std::make_pair(c,l.first));
            division_->addItem(l.second);
            c++;
        }
    }
}

void widgetCadastre::refreshSection(){
    //std::cout << "refresh Section  Combobox" << std::endl;
    if (division_->currentIndex()!=0 && aMLabelDiv.find(division_->currentIndex())!=aMLabelDiv.end()){
        section_->clear();
        section_->setNoSelectionEnabled(1);
        section_->setCurrentIndex(-1);
        paCa_->clear();
        paCa_->setCurrentIndex(-1);
        int c(0);
        for (auto & l : mCad->getSectionForDiv(aMLabelDiv.at(division_->currentIndex()))){
            section_->addItem(l);
            c++;
        }
    }
}

void widgetCadastre::refreshPaCa(){
    //std::cout << "refresh paca Combobox" << std::endl;
        paCa_->clear();
        paCa_->setCurrentIndex(-1);
        int c(0);
        for (capa * l : mCad->getCaPaPtrVector(aMLabelDiv.at(division_->currentIndex()),section_->currentText().toUTF8())){
            paCa_->addItem(l->CaPaKey);
            c++;
        }
}

void widgetCadastre::submit(){
    // converti le polygone choisi en json et envoie le signal à parcellaire qui prend la relève pour l'affichage.
    if (commune_->currentIndex()!=0){

    if (division_->currentIndex()==0){
        // commune
        std::string name0 = std::tmpnam(nullptr);
        std::string name1 = name0.substr(5,name0.size()-5);
        //std::string aOut = mDico->File("TMPDIR")+"/"+name1+".geojson";
        std::cout << "\n" << mCad->createPolygonCommune(aMLabelCom.at(commune_->currentIndex())) << std::endl;
    }

    }
}

/*
void widgetCadastre::display(){
    //std::cout << " parcellaire::display " << std::endl;
    boost::system::error_code ec;
    std::string JSfile(mDico->File("addOLgeojson"));
    if (boost::filesystem::exists(JSfile,ec)){
        assert(!ec);
        std::cout << " ... " << std::endl;
        std::stringstream ss;
        std::string aFileIn(JSfile);

        std::ifstream in(aFileIn);
        ss << in.rdbuf();
        in.close();
        std::string JScommand(ss.str());

        std::string aFind1("NAME");

        std::string aReplace(geoJsonRelName());

        boost::replace_all(JScommand,aFind1,aReplace);

        // extent du parcellaire
        boost::replace_all(JScommand,"MAXX",std::to_string(mParcellaireExtent.MaxX));
        boost::replace_all(JScommand,"MAXY",std::to_string(mParcellaireExtent.MaxY));
        boost::replace_all(JScommand,"MINX",std::to_string(mParcellaireExtent.MinX));
        boost::replace_all(JScommand,"MINY",std::to_string(mParcellaireExtent.MinY));

        doJavaScript(JScommand);
        // centrer la map sur le shp
        doJavaScript("map.getView().setCenter(["+std::to_string(centerX)+","+std::to_string(centerY)+" ]);");
    } else {
        std::cout << " ne trouve pas le fichier script de js " << std::endl;
    }
    //std::cout << " done " << std::endl;
}
*/
