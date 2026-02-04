#include "grouplayers.h"

// 129000, 75001 taille en pix du masque forêt qui est passé à 2 m de résolution lors d'une mise à jours; j'aimerai pouvoir le faire télécharger

extern bool globTest;

groupLayers::groupLayers(cWebAptitude *cWebApt) : mDico(cWebApt->mDico), m_app(cWebApt), mMap(cWebApt->mMap), mTypeClassifST(FEE), mParent(cWebApt->mGroupLayerW), mLegendDiv(cWebApt->mLegendW), mAnaPoint(NULL), mSelectLayers(NULL), sigMapExport(this, "1.0"), sigMapCenter(this, "2.0"), slotMapCenter(this)
{
    mParent->setOverflow(Wt::Overflow::Visible);
    mParent->setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Middle);

    slotMapExport.setJavaScript(
        "function () {"
        "var extent = map.getView().calculateExtent(map.getSize());"
        "var bottomLeft = ol.extent.getBottomLeft(extent);"
        "var topRight =ol.extent.getTopRight(extent);"
        "if (bottomLeft != null) {" +
        sigMapExport.createCall({"topRight[0]", "topRight[1]", "bottomLeft[0]", "bottomLeft[1]"}) + "}}");

    this->sigMapExport.connect(std::bind(&groupLayers::updateMapExtentAndCropIm, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

    slotMapCenter.setJavaScript(
        "function () {"
        "var centre = map.getView().getCenter();"
        "var z = map.getView().getZoom();" +
        sigMapCenter.createCall({"centre[0]", "centre[1]", "z"}) + "}");
    this->sigMapCenter.connect(std::bind(&groupLayers::saveExtent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    mTitle = mLegendDiv->addWidget(std::make_unique<WText>(WString::tr("legendMsg")));

    /* Liste cartes 1	*/
    std::unique_ptr<Wt::WTree> tree = std::make_unique<Wt::WTree>();
    tree->setSelectionMode(Wt::SelectionMode::Extended);
    tree->addStyleClass("tree_left");
    // auto folderIcon = std::make_unique<Wt::WIconPair>("icons/yellow-folder-closed.png", "icons/yellow-folder-open.png", false);

    auto main_node = std::make_unique<Wt::WTreeNode>(tr("groupeCoucheAll")); // std::move(folderIcon) // pour mettre des icones ouvert/fermé !
    tree->setTreeRoot(std::move(main_node));
    tree->treeRoot()->label()->setTextFormat(Wt::TextFormat::Plain);
    tree->treeRoot()->setLoadPolicy(Wt::ContentLoading::NextLevel);
    tree->treeRoot()->expand();

    std::map<std::string, Wt::WTreeNode *> aMNodes;
    // ajout des nodes pour les groupes de couches
    for (std::string gr : mDico->Dico_groupe)
    {

        std::unique_ptr<Wt::WTreeNode> node1 = std::make_unique<Wt::WTreeNode>(mDico->groupeLabel(gr));
        node1->addStyleClass("tree_node");
        Wt::WTreeNode *n = tree->treeRoot()->addChildNode(std::move(node1));
        n->label()->clicked().connect([=]
                                      {
            if(n->isExpanded()){n->collapse();} else {
                n->expand();} });

        if (mDico->groupeExpert(gr))
        {
            this->changeExpertMode().connect(n, &Wt::WTreeNode::setNodeVisible);
        }
        aMNodes.emplace(gr, n);
    }

    // ajout des cartes
    for (auto &pair : mDico->VlayerBase())
    {
        std::shared_ptr<layerBase> aLB = pair.second;
        if (mDico->lay2groupe(pair.first) != "APT_FEE")
        {
            if (mDico->lay4Visu(pair.first))
            {
                // 1) création du mWtText pour cette couche
                WText *wtext = NULL;
                Wt::WTreeNode *n = NULL;
                if (aMNodes.find(mDico->lay2groupe(pair.first)) != aMNodes.end())
                {
                    n = aMNodes.at(mDico->lay2groupe(pair.first))->addChildNode(std::make_unique<Wt::WTreeNode>(""));
                    wtext = n->label();

                    // 2) création de la couche
                    std::shared_ptr<Layer> aL = std::make_shared<Layer>(this, aLB, wtext);
                    // 3) ajout des interactions
                    std::string aCode = aL->Code();
                    wtext->doubleClicked().connect([this, aCode]
                                                   { clickOnName(aCode); });
                    aL->changeExpertMode().connect(n, &Wt::WTreeNode::setNodeVisible);
                    mVLs.push_back(aL);
                }
                else
                {
                    std::cout << "problème pour couche " << pair.first << " car n'as pas de groupe de couche atitré" << std::endl;
                }
            }
            else
            {
                mVLs.push_back(std::make_shared<Layer>(this, aLB));
            }
        }
    }

    // on refais la boucle pour APT FEE pour avoir ordre alphabétique du nom en français et pas alpha du code ESS
    for (std::string essCode : mDico->Dico_Ess)
    {
        std::string essFEE = essCode + "_FEE";

        if (mDico->hasLayerBase(essFEE))
        {
            std::shared_ptr<layerBase> aLB = mDico->getLayerBase(essFEE);
            WText *wtext = NULL;
            Wt::WTreeNode *n = NULL;
            if (aMNodes.find(mDico->lay2groupe(aLB->Code())) != aMNodes.end())
            {
                n = aMNodes.at(mDico->lay2groupe(aLB->Code()))->addChildNode(std::make_unique<Wt::WTreeNode>(""));
                wtext = n->label();
                // 2) création de la couche
                std::shared_ptr<Layer> aL = std::make_shared<Layer>(this, aLB, wtext);
                // 3) ajout des interactions
                std::string aCode = aL->Code();
                wtext->doubleClicked().connect([this, aCode]
                                               { clickOnName(aCode); });
                aL->changeExpertMode().connect(n, &Wt::WTreeNode::setNodeVisible);
                mVLs.push_back(aL);
            }
            else
            {
                std::cout << "problème pour couche " << aLB->Code() << " car n'as pas de groupe de couche atitré" << std::endl;
            }
        }
    }

    mParent->addWidget(std::make_unique<WText>(tr("coucheStep1")));
    mParent->addWidget(std::move(tree));

    // mSelect4Stat= new selectLayers4Stat(this);
    mSelectLayers = new selectLayers(this);
    mStation = new ST(mDico);

    /*   AUTRES DIV   */
    mAnaPoint = new simplepoint(this, m_app->mSimplepointW);

    // updateGL pour cacher les couches expert
    // updateGL(); // -> bougé dans classe parent cwebapt car segfault not init refs !

    if (globTest){std::cout << "done " << std::endl;}
}

groupLayers::~groupLayers()
{
    if (globTest)
    {
        std::cout << "destructeur de group layer " << std::endl;
    }
    delete mAnaPoint;
    delete mStation;
    mMap = NULL;
    m_app = NULL;
    mDico = NULL;
    mAnaPoint = NULL;
    mVLs.clear();
}

void groupLayers::updateActiveLay(std::string aCode)
{
    // std::cout << " group Layers je vais faire un update du rendu visuel de chacun des label de couche \n\n\n" << std::endl;
    //  désactiver toutes les couches actives et changer le rendu du label
    for (std::shared_ptr<Layer> l : mVLs)
    {
        l->setActive(aCode == l->Code());
    }
}

void groupLayers::clickOnName(std::string aCode)
{

    // std::cout << " j'ai cliqué sur un label " << aCode <<  "\n\n"<< std::endl;
    //  udpate du rendu visuel de tout les labels de couches -- cela se situe au niveau du grouplayer
    updateActiveLay(aCode);

    TypeLayer type = TypeLayer::Init;
    std::shared_ptr<Layer> layer;
    for (std::shared_ptr<Layer> l : mVLs)
    {
        if (aCode == l->Code())
        {
            layer = l;
            type = l->getCatLayer();
            break;
        }
    }

    // cacher la fenetre popup
    mParent->doJavaScript("overlay?.setVisible(0);");

    // changer le mode CS vs FEE de grouplayer, utilise pour le tableau d'aptitude
    // attention de ne pas prendre la couche "CS_FEE" dans le tas (pour Chêne Sessile).
    if ((type == TypeLayer::CS) || (type == TypeLayer::KK) || (type == TypeLayer::Station && aCode.substr(0, 2) == "CS"))
    {
        if (globTest)
        {
            std::cout << " passe en mode classif CS , couche " << aCode << std::endl;
        }
        mTypeClassifST = TypeClassifST::CS;
    }
    else
    {
        mTypeClassifST = TypeClassifST::FEE;
        if (globTest)
        {
            std::cout << " passe en mode classif FEE , couche " << aCode << std::endl;
        }
    }
    // ajouter la couche à la carte
    for (std::shared_ptr<Layer> l : mVLs)
    {
        if (l->IsActive())
        {
            l->displayLayer();
            if (l->Code() != "IGN")
            {
                m_app->addLog("display layer " + l->Code(), typeLog::selectLayer);
            }
            break;
        }
    }
    // ajout de la carte après le "diplaylayer" sinon ordre du code js pas bon? ben oui car couche pas encore dans activeLayers[]
    m_app->mPanier->addMap(aCode, layer);
    updateLegendeDiv(m_app->mPanier->mVLs);
}

/**
 * @brief groupLayers::extractInfo Affiche les stat d'un point clicqué sur la map.
 * @param x
 * @param y
 */
void groupLayers::extractInfo(double x, double y)
{

    if (!isnan(x) && !isnan(y) && !(x == 0 && y == 0))
    {
        if (globTest)
        {
            std::cout << "groupLayers ; extractInfo " << x << " , " << y << std::endl;
        }
        mStation->vider();
        mStation->setOK();
        mStation->setX(x);
        mStation->setY(y);
        mAnaPoint->vider();

        // tableau des informations globales - durant ce round, l'objet ST est modifié
        mAnaPoint->titreInfoRaster();

        ptPedo ptPed = ptPedo(mDico->mPedo, x, y);
        mAnaPoint->add1InfoRaster(ptPed.displayInfo(PEDO::DRAINAGE));
        mAnaPoint->add1InfoRaster(ptPed.displayInfo(PEDO::PROFONDEUR));
        mAnaPoint->add1InfoRaster(ptPed.displayInfo(PEDO::TEXTURE));
        mAnaPoint->add1InfoRaster(ptPed.displayInfo(PEDO::CHARGE));

        for (std::shared_ptr<Layer> l : mVLs)
        {
            if (((l->getCatLayer() == TypeLayer::KK) | (l->getCatLayer() == TypeLayer::Station)) | ((l->IsActive()) & (l->getCatLayer() != TypeLayer::Externe)))
            {
                if (l->isVisible())
                {
                    std::vector<std::string> layerLabelAndValue = l->displayInfo(x, y);
                    if (l->l4StatP())
                    {
                        mAnaPoint->add1InfoRaster(layerLabelAndValue);
                    }
                    if ((l->IsActive()) && l->Code() != "CS_A")
                    {
                        // affiche une popup pour indiquer la valeur pour cette couche
                        // attention, il faut escaper les caractères à problèmes du genre apostrophe
                        boost::replace_all(layerLabelAndValue.at(1), "'", "\\'"); // javascript bug si jamais l'apostrophe n'est pas escapée
                        boost::replace_all(layerLabelAndValue.at(0), "'", "\\'");
                        mParent->doJavaScript("content.innerHTML = '<p>" + layerLabelAndValue.at(0) + ":</p><code>" + layerLabelAndValue.at(1) + "</code>';" + "var coordinate = [" + std::to_string(x) + "," + std::to_string(y) + "];" + "overlay.setPosition(coordinate);");
                    }
                    else if ((l->IsActive()) && l->Code() == "CS_A")
                    {
                        int aVal = l->getValue(x, y);
                        // affiche une popup pour indiquer la valeur pour cette couche
                        // attention, il faut escaper les caractères à problèmes du genre apostrophe
                        boost::replace_all(layerLabelAndValue.at(1), "'", "\\'"); // javascript bug si jamais l'apostrophe n'est pas escapée
                        boost::replace_all(layerLabelAndValue.at(0), "'", "\\'");

                        if (aVal != 0)
                        {
                            std::string js = "content.innerHTML = '<p>" + layerLabelAndValue.at(0) + ":</p><code>" + layerLabelAndValue.at(1) + "</code> <br></br> <a href=\"https://forestimator.gembloux.ulg.ac.be/telechargement/US-A" + std::to_string(aVal) + ".pdf\" target=\"_blank\" rel=\"noopener\">Consulter la description de la station forestière</a>';" + "var coordinate = [" + std::to_string(x) + "," + std::to_string(y) + "];" + "overlay.setPosition(coordinate);";
                            // std::cout << "js : " << js << std::endl;
                            mParent->doJavaScript(js);
                        }
                    }
                }
            }

            // si la couche active est la CNSW, on affiche les info pédo dans la fenetre "overlay". Attention, CNSWrast n'est plus "Externe" maintenant que j'y ai associé la couche raster.
            if ((l->IsActive() && l->Code() == "CNSWrast"))
            {
                mParent->doJavaScript("content.innerHTML = '" + ptPed.displayAllInfoInOverlay() + "';" + "var coordinate = [" + std::to_string(x) + "," + std::to_string(y) + "];" + "overlay.setPosition(coordinate);");
            }

            // si la couche active est le cadastre
            if ((l->IsActive() && l->Code() == "Cadastre"))
            {

                ptCadastre *ptCad = new ptCadastre(mDico->mCadastre, x, y);
                ptCad->sendPolygone().connect(std::bind(&parcellaire::polygoneCadastre, m_app->mPA, std::placeholders::_1, std::placeholders::_2));
                mParent->doJavaScript("content.innerHTML = '" + ptCad->displayAllInfoInOverlay() + "';" + "var coordinate = [" + std::to_string(x) + "," + std::to_string(y) + "];" + "overlay.setPosition(coordinate);");
                // comment créer le boutton pour que le polgyone du cadastre serve pour l'analyse surfacique? vu que je passe par du javascript pour la fenetre, je ne peux pas y ajouter de boutton...
                WDialog *dialogPtr = mParent->addChild(std::make_unique<Wt::WDialog>("Charger la parcelle cadastrale"));
                dialogPtr->contents()->addNew<Wt::WText>(tr("msg.charger.poly.capa"));
                WPushButton *ok = dialogPtr->footer()->addNew<Wt::WPushButton>("Oui");
                ok->setDefault(false);
                ok->clicked().connect([=]
                                      {
                    ptCad->usePolyg4Stat();
                    delete(ptCad);
                    dialogPtr->reject(); });
                WPushButton *annuler = dialogPtr->footer()->addNew<Wt::WPushButton>("Non");
                annuler->setDefault(true);
                annuler->clicked().connect([=]
                                           {dialogPtr->reject();
                delete(ptCad); });
                dialogPtr->show();
            }
        }
        // tableau du détail du calcul de l'aptitude d'une essence pour FEE
        for (std::shared_ptr<Layer> l : mVLs)
        {
            // on a bien une essence active et on est en mode FEE
            if (l->IsActive() && l->getCatLayer() == TypeLayer::FEE && mTypeClassifST == FEE)
            {
                // on note la chose dans l'objet ecogramme, car la classe simplepoint va devoir savoir si il y a un ecogramme à export en jpg ou non
                mStation->setHasFEEApt(1);
                mAnaPoint->detailCalculAptFEE(mStation);
            }
        }
        // tableau des aptitudes pour toutes les essences
        mAnaPoint->afficheAptAllEss();
        mMap->updateView();
    }
    else
    {
        std::cout << "x et y ne sont pas des nombres , pas bien " << std::endl;
    }
}

/**
 * @brief groupLayers::computeStatGlob
 * @param poGeomGlobale
 */
void groupLayers::computeStatGlob(OGRGeometry *poGeomGlobale)
{
    std::cout << " groupLayers::computeStatGlob " << std::endl;
    /*char * test;
    poGeomGlobale->exportToWkt(&test);
    std::cout << " geometrie WKT :" << test << std::endl;
    */

    clearStat();

    // pour les statistiques globales, on prend toutes les couches selectionnées par select4Download
    for (auto &l : getSelectedLayer4Download())
    {

        if (l->l4Stat())
        {
            // clé : la valeur au format légende (ex ; Optimum). Valeur ; pourcentage pour ce polygone
            std::map<std::string, int> stat = l->computeStat1(poGeomGlobale);
            mVLStat.push_back(std::make_shared<layerStatChart>(l, stat, poGeomGlobale));
        }

        // mPBar->setValue(mPBar->value() + 1);
        // m_app->processEvents();
    }
    // mPBar->setValue(mPBar->maximum());
    // return aRes;
    m_app->addLog("compute stat, " + std::to_string(getNumSelect4Download()) + " traitements", typeLog::anas); // add some web stats
}

void groupLayers::computeStatAllPol(OGRLayer *lay, std::string path)
{
    std::ofstream aFile(path.c_str());
    aFile.precision(10);

    aFile << "<layerStat>\n";
    OGRFeature *poFeature;
    OGRFeatureDefn *def = lay->GetLayerDefn();
    lay->ResetReading();
    while ((poFeature = lay->GetNextFeature()) != NULL)
    {
        // écriture des informations du polygones dans le fichiers résultats
        aFile << "<feature>\n";
        for (int f(0); f < def->GetFieldCount(); f++)
        {
            aFile << "<" << def->GetFieldDefn(f)->GetNameRef() << ">" << poFeature->GetFieldAsString(f) << "</" << def->GetFieldDefn(f)->GetNameRef() << ">"
                                                                                                                                                         "\n";
        }
        OGRGeometry *poGeom = poFeature->GetGeometryRef();
        poGeom->closeRings();
        poGeom->flattenTo2D();
        char *polWkt;
        poGeom->exportToWkt(&polWkt);
        for (auto &l : getSelectedLayer4Download())
        {
            aFile << "<processing>\n";
            if (l->Code() == "MNH2019")
            {
                aFile << "<processingName>hdom2019</processingName>\n";
                aFile << mDico->geoservice("hdom", "MNH2019", polWkt, typeAna::surfacique, 1);
            }
            else
            {
                if (l->l4Stat())
                {
                    aFile << "<processingName>" + l->Code() + "</processingName>\n";
                    aFile << mDico->geoservice(l->Code(), "", polWkt, typeAna::surfacique, 1);
                }
            }
            aFile << "</processing>\n";
        }
        aFile << "</feature>\n";
    }
    aFile << "</layerStat>\n";
    aFile.close();
    return;
}

/**
 * @brief groupLayers::apts
 * @return
 */
std::map<std::string, int> groupLayers::apts()
{
    std::map<std::string, int> aRes;
    switch (mTypeClassifST)
    {
    case FEE:
    {
        if (globTest)
        {
            std::cout << " GL get apt pour mode FEE " << std::endl;
        }
        if (mStation->readyFEE())
        {
            if (globTest)
            {
                std::cout << "station a NT NH ZBIO et Topo " << std::endl;
            }
            for (std::shared_ptr<Layer> l : mVLs)
            {
                if (l->getCatLayer() == TypeLayer::FEE)
                {
                    // j'ai deux solution pour avoir les aptitudes ; soit je lis la valeur du raster apt, soit je recalcule l'aptitude avec les variables environnementales
                    // std::shared_ptr<cEss> Ess= l->Ess();
                    if (mDico->hasEss(l->EssCode()))
                    {
                        std::shared_ptr<cEss> Ess = mDico->getEss(l->EssCode());
                        int apt = Ess->getFinalApt(mStation->mNT, mStation->mNH, mStation->mZBIO, mStation->mTOPO);
                        aRes.emplace(std::make_pair(Ess->Code(), apt));
                    }
                }
            }
        }
        break;
    }
    case CS:
    {
        if (globTest)
        {
            std::cout << " GL get apt pour mode CS " << std::endl;
        }
        if (mStation->readyCS())
        {
            if (globTest)
            {
                std::cout << "station a bien une station du catalogue " << std::endl;
            }
            for (auto kv : Dico()->getAllEss())
            {
                std::shared_ptr<cEss> Ess = kv.second;
                int apt = Ess->getApt(mStation->mZBIO, mStation->mSt);
                if (apt != 0)
                    aRes.emplace(std::make_pair(Ess->Code(), apt));
            }
        }
        break;
    }
    }
    return aRes;
}

/**
 * @brief groupLayers::updateGL
 */
void groupLayers::updateGL()
{
    bool expertMode = 0;

    if (m_app->isLoggedIn())
    {
        loadExtents(m_app->getUser().id());
        mExtentDivGlob->show();
        expertMode = getExpertModeForUser(m_app->getUser().id());
    }
    else
    {
        mExtentDivGlob->hide();
    }

    auto pdf = std::make_shared<pointPdfResource>(mAnaPoint);
    auto pdfLink = Wt::WLink(pdf);
    pdfLink.setTarget(Wt::LinkTarget::NewWindow);
    mAnaPoint->createPdfBut->setLink(pdfLink);

    // report analyse surfacique
    auto pdfSurf = std::make_shared<surfPdfResource>(m_app->mPA->mStatW);
    auto pdfLink2 = Wt::WLink(pdfSurf);
    pdfLink2.setTarget(Wt::LinkTarget::NewWindow);
    m_app->mPA->mStatW->createPdfBut->setLink(pdfLink2);

    // boucle sur les layers et envoi du signal pour cacher ou rendre visible les checkbox
    for (std::shared_ptr<Layer> l : mVLs)
    {
        l->ExpertMode(expertMode);
    }
    // pour cacher les noeuds racines , celui "aptitude CS"
    expertMode_.emit(expertMode);
}

/**
 * @brief groupLayers::updateLegendeDiv Refresh les legendes pour les couches du panier
 * @param layers
 */
void groupLayers::updateLegendeDiv(std::vector<std::shared_ptr<Layer>> layers)
{
    mLegendDiv->clear();

    if (layers.size() == 0)
        mTitle = mLegendDiv->addWidget(std::make_unique<WText>(WString::tr("legendMsg")));
    else
    {
        mTitle = mLegendDiv->addWidget(std::make_unique<WText>(WString::tr("legendTitre")));

        for (auto layer : layers)
        {
            this->updateLegende(layer);
        }
    }
}

/**
 * @brief groupLayers::updateLegende Refresh la legende pour une couche
 * @param l
 */
void groupLayers::updateLegende(const std::shared_ptr<Layer> l)
{
    if (l->getCatLayer() != TypeLayer::Externe)
    {
        Wt::WAnimation animation(Wt::AnimationEffect::SlideInFromTop, Wt::TimingFunction::EaseOut, 100);

        auto panel = std::make_unique<Wt::WPanel>();
        panel->setTitle("<h3>" + l->getLegendLabel() + "</h3>");
        panel->addStyleClass("centered-example");
        panel->setCollapsible(true);
        panel->setAnimation(animation);
        // panel->setCollapsed(false);

        auto tab = std::make_unique<WTable>();
        tab->setHeaderCount(1);
        tab->setWidth(Wt::WLength("90%"));
        tab->toggleStyleClass("table-striped", true);
        tab->setMaximumSize(1000, 1000);

        // mTitle->setText(WString::tr("legendTitre"));
        int row(0);
        for (auto kv : l->getDicoVal())
        {
            if (l->hasColor(kv.first))
            {
                std::shared_ptr<color> col = l->getColor(kv.first);
                tab->elementAt(row, 0)->addWidget(std::make_unique<WText>(kv.second));
                tab->elementAt(row, 1)->setWidth("40%");
                tab->elementAt(row, 1)->decorationStyle().setBackgroundColor(WColor(col->mR, col->mG, col->mB));
                row++;
            }
        }
        panel->setCentralWidget(std::move(tab));
        mLegendDiv->addWidget(std::move(panel));
    }
}

/**
 * @brief groupLayers::getActiveLay
 * @return
 */
std::shared_ptr<Layer> groupLayers::getActiveLay()
{
    std::shared_ptr<Layer> aRes = NULL;
    for (std::shared_ptr<Layer> l : mVLs)
    {
        if ((l->IsActive()))
        {
            aRes = l;
            break;
        }
    }
    // au lancement de l'appli, aucune couche n'est active
    if (aRes == NULL)
    {
        for (std::shared_ptr<Layer> l : mVLs)
        {
            if ((l->Code() == "IGN"))
            {
                aRes = l;
                l->setActive();
                break;
            }
        }
    }
    return aRes;
}

/**
 * @brief groupLayers::getLay Getter d'une couche selon son code
 * @param aCode
 * @return
 */
std::shared_ptr<Layer> groupLayers::getLay(std::string aCode)
{
    std::shared_ptr<Layer> aRes = NULL;
    for (std::shared_ptr<Layer> l : mVLs)
    {
        if ((l->Code() == aCode))
        {
            aRes = l;
            break;
        }
    }
    return aRes;
}

/**
 * @brief groupLayers::exportLayMapView Exporte la vue actuelle de la top couche du panier
 */
void groupLayers::exportLayMapView()
{

    std::cout << "exportLayMapView " << std::endl;

    std::shared_ptr<Layer> l = getActiveLay(); // attention, si on vient d'ouvrir le soft, aucune layer n'est actives!! gerer les ptr null
    if (l && l->getCatLayer() != TypeLayer::Externe)
    {

        // si la couche est un raster de valeur continue avec gain et offset, prévenir l'utilisateur avec une boite de dialogue
        if (l->getTypeVar() == TypeVar::Continu && l->Gain() != 1.0)
        {
            Wt::WMessageBox *messageBox = this->addChild(std::make_unique<Wt::WMessageBox>(
                "Attention",
                tr("msg.Gain.info").arg(l->Gain()),
                Wt::Icon::Information,
                Wt::StandardButton::Ok));
            messageBox->setModal(true);
            messageBox->buttonClicked().connect([=]
                                                { this->removeChild(messageBox); });
            messageBox->show();
        }

        m_app->loadingIndicator()->setMessage(tr("LoadingI3"));
        m_app->loadingIndicator()->show();
        // crop layer and download

        // crée l'archive
        std::string archiveFileName = mDico->File("TMPDIR") + "/" + l->Code() + ".zip";
        std::string aCroppedRFile = mDico->File("TMPDIR") + "/" + l->Code() + "_crop.tif";
        std::string mClientName = l->Code() + "_crop";
        //rasterFiles r = l->getRasterfile();
        if (l->cropIm(aCroppedRFile, mMapExtent))
        {
            std::cout << "create archive pour raster croppé " << std::endl;
            auto zf = std::make_unique<ZipArchive>(archiveFileName);
            zf->open(ZipArchive::WRITE);
            // pour bien faire; choisir un nom qui soit unique, pour éviter conflict si plusieurs utilisateurs croppent la mm carte en mm temps
            zf->addFile(mClientName + ".tif", aCroppedRFile);
            if (l->hasSymbology())
            {
                zf->addFile(mClientName + ".qml", l->symbology());
            }
            m_app->processEvents();
            zf->close();
            // le fileResources sera détruit au moment de la destruction GroupL
            WFileResource *fileResource = new Wt::WFileResource("plain/text", archiveFileName);
            fileResource->suggestFileName(mClientName + ".zip");
            m_app->addLog(l->Code(), typeLog::dsingle);
            m_app->redirect(fileResource->url());
        }
        else
        {
            Wt::WMessageBox *messageBox = this->addChild(std::make_unique<Wt::WMessageBox>(
                "Erreur",
                "<p> Cette couche ne peut être découpée sur cette emprise, essayer avec une zone plus petite.</p>",
                Wt::Icon::Critical,
                Wt::StandardButton::Ok));
            messageBox->setModal(true);
            messageBox->buttonClicked().connect([=]
                                                { this->removeChild(messageBox); });
            messageBox->show();
        }

        m_app->loadingIndicator()->hide();
        m_app->loadingIndicator()->setMessage(tr("defaultLoadingI"));
    }
    else
    {
        Wt::WMessageBox *messageBox = this->addChild(std::make_unique<Wt::WMessageBox>(
            "Erreur",
            "<p> Cette couche ne peut être téléchargé, veillez essayer avec une autres couche.</p>",
            Wt::Icon::Critical,
            Wt::StandardButton::Ok));

        messageBox->setModal(true);
        messageBox->buttonClicked().connect([=]
                                            { this->removeChild(messageBox); });
        messageBox->show();
    }
}

/** Fonctions pour gérer les extents sauvés dans DB * */
int groupLayers::openConnection()
{
    int rc;
    std::string db_path = m_app->docRoot() + "/extents.db";
    // std::cout << "..." << std::endl;
    rc = sqlite3_open_v2(db_path.c_str(), &db_, SQLITE_OPEN_READWRITE, NULL);
    if (rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db_));
        std::cout << " db_path " << db_path << std::endl;
    }
    return rc;
}

void groupLayers::closeConnection()
{
    int rc = sqlite3_close_v2(db_);
    if (rc)
    {
        fprintf(stderr, "Can't close database: %s\n\n\n", sqlite3_errmsg(db_));
    }
}

bool groupLayers::getExpertModeForUser(std::string id)
{

    openConnection();
    printf("get Expert Mode For User...");
    bool aRes(0);
    sqlite3_stmt *stmt;
    const char *query = "SELECT ModeExpert FROM user_expert WHERE id_user=?;";
    if (sqlite3_prepare_v2(db_, query, -1, &stmt, NULL) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, std::stoi(id));
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0) != SQLITE_NULL)
            {
                aRes = sqlite3_column_int(stmt, 0);
            }
            else
            {
                std::cout << "je ne parviens pas à lire la table user_expert " << std::endl;
            }
        }
        sqlite3_finalize(stmt);
    }

    closeConnection();
    std::cout << "mode expert est à " << aRes << std::endl;
    return aRes;
}

/**
 * @brief groupLayers::loadExtents Charge/recharge les extents du user
 * @param id
 */
void groupLayers::loadExtents(std::string id)
{
    openConnection();
    printf("loadextents...");
    mExtentDiv->clear();

    sqlite3_stmt *stmt;
    const char *query = "SELECT centre_x,centre_y,zoom,name,id FROM user_extent WHERE id_user=?";
    if (sqlite3_prepare_v2(db_, query, -1, &stmt, NULL) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, std::stoi(id));
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            std::string cx = std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)));
            std::string cy = std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)));
            std::string z = std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)));
            std::string n = std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3)));
            std::string id_extent = std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4)));
            /*std::cout << " value 1 : " << cx << std::endl;
            std::cout << " value 2 : " << cy << std::endl;
            std::cout << " value 3 : " << z << std::endl;*/
            WPushButton *bp = mExtentDiv->addNew<Wt::WPushButton>(n);
            bp->setInline(1);
            bp->clicked().connect([=]
                                  {
            mParent->doJavaScript("map.getView().setCenter(["+cx+","+cy+" ]);");
            mParent->doJavaScript("map.getView().setZoom("+z+");"); });
            WAnchor *del = mExtentDiv->addNew<Wt::WAnchor>(WLink(""), "(x)");
            del->clicked().connect([=]
                                   {
            WDialog * dialogPtr =  mParent->addChild(std::make_unique<Wt::WDialog>(tr("extent_del_comfirm")));
            WPushButton *ok = dialogPtr->footer()->addNew<Wt::WPushButton>("Supprimer");
            ok->setDefault(false);
            ok->clicked().connect([=]{
                printf("delete extent\n");
                deleteExtent(id_extent);
                dialogPtr->reject();
            });

            WPushButton * annuler = dialogPtr->footer()->addNew<Wt::WPushButton>("Annuler");
            annuler->setDefault(true);
            annuler->clicked().connect([=]{dialogPtr->reject();});

            dialogPtr->show(); });
        }
        closeConnection();

        // boutons pour enregistrer l'extent courant
        mExtentDiv->addNew<Wt::WBreak>();
        tb_extent_name = mExtentDiv->addWidget(std::make_unique<WLineEdit>());
        tb_extent_name->setInline(1);
        tb_extent_name->setPlaceholderText("Nom de l'emprise...");
        tb_extent_name->setWidth("200px");
        tb_extent_name->addStyleClass("extent_inline");
        WPushButton *button_s = mExtentDiv->addWidget(std::make_unique<WPushButton>(tr("sauver_extent")));
        button_s->addStyleClass("btn btn-success");
        button_s->setInline(1);
        button_s->addStyleClass("extent_button");
        button_s->clicked().connect(this->slotMapCenter);

        std::cout << "done" << std::endl;
    }
}

void groupLayers::saveExtent(double c_x, double c_y, double zoom)
{
    openConnection();

    std::string id = m_app->getUser().id();
    std::string n = tb_extent_name->text().toUTF8();
    boost::replace_all(n, "'", "");
    std::string cx = std::to_string((int)c_x);
    std::string cy = std::to_string((int)c_y);
    std::string z = std::to_string((int)zoom);
    const char *query = "INSERT INTO user_extent (id_user,centre_x,centre_y,zoom,name) VALUES (?,?,?,?,?)";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db_, query, -1, &stmt, NULL) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, std::stoi(id));
        sqlite3_bind_int(stmt, 2, (int)c_x);
        sqlite3_bind_int(stmt, 3, (int)c_y);
        sqlite3_bind_int(stmt, 4, (int)zoom);
        sqlite3_bind_text(stmt, 5, n.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) != SQLITE_DONE)
        {
            std::cerr << "saveExtent: Error inserting extent: " << sqlite3_errmsg(db_) << std::endl;
        }
        sqlite3_finalize(stmt);
    }
    closeConnection();

    loadExtents(id);
    m_app->addLog("save an extent", typeLog::extend); // add some web stats
}

void groupLayers::deleteExtent(std::string id_extent)
{
    openConnection();
    const char *query = "DELETE FROM user_extent WHERE id=?";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db_, query, -1, &stmt, NULL) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, std::stoi(id_extent));
        if (sqlite3_step(stmt) != SQLITE_DONE)
        {
            std::cerr << "deleteExtent: Error deleting extent: " << sqlite3_errmsg(db_) << std::endl;
        }
        sqlite3_finalize(stmt);
    }
    closeConnection();

    loadExtents(m_app->getUser().id());
    m_app->addLog("delete an extent", typeLog::extend); // add some web stats
}
/** FIN extents **/

std::vector<std::shared_ptr<Layer> > groupLayers::getSelect4Download() { return mSelectLayers->getSelectedLayer(); }
// std::vector<rasterFiles> groupLayers::getSelect4Stat(){return mSelect4Stat->getSelectedRaster();}

// int groupLayers::getNumSelect4Stat(){return mSelect4Stat->numSelectedLayer();}
int groupLayers::getNumSelect4Download() { return mSelectLayers->numSelectedLayer(); }

std::vector<std::shared_ptr<Layer>> groupLayers::getSelectedLayer4Download() { return mSelectLayers->getSelectedLayer(); }

bool isValidXmlIdentifier(const std::string &str)
{
    return str.find("??") == std::string::npos;
}

bool isValidHtml(const std::string &text)
{
    bool aRes(0);
    Wt::WText t(text);
    if (t.textFormat() == Wt::TextFormat::XHTML)
    {
        aRes = 1;
    }
    else
    {
        // std::cout << " attention, le texte " << text << " n'est pas un code html valide " << std::endl;
    }
    return aRes;
}

std::string getHtml(std::string groupCode)
{
    std::string title = WString::tr(groupCode + ".title").toUTF8();
    if (!isValidXmlIdentifier(title) || !isValidHtml(title))
    {
        title = "";
        if (globTest){cout << "Warning: Title not found in FILE: forestimator-documentation.xml for TAG: " << groupCode << ".title" << endl;}
    }

    std::string project = "<h4>Projet</h4>" + WString::tr(groupCode + ".projet").toUTF8();
    if (!isValidXmlIdentifier(project) || !isValidHtml(project))
    {
        project = "";
    }

    std::string description = "<h4>Description</h4>" + WString::tr(groupCode + ".description").toUTF8();
    if (!isValidXmlIdentifier(description) || !isValidHtml(description))
    {
        description = "";
        if (globTest){cout << "Warning: No description found in FILE: forestimator-documentation.xml for TAG: " << groupCode + ".description" << std::endl;}
    }

    std::string version = "<h4>Version</h4>" + WString::tr(groupCode + ".version").toUTF8();
    if (!isValidXmlIdentifier(version) || !isValidHtml(version))
    {
        version = "";
    }

    std::string logs = "<h4>Informations de modification</h4>" + WString::tr(groupCode + ".logs").toUTF8();
    if (!isValidXmlIdentifier(logs) || !isValidHtml(logs))
    {
        logs = "";
    }

    std::string copyright = "<h4>Copyright</h4>" + WString::tr(groupCode + ".copyright").toUTF8();
    if (!isValidXmlIdentifier(copyright) || !isValidHtml(copyright))
    {
        copyright = "";
    }

    std::string references = "<h4>Références</h4>" + WString::tr(groupCode + ".ref").toUTF8();
    if (!isValidXmlIdentifier(references) || !isValidHtml(references))
    {
        references = "";
    }
    return title + project + description + version + logs + copyright + references;
}

GDALDataset *getDSonEnv(std::string inputRaster, OGRGeometry *poGeom)
{
    OGREnvelope ext;
    poGeom->getEnvelope(&ext);
    GDALDataset *aRes = NULL;
    GDALDataset *DS = reinterpret_cast<GDALDataset *>(GDALOpen(inputRaster.c_str(), GA_ReadOnly));
    if (DS == NULL)
    {
        std::cout << "je n'ai pas lu l'image " << inputRaster << std::endl;
    }
    else
    {
        char **papszArgv = nullptr;
        papszArgv = CSLAddString(papszArgv, "-projwin"); // Selects a subwindow from the source image for copying but with the corners given in georeferenced coordinate
        papszArgv = CSLAddString(papszArgv, std::to_string(ext.MinX).c_str());
        papszArgv = CSLAddString(papszArgv, std::to_string(ext.MaxY).c_str());
        papszArgv = CSLAddString(papszArgv, std::to_string(ext.MaxX).c_str());
        papszArgv = CSLAddString(papszArgv, std::to_string(ext.MinY).c_str());

        GDALTranslateOptions *option = GDALTranslateOptionsNew(papszArgv, nullptr);
        if (option)
        {
            // std::cout <<" options parsées " << std::endl;
            GDALDatasetH DScrop = GDALTranslate("/vsimem/out", DS, option, nullptr);

            if (DScrop)
            {
                // j'ai toujours 2 raster a ouvrir conjointement
                aRes = GDALDataset::FromHandle(DScrop);
            }
        }

        GDALTranslateOptionsFree(option);
        if (papszArgv != nullptr)
        {
            CSLDestroy(papszArgv);
            papszArgv = nullptr;
        }
        GDALClose(DS);
    }
    return aRes;
}
