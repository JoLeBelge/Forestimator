#include "grouplayers.h"

// 129000, 75001 taille en pix du masque forêt qui est passé à 2 m de résolution lors d'une mise à jours; j'aimerai pouvoir le faire télécharger

extern bool globTest;

groupLayers::groupLayers(cWebAptitude *cWebApt) : mDico(cWebApt->mDico), m_app(cWebApt), sigMapExport(this, "1.0"), sigMapCenter(this, "2.0"), slotMapCenter(this)
{
    setOverflow(Wt::Overflow::Visible);
    setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Middle);

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

    /* Liste cartes 1	*/
    std::unique_ptr<Wt::WTree> tree = std::make_unique<Wt::WTree>();
    tree->setSelectionMode(Wt::SelectionMode::Extended);
    tree->addStyleClass("tree_left");
    auto main_node = std::make_unique<Wt::WTreeNode>(tr("groupeCoucheAll"));
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
                    std::shared_ptr<Layer> aL = std::make_shared<Layer>(aLB, wtext);
                    // 3) ajout des interactions olala...
                    std::string aCode = aL->Code();
                    wtext->doubleClicked().connect([this, aCode]
                    { m_app->mPanier->addMap(aCode);});
                    aL->changeExpertMode().connect(n, &Wt::WTreeNode::setNodeVisible);
                    mVLs.push_back(aL);
                }
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
                std::shared_ptr<Layer> aL = std::make_shared<Layer>(aLB, wtext);
                // 3) ajout des interactions
                std::string aCode = aL->Code();
                wtext->doubleClicked().connect([this, aCode]
                    { m_app->mPanier->addMap(aCode);});
                aL->changeExpertMode().connect(n, &Wt::WTreeNode::setNodeVisible);
                //mVLs.push_back(aL);
            }
            else
            {
                std::cout << "problème pour couche " << aLB->Code() << " car n'as pas de groupe de couche atitré" << std::endl;
            }
        }
    }

    addWidget(std::make_unique<WText>(tr("coucheStep1")));
    addWidget(std::move(tree));


    if (globTest){std::cout << "done " << std::endl;}
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



/**
 * @brief groupLayers::computeStatGlob
 * @param poGeomGlobale
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





void groupLayers::exportLayMapView()
{

    std::cout << "exportLayMapView " << std::endl;

    std::shared_ptr<layerBase> l = mDico->getLayerBase(m_app->getActiveLay());
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
            //std::cout << "create archive pour raster croppé " << std::endl;
            auto zf = std::make_unique<ZipArchive>(archiveFileName);
            zf->open(ZipArchive::WRITE);
            // pour bien faire; choisir un nom qui soit unique, pour éviter conflict si plusieurs utilisateurs croppent la mm carte en mm temps
            zf->addFile(mClientName + ".tif", aCroppedRFile);
            if (l->hasSymbology())
            {
                zf->addFile(mClientName + ".qml", l->symbology());
            }
            // dictionnaire
            boost::filesystem::path tmpPath = boost::filesystem::path(mDico->File("TMPDIR")) / boost::filesystem::unique_path("tmp-%%%%-%%%%-%%%%.csv");
            std::ofstream out(tmpPath, std::ios::app);
            out << "Valeur Numérique Raster;Signification\n";
            out << l->getDicoValStr();
            out.close();
            zf->addFile(mClientName + "_dico.csv", tmpPath.c_str());

            // readme
            boost::filesystem::path tmpPath2 = boost::filesystem::path(mDico->File("TMPDIR")) / boost::filesystem::unique_path("tmp-%%%%-%%%%-%%%%.csv");
            std::ofstream out2(tmpPath2, std::ios::app);
            out2 << Wt::WString::tr("readme.download.layer").arg("https://forestimator.gembloux.ulg.ac.be/telechargement/" + l->Code()).toUTF8();
            out2.close();
            zf->addFile("README.txt", tmpPath2.c_str());

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
            WPushButton *bp = mExtentDiv->addNew<Wt::WPushButton>(n);
            bp->setInline(1);
            bp->clicked().connect([=]
            {
                doJavaScript("map.getView().setCenter(["+cx+","+cy+" ]);");
                doJavaScript("map.getView().setZoom("+z+");"); });
            WAnchor *del = mExtentDiv->addNew<Wt::WAnchor>(WLink(""), "(x)");
            del->clicked().connect([=]
            {
                WDialog * dialogPtr =  addChild(std::make_unique<Wt::WDialog>(tr("extent_del_comfirm")));
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
