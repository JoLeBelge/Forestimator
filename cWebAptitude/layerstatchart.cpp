#include "layerstatchart.h"

layerStatChart::layerStatChart(Layer *aLay, std::map<std::string, int> aStat, std::string aMode):layerStat(aLay,aStat,aMode),mTable(NULL),rowAtMax(0)
{

    //std::cout << "création d'un layer StatChart pour " << mLay->getLegendLabel() << std::endl;
    mModel = std::make_shared<WStandardItemModel>();
    // pas sur que j'ai besoin de spécifier le proto
    //mModel->setItemPrototype(cpp14::make_unique<WStandardItem>());

    // Configure the header.
    mModel->insertColumns(mModel->columnCount(), 2);
    //mModel->setHeaderData(0, WString(mLay->getLegendLabel()));
    mModel->setHeaderData(0, "Catégories");
    mModel->setHeaderData(1, WString("Proportions"));

    // Set data in the model.
    mModel->insertRows(mModel->rowCount(), mStatSimple.size());
    int row = 0;
    int aMax(1);

    //std::cout << "set data in model" << std::endl;
    for (auto & kv : mStatSimple){
        //clé : la valeur au format légende (ex ; Optimum). Valeur ; pourcentage pour ce polygone
        mModel->setData(  row, 0, WString(kv.first));
        //mModel->setData(  row, 1, WString("Blueberry"), ItemDataRole::ToolTip);
        mModel->setData(  row, 1, kv.second);
        if (kv.second>aMax) {aMax=kv.second; rowAtMax=row;}
        row++;
    }

}

Wt::WContainerWidget * layerStatChart::getChart(){
    // crée un smart ptr pour un chart vide
    //std::cout << " creation d'un chart " << std::endl;
Wt:WContainerWidget * aRes= new Wt::WContainerWidget();
    aRes->setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Center);
    aRes->setInline(0);
    aRes->setOverflow(Wt::Overflow::Auto);

    aRes->addWidget(cpp14::make_unique<WText>("<h4>"+mLay->getLegendLabel()+"</h4>"));
    aRes->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());
    //std::cout << " statsimple : " << mStatSimple.size() << " elem " << std::endl;
    if (mStatSimple.size()>0){
        WTableView* table =aRes->addWidget(cpp14::make_unique<WTableView>());
        aRes->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());
        table->setMargin(10, Side::Top | Side::Bottom);
        table->setMargin(WLength::Auto, Side::Left | Side::Right);
        table->setSortingEnabled(1,false);
        table->setSortingEnabled(0,false);// pas très utile
        //std::cout << "set model " << std::endl;
        table->setModel(mModel);
        table->setColumnWidth(0, 200);
        table->setColumnWidth(1, 150);
        table->setRowHeight(28);
        table->setHeaderHeight(28);
        table->setWidth(200 + 150 + 14+2);
        // seulement un chart pour les variables discontinues
        if (mTypeVar==TypeVar::Classe){
            Chart::WPieChart * aChart  =aRes->addWidget(cpp14::make_unique<Chart::WPieChart>());
            aChart->setModel(mModel);       // Set the model.
            aChart->setLabelsColumn(0);    // Set the column that holds the labels.
            aChart->setDataColumn(1);      // Set the column that holds the data.

            // changer la couleur
            //std::cout << "change la couleur" << std::endl;
            int row = 0;
            for (auto & kv : mStatSimple){
                std::string aCodeStr(kv.first);
                //std::cout << " row " <<  row << ", " << aCodeStr<< std::endl;
                color col(mLay->getColor(aCodeStr));
                // std::cout << "got color" << std::endl;
                aChart->setBrush(row,Wt::WBrush(Wt::WColor(col.mR,col.mG,col.mB)));
                //std::cout << "brush setted " << std::endl;
                row++;
            }
            //std::cout << "config la position des labels" << std::endl;
            // Configure location and type of labels.
            //aChart->setDisplayLabels(Chart::LabelOption::Outside |
            //                        Chart::LabelOption::TextLabel |
            //                        Chart::LabelOption::TextPercentage);
            // Enable a 3D and shadow effect.
            aChart->setPerspectiveEnabled(true, 0.2);
            aChart->setShadowEnabled(true);

            //if (mStat.size()>1) {aChart->setExplode(rowAtMax, 0.1);}  // Explode l'élément majoritaire WARN, le camembert sort du graphique, bug
            aChart->resize(300, 300);    // WPaintedWidget must be given an explicit size.
            aChart->setMargin(20, Side::Top | Side::Bottom); // Add margin vertically.
            //aChart->setMargin(WLength::Auto, Side::Left | Side::Right); // Center horizontally. il faut mettre des marges, qui sont comtpée au départ du cammembert, pour mettre les label
            aChart->setMargin(50, Side::Left | Side::Right);
        }

    } else {
        aRes->addWidget(cpp14::make_unique<WText>("Pas de statistique pour cette couche"));
    }

    return aRes;
}

void layerStat::simplifieStat(){

    int nbPix(0);
    for (auto & kv : mStat){
        nbPix+=kv.second;
    }

    switch (mTypeVar){
    case TypeVar::Classe:{

        // calcul des pourcentages au lieu du nombre de pixel
        if (nbPix>0){
            for (auto & kv : mStat){
                kv.second=(100*kv.second)/nbPix;
            }
        }

        int autres(0);
        int tot(0);
        for (auto & kv : mStat){
            if (kv.second>5){
                mStatSimple.emplace(kv);
                tot+=kv.second;
            } else {
                autres+=kv.second;
            }
        }
        if (autres>0) {
            tot+=autres;
            // correction de l'erreur d'arrondi si elle est de 2 pct max
            if ((tot>97) & (tot <100)) { autres+= 100-tot; tot=100;}
            mStatSimple.emplace(std::make_pair("Autre",autres));
        }
        // ajout pct pour no data - certaine couche l'on déjà, d'autre pas.
        if (tot<97 & tot>5){
            mStatSimple.emplace(std::make_pair("Sans données",100-tot));
        }
        break;}

    case TypeVar::Continu:{
        // regroupe les valeurs en 10 groupes avec le mm nombre d'occurence
        // classer les occurences par valeur de hauteur car la map n'est pas trié par ordre de  hauteur car 11.0 et avant 2 ou 20.0, tri 'alphabétique' des chiffres.

        std::map<double, int> aStatOrdered;
        for (auto & kv : mStat){
            aStatOrdered.emplace(std::make_pair(std::stod(kv.first),kv.second));
        }

        int nbClasse(10);
        int occurenceCumul(0);
        std::string curLimHaute("");
        double seuil(nbPix/nbClasse);
        int numClasCur(1);
        for (auto & kv : aStatOrdered){
            //std::cout << "hauteur " << kv.first << " , pixels " << kv.second << std::endl;
            if (kv.second!=0){
                if (kv.second<seuil){

                    occurenceCumul+=kv.second;
                    curLimHaute=nth_letter(numClasCur)+ " " + dToStr(kv.first);
                    //std::cout << " curLimHaute = " << curLimHaute << std::endl;

                } else {
                    if (occurenceCumul>0){
                        mStatSimple.emplace(curLimHaute,(100*occurenceCumul)/nbPix);
                        numClasCur++;
                        mStatSimple.emplace(nth_letter(numClasCur)+ " " +dToStr(kv.first),(100*kv.second)/nbPix);
                         numClasCur++;
                        occurenceCumul=0;
                    } else {
                        mStatSimple.emplace(nth_letter(numClasCur)+ " " +dToStr(kv.first),(100*kv.second)/nbPix);
                    }
                }
                if (occurenceCumul>seuil){
                    mStatSimple.emplace(curLimHaute,(100*occurenceCumul)/nbPix);
                    numClasCur++;
                    occurenceCumul=0;
                }
            }
        }

        if (occurenceCumul>0){
            mStatSimple.emplace(curLimHaute,(100*occurenceCumul)/nbPix);
        }

        /*for (auto & kv : mStatSimple){
            std::cout << " limite haute : " << kv.first << " , " << kv.second << "%" << std::endl;

        }*/

        break;
    }
    }

}

int layerStat::getFieldVal(bool mergeOT){
    unsigned int aRes(0);
    if (mLay->Type()==TypeLayer::Apti){
        aRes=getO(mergeOT);
    } else if (mLay->getCode()=="MNH2019"){
        int nbPixTreeCover(0);
        int nbPix(0);
        for (auto & kv : mStat){
            if (std::stod(kv.first)>2.0){nbPixTreeCover+=kv.second;}
            nbPix+=kv.second;
        }
        aRes=(100*nbPixTreeCover)/nbPix;
    } else {
        std::cout << "  pas de méthode pour remplir le champ de la table d'attribut pour le layer " << mLay->getLegendLabel() << std::endl;
    }
    return aRes;
}

int layerStat::getO(bool mergeOT){
    unsigned int aRes(0);
    if (mLay->Type()==TypeLayer::Apti){
        // il faudrait plutôt faire le calcul sur les statistique mStat, car StatSimple peut avoir regroupé des classe trop peu représentées!
        // mais attention alors car le % n'est pas encore calculé, c'est le nombre de pixels.
        for (auto & kv : mStatSimple){
            int codeApt(666);
            for (auto & kv2 : *mLay->mDicoVal){
                if (kv2.second==kv.first){codeApt=kv2.first;}
            }
            int aptContr=mLay->Dico()->AptContraignante(codeApt);
            if (mergeOT) {if (aptContr<3) aRes+=kv.second;} else { if (aptContr<2) aRes+=kv.second; }
        }}
    else {
        std::cout << " problem, on me demande de calculer la proportion d'optimum pour une carte qui n'est pas une carte d'aptitude" << std::endl;
    }
    return aRes;
}

layerStat::layerStat(Layer * aLay, std::map<std::string,int> aStat, std::string aMode):mLay(aLay),mStat(aStat),mMode(aMode),mTypeVar(aLay->Var()){

    simplifieStat();
}

std::string dToStr(double d){
    std::ostringstream streamObj;
    // Set Fixed -Point Notation
    streamObj << std::fixed;
    // Set precision to 1 digits
    streamObj << std::setprecision(1);
    //Add double to stream
    streamObj << d;
    return streamObj.str();
}

std::string  nth_letter(int n)
{
    std::string aRes("A");
    if (n >= 1 && n <= 26) { aRes="abcdefghijklmnopqrstuvwxyz"[n-1];}
    return aRes;
}


