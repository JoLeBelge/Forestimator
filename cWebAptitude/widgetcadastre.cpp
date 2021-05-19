#include "widgetcadastre.h"

widgetCadastre::widgetCadastre(cadastre * aCad):mCad(aCad),WTable()
{
    int row(0);
    elementAt(row,0)->addWidget(cpp14::make_unique<WLabel>(tr("cadastre.commune")));
    commune_ = elementAt(0,1)->addWidget(cpp14::make_unique<WComboBox>());
    ++row;
    elementAt(row,0)->addWidget(cpp14::make_unique<WLabel>(tr("cadastre.division")));
    division_ = elementAt(0,1)->addWidget(cpp14::make_unique<WComboBox>());
    ++row;
    // cette partie est grisée tant que commune et division ne sont pas rempli
    elementAt(row,0)->addWidget(cpp14::make_unique<WLabel>(tr("cadastre.section")));
    section_ = elementAt(0,1)->addWidget(cpp14::make_unique<WComboBox>());
    ++row;
    elementAt(row,0)->addWidget(cpp14::make_unique<WLabel>(tr("cadastre.paca")));
    paCa_ = elementAt(0,1)->addWidget(cpp14::make_unique<WComboBox>());

    int c(0);
    for (auto & l : mCad->getCommuneLabel()){
        // sert à avoir directement le lien entre l'index de la combobox commune et le code postal
        aMLabelCom.emplace(std::make_pair(c,l.first));
        commune_->addItem(l.second);
        c++;
    }
    commune_->changed().connect(std::bind(&widgetCadastre::refreshDivision, this));
}

void widgetCadastre::refreshDivision(){
    std::cout << "refresh Division  Combobox" << std::endl;

    if (aMLabelCom.find(commune_->currentIndex())!=aMLabelCom.end()){
        division_->clear();
        aMLabelDiv.clear();
        int c(0);
        for (auto & l : mCad->getDivisionLabel(aMLabelCom.at(commune_->currentIndex()))){
            // sert à avoir directement le lien entre l'index de la combobox commune et le code postal
            aMLabelDiv.emplace(std::make_pair(c,l.first));
            division_->addItem(l.second);
            c++;
        }
    }
}
