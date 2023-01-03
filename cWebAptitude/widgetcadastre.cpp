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

    section_->disable();
    paCa_->disable();
    division_->disable();

    // session pour se connecter à la db cadastre
    //std::cout << " bd cadastre " << mCad->mDirBDCadastre << std::endl;
    if (!boost::filesystem::exists(mCad->mDirBDCadastre)){std::cout << " bd cadastre " << mCad->mDirBDCadastre << " n'existe pas!! ça va planter ... \n\n\n\n" <<std::endl;}
    std::unique_ptr<dbo::backend::Sqlite3> sqlite3{new dbo::backend::Sqlite3(mCad->mDirBDCadastre)};
    session.setConnection(std::move(sqlite3));
    session.mapClass<capa>("capa");
}

void widgetCadastre::refreshDivision(){
    //std::cout << "refresh Division  Combobox" << std::endl;
    division_->clear();
    aMLabelDiv.clear();
    section_->clear();
    paCa_->clear();
    aMLabelPaCa.clear();
    section_->disable();
    paCa_->disable();
    division_->enable();
    if (commune_->currentIndex()!=0 && aMLabelCom.find(commune_->currentIndex())!=aMLabelCom.end()){   
        aMLabelDiv.emplace(std::make_pair(0,0));
        division_->addItem(tr("cadastre.choisir"));
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
    section_->clear();
    paCa_->clear();
    aMLabelPaCa.clear();
    section_->enable();
    if (division_->currentIndex()!=0 && aMLabelDiv.find(division_->currentIndex())!=aMLabelDiv.end()){

        section_->addItem(tr("cadastre.choisir"));

        for (auto & l : mCad->getSectionForDiv(aMLabelDiv.at(division_->currentIndex()),&session)){
            section_->addItem(l);
        }
    }
}

void widgetCadastre::refreshPaCa(){
    //std::cout << "refresh paca Combobox" << std::endl;
        paCa_->clear();
        aMLabelPaCa.clear();
        paCa_->enable();
        if (section_->currentIndex()!=0){
        paCa_->addItem(tr("cadastre.choisir"));
        int c(1);
        for (dbo::ptr<capa> l : mCad->getCaPaPtrVector(aMLabelDiv.at(division_->currentIndex()),section_->currentText().toUTF8(), & session)){
            paCa_->addItem(l->CaPaKey);
            aMLabelPaCa.emplace(std::make_pair(c,l->mPID));
            c++;
        }
        }
}

void widgetCadastre::submit(){
    // converti le polygone choisi en json et envoie le signal à parcellaire qui prend la relève pour l'affichage.
    if (commune_->currentIndex()!=0){

    if (division_->currentIndex()==0){
        // commune
        pathGeoJson_.emit(mCad->createPolygonCommune(aMLabelCom.at(commune_->currentIndex())),"commune");
    } else if (section_->currentIndex()==0){
        // division
        pathGeoJson_.emit(mCad->createPolygonDiv(aMLabelDiv.at(division_->currentIndex())),"division");
    } else if (paCa_->currentIndex()!=0 && section_->currentIndex()!=0){
        // parcelle cadastrale
        //pathGeoJson_.emit(mCad->createPolygonPaCa(paCa_->currentText().toUTF8()));
        // avec FID c'est plus rapide pour la recherche dans le shp
        pathGeoJson_.emit(mCad->createPolygonPaCa(aMLabelPaCa.at(paCa_->currentIndex())),"parcelle cadastrale");
    }
    }
}

