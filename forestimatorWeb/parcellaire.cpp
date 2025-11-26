#include "parcellaire.h"

int globSurfMax(10000);      // en ha, surface max pour tout le shp
int globSurfMaxOnePol(1000); // en ha, surface max pour un polygone
int globVolMaxShp(10000);    // en ko // n'as pas l'air de fonctionner comme je le souhaite

extern bool globTest;

parcellaire::parcellaire(groupLayers *aGL, cWebAptitude *app, statWindow *statW) : mGL(aGL), centerX(0.0), centerY(0.0), mClientName(""), mName(""), mFullPath(""), m_app(app), fu(NULL), msg(NULL), hasValidShp(0), downloadRasterBt(NULL), mStatW(statW), poGeomGlobale(NULL), mLabelName("")
{
    // std::cout << "creation parcellaire " << std::endl;
    mDico = aGL->Dico();
    GDALAllRegister();
    mParcellaireExtent.MinX = 0;
    mParcellaireExtent.MinY = 0;
    mParcellaireExtent.MaxX = 0;
    mParcellaireExtent.MaxY = 0;

    setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Left);
    setInline(0);
    addWidget(std::make_unique<WText>(tr("anaStep1")));

    fu = addNew<Wt::WFileUpload>();
    fu->setFileTextSize(globVolMaxShp); // Set the maximum file size. il faut également changer param max-request-size dans wt_config
    fu->setFilters(".shp, .shx, .dbf, .prj, .gpkg");
    fu->setMultiple(true);
    fu->setInline(0);
    fu->addStyleClass("btn-file");
    addWidget(std::make_unique<Wt::WBreak>());

    msg = addWidget(std::make_unique<Wt::WText>());
    msg->setInline(0);
    addWidget(std::make_unique<WText>(tr("anaStep2")));

    mContSelect4D = addWidget(std::make_unique<Wt::WContainerWidget>());

    addWidget(std::make_unique<WText>(tr("anaStep3")));

    downloadRasterBt = addWidget(std::make_unique<Wt::WPushButton>(tr("parcellaire.tele.btn")));
    downloadRasterBt->setStyleClass("btn btn-success");
    downloadRasterBt->setWidth(200);
    downloadRasterBt->setInline(0);
    downloadRasterBt->disable();
    addWidget(std::make_unique<Wt::WBreak>());
    anaOnAllPolygBt = addWidget(std::make_unique<Wt::WPushButton>(tr("parcellaire.ana.btn")));
    anaOnAllPolygBt->setStyleClass("btn btn-success");
    anaOnAllPolygBt->setWidth(200);
    anaOnAllPolygBt->setInline(0);
    anaOnAllPolygBt->disable();

    fu->fileTooLarge().connect([=]
                               { msg->setText("Le fichier est trop volumineux (max " + std::to_string(globVolMaxShp) + "Ko)."); });
    fu->changed().connect(this, &parcellaire::fuChanged);
    fu->uploaded().connect(this, &parcellaire::upload);

    downloadRasterBt->clicked().connect(this, &parcellaire::downloadRaster);
    anaOnAllPolygBt->clicked().connect(this, &parcellaire::anaAllPol);
    //  ouu je pense que c'est mal, car si j'appui sur boutton télécharger les cartes, il me dis que toutes les cartes sont sélectionnées
    mContSelect4D->addWidget(std::unique_ptr<baseSelectLayers>(mGL->mSelectLayers));
}

parcellaire::~parcellaire()
{
    //cleanShpFile();
    m_app = NULL;
    msg = NULL;
    mGL = NULL;
    mDico = NULL;
}

void parcellaire::cleanShpFile()
{
    // std::cout << "clean shp files ..." ;
    std::vector<boost::filesystem::path> aFiles;
    // supprime les éventuels fichiers shp chargé
    for (auto &p : boost::filesystem::recursive_directory_iterator(mDico->File("TMPDIR")))
    {
        if (p.path().stem().string() == mName)
        {
            aFiles.push_back(p.path());
        }
    }
    for (auto file : aFiles)
    {
        boost::filesystem::remove(file);
    }
    // if (poGeomGlobale!=NULL){OGRGeometryFactory::destroyGeometry(poGeomGlobale);} // segfault
    hasValidShp = 0;
    // je devrait réinitialiser les nom mFullPath et autre ici aussi?
    mExtention = "";
}

bool parcellaire::computeGlobalGeom(std::string extension, bool limitSize)
{
    // std::cout << "computeGlobalGeom " << std::endl;
    bool aRes(0);
    if (extension == "")
    {
        extension = mExtention;
    }
    std::string input(mFullPath + "." + extension);
    const char *inputPath = input.c_str();
    GDALDataset *DS = (GDALDataset *)GDALOpenEx(inputPath, GDAL_OF_VECTOR | GDAL_OF_READONLY, NULL, NULL, NULL);
    if (DS != NULL)
    {
        OGRLayer *lay = DS->GetLayer(0);
        // union de tout les polygones du shp
        OGRFeature *poFeature;
        OGRGeometry *poGeom;
        OGRGeometry *poGeom2;
        std::unique_ptr<OGRMultiPolygon> multi = std::make_unique<OGRMultiPolygon>();
        OGRErr err = OGRERR_NONE;
        OGRMultiPolygon *poGeomM;

        int nbValidPol(0);
        while ((poFeature = lay->GetNextFeature()) != NULL)
        {
            // OGRGeometry * tmp=OGRGeometryFactory::forceTo((poFeature->GetGeometryRef()));
            poFeature->GetGeometryRef()->flattenTo2D();
            switch (poFeature->GetGeometryRef()->getGeometryType())
            {
            case (wkbPolygon):
            {
                poGeom = poFeature->GetGeometryRef();
                poGeom->closeRings();
                poGeom = poGeom->Buffer(0.0);
                // poGeom->Simplify(1.0);
                err = multi->addGeometry(poGeom);
                if (err == OGRERR_NONE)
                    nbValidPol++;
                break;
            }

            case wkbMultiPolygon:
            {
                poGeomM = poFeature->GetGeometryRef()->toMultiPolygon();
                int n(poGeomM->getNumGeometries());
                for (int i(0); i < n; i++)
                {
                    poGeom = poGeomM->getGeometryRef(i);
                    poGeom->closeRings();
                    poGeom = poGeom->Buffer(0.0);
                    // if (poFeature->GetGeometryRef()->getGeometryType()==wkbPolygon)
                    err = multi->addGeometry(poGeom);
                    if (err == OGRERR_NONE)
                        nbValidPol++;
                }
                break;
            }
            default:
                std::cout << "Geometrie " << poFeature->GetFID() << ", type de geometrie non pris en charge ; " << poFeature->GetGeometryRef()->getGeometryName() << ", id " << poFeature->GetGeometryRef()->getGeometryType() << std::endl;

                break;
            }
            if (err != OGRERR_NONE)
            {
                std::cout << "problem avec ajout de la geometrie " << poFeature->GetFID() << ", erreur : " << err << std::endl;
            }
            OGRFeature::DestroyFeature(poFeature);
        }

        // test si
        if (nbValidPol > 0)
        {
            poGeom2 = multi->UnionCascaded();

            poGeom2 = poGeom2->Buffer(1.0); // ça marche bien on dirait! je sais pas si c'est le buffer 1 ou le simplify 1 qui enlève les inner ring (hole) qui restent.
            poGeomGlobale = poGeom2->Simplify(1.0);
            int aSurfha = OGR_G_Area(poGeomGlobale) / 10000;
            printf("aSurfha=%d", aSurfha);
            if (!limitSize || aSurfha < globSurfMax)
            {
                poGeomGlobale->getEnvelope(&mParcellaireExtent);
                centerX = (mParcellaireExtent.MaxX + mParcellaireExtent.MinX) / 2;
                centerY = (mParcellaireExtent.MaxY + mParcellaireExtent.MinY) / 2;
                aRes = 1;
            }
            else
            {
                // message box
                auto messageBox =
                    addChild(std::make_unique<Wt::WMessageBox>(
                        "Import du shapefile polygone",
                        tr("parcellaire.upload.size"),
                        Wt::Icon::Information,
                        Wt::StandardButton::Ok));

                messageBox->setModal(true);
                messageBox->buttonClicked().connect([=]
                                                    { removeChild(messageBox); });
                messageBox->show();
            }
        }
    }
    else
    {
        std::cout << "computeGlobalGeom : je n'arrive pas à ouvrir " << mFullPath << "." << extension << std::endl;
    }
    GDALClose(DS);
    return aRes;
}

void parcellaire::display()
{
    // std::cout << " parcellaire::display " << std::endl;
    std::string JScommand = std::string("parcellaire = new ol.layer.Vector({") +
                            "source: new ol.source.Vector({" +
                            "format: new ol.format.GeoJSON()," +
                            "url: '" + geoJsonRelName() + "'})," +
                            "style:new ol.style.Style({" +
                            "stroke: new ol.style.Stroke({" +
                            "color: 'blue'," +
                            "width: 2})" +
                            "  })," +
                            "extent: [MINX,MINY,MAXX,MAXY]," +
                            "});" +
                            "updateGroupeLayers();" +
                            "map.getView().fit(parcellaire.getExtent());" +
                            "map.getView().setCenter([" + std::to_string(centerX) + "," + std::to_string(centerY) + " ]);";
    // extent du parcellaire
    boost::replace_all(JScommand, "MAXX", std::to_string(mParcellaireExtent.MaxX));
    boost::replace_all(JScommand, "MAXY", std::to_string(mParcellaireExtent.MaxY));
    boost::replace_all(JScommand, "MINX", std::to_string(mParcellaireExtent.MinX));
    boost::replace_all(JScommand, "MINY", std::to_string(mParcellaireExtent.MinY));
    doJavaScript(JScommand);
}

void parcellaire::fuChanged()
{
    // si on a changé le contenu, on tente de le télécharger
    fu->upload();
    // uploadButton->enable();
}

void parcellaire::upload()
{
    if (globTest)
    {
        std::cout << "upload commence.. ";
    }
    // computeStatButton->disable();
    downloadRasterBt->disable();
    // anaOnAllPolygBt->disable();
    //cleanShpFile();
    boost::filesystem::path p(fu->clientFileName().toUTF8()), p2(this->fu->spoolFileName());
    mClientName = p.stem().c_str();

    mName = ((std::string)p2.filename().c_str()) + "-" + mClientName;
    mFullPath = this->mDico->File("TMPDIR") + mName;

    int nbFiles(0);
    bool isShp(0);
    mExtention = "gpkg";
    for (Http::UploadedFile file : fu->uploadedFiles())
    {
        boost::filesystem::path a(file.clientFileName());
        // boost::filesystem::rename(file.spoolFileName(),mFullPath+a.extension().c_str());
        //  sur server boost::filesystem::rename: Invalid cross-device link:  car /data1 et tmp sont sur des différents volumes.
        // solution ; copy! de toute manière tmp/ est pugé souvent
        // std::cout << "ACHTUNG: " << mFullPath+a.extension().c_str() << ";" << std::endl;
        string it = "cp " + file.spoolFileName() + " " + mFullPath + a.extension().c_str();
        std::system(it.c_str());
        if ((a.extension().string() == ".shp") | (a.extension().string() == ".shx") | (a.extension().string() == ".dbf"))
        {
            nbFiles++;
            isShp = 1;
            mExtention = "shp";
        }
    }

    // ici je converti en json et affichage dans ol
    if (isShp)
    {
        if (nbFiles == 3)
        {
            msg->setText(tr("analyse.surf.msg.uploadOK"));
            if (globTest)
            {
                std::cout << "Téléchargement du shp effectué avec succès.. " << std::endl;
            }
            mLabelName = "";
            if (to31370AndGeoJson())
            {
                mGL->m_app->addLog("upload a shp");
                if (computeGlobalGeom())
                {
                    hasValidShp = true;
                    downloadRasterBt->enable();
                    anaOnAllPolygBt->enable();
                    display();
                    mGL->mMap->setToolTip(tr("tooltipMap2"));
                }
            }
        }
        else
        {
            msg->setText(tr("analyse.surf.msg.shpIncomplete"));
            cleanShpFile();
        }
    }
    else
    {
        // geopackage
        auto messageBox =
            addChild(std::make_unique<Wt::WMessageBox>(
                "Chargement de polygones au format Geopackage",
                tr("analyse.surf.msg.ImportGeopackage"),
                Wt::Icon::Warning,
                Wt::StandardButton::Ok));

        messageBox->setModal(true);
        messageBox->buttonClicked().connect([=]
                                            { removeChild(messageBox); });
        messageBox->show();

        mLabelName = "";
        if (to31370AndGeoJson())
        {
            mGL->m_app->addLog("upload a shp gpkg");
            if (computeGlobalGeom())
            {
                hasValidShp = true;
                downloadRasterBt->enable();
                anaOnAllPolygBt->enable();
                display();
                mGL->mMap->setToolTip(tr("tooltipMap2"));
            }
        }
        msg->setText(tr("analyse.surf.msg.uploadOK"));
    }
}

void parcellaire::visuStat(OGRFeature *poFeature)
{
    if (globTest)
    {
        std::cout << " parcellaire::visuStat()... ";
    }

    mStatW->vider();
    mStatW->titre("<h4>Statistique pour polygone FID " + std::to_string(poFeature->GetFID()) + " de " + mClientName + "</h4>");
    if (mLabelName != "")
    {
        mStatW->titre("<h4>Statistique pour " + mLabelName + "</h4>");
    }
    mStatW->generateGenCarte(poFeature);
    mStatW->genIndivCarteAndAptT();
    m_app->setInternalPath("/resultat", true);
}

// si je click sur un polygone dans ol, calcule les stat et affiche dans une nouvelle page
void parcellaire::computeStatAndVisuSelectedPol(int aId)
{
    std::cout << " computeStatAndVisuSelectedPol  , id " << aId << std::endl;
    m_app->loadingIndicator()->setMessage(tr("LoadingI1"));
    m_app->loadingIndicator()->show();
    std::string input(geoJsonName());
    const char *inputPath = input.c_str();
    GDALDataset *mDS = (GDALDataset *)GDALOpenEx(inputPath, GDAL_OF_VECTOR | GDAL_OF_READONLY, NULL, NULL, NULL);
    if (mDS != NULL)
    {
        // layer
        OGRLayer *lay = mDS->GetLayer(0);
        OGRFeature *poFeature;
        bool find(0);
        while ((poFeature = lay->GetNextFeature()) != NULL)
        {
            if (poFeature->GetFID() == aId)
            {
                OGRGeometry *poGeom = poFeature->GetGeometryRef();
                poGeom->closeRings();
                poGeom->flattenTo2D(); // sinon la géométrie est genre polygone3D et pas pris en charge dans staticMap par ex
                mGL->computeStatGlob(poGeom);
                find = 1;
                break;
            }
        }
        if (find)
        {
            visuStat(poFeature);
        }
        GDALClose(mDS);
    }
    else
    {
        std::cout << "ne parviens pas à lire " << input << std::endl;
    }
    m_app->loadingIndicator()->hide();
    m_app->loadingIndicator()->setMessage(tr("defaultLoadingI"));
}

std::string parcellaire::geoJsonName() { return mFullPath + ".geojson"; }
std::string parcellaire::geoJsonRelName() { return "tmp/" + mName + ".geojson"; }
std::string parcellaire::fileName() { return mFullPath + "." + mExtention; }

void parcellaire::downloadRaster()
{

    std::vector<rasterFiles> vRs = mGL->getSelect4Download();
    if (vRs.size() > 0)
    {
        m_app->addLog("download " + vRs.size(), typeLog::dmulti);
        m_app->loadingIndicator()->setMessage(tr("LoadingI4"));
        m_app->loadingIndicator()->show();
        // crée l'archive
        std::string suffix = "_ForestimatorRaster.zip";
        ZipArchive *zf = new ZipArchive(mFullPath + suffix);
        zf->open(ZipArchive::WRITE);
        // crop les raster selectionnés

        // mGL->mPBar->setValue(0);
        // mGL->mPBar->setMaximum(vRs.size());
        for (const rasterFiles &r : vRs)
        {
            std::string aCroppedRFile = mFullPath + "_" + r.Code() + ".tif";
            // mGL->mPBar->setToolTip("découpe de la carte " + r.code() + "...");
            if (cropImWithShp(r.getPathTif(), aCroppedRFile))
            {
                zf->addFile(mClientName + "_" + r.Code() + ".tif", aCroppedRFile);
                if (r.hasSymbology())
                {
                    zf->addFile(mClientName + "_" + r.Code() + ".qml", r.symbology());
                }
            }
            // mGL->mPBar->setValue(mGL->mPBar->value()+1);
            // m_app->processEvents();
        }
        // mGL->mPBar->setToolTip("");
        zf->close();
        delete zf;
        m_app->loadingIndicator()->hide();
        m_app->loadingIndicator()->setMessage(tr("defaultLoadingI"));

        WFileResource *fileResource = new Wt::WFileResource("plain/text", mFullPath + suffix);
        fileResource->suggestFileName(mClientName + "_raster.zip");
        m_app->redirect(fileResource->url());
    }
    else
    {
        auto messageBox =
            addChild(std::make_unique<Wt::WMessageBox>(
                "Sélection des couches à exporter",
                tr("download.lay.error.noLay"),
                Wt::Icon::Information,
                Wt::StandardButton::Ok));

        messageBox->setModal(true);
        messageBox->buttonClicked().connect([=]
                                            { removeChild(messageBox); });
        messageBox->show();
    }
}

bool parcellaire::cropImWithShp(std::string inputRaster, std::string aOut)
{
    bool aRes(0);
    std::cout << " cropImWithShp" << std::endl;
    // enveloppe de la géométrie globale
    OGREnvelope ext;
    poGeomGlobale->getEnvelope(&ext);
    aRes = cropIm(inputRaster, aOut, ext);
    return aRes;
}

void parcellaire::selectPolygon(double x, double y)
{
    if (!isnan(x) && !isnan(y) && !(x == 0 && y == 0))
    {
        std::cout << "parcellaire::selectPolygon " << std::endl;
        std::string input(geoJsonName()); // lecture du geojson et pas du shp, comme cela compatible avec polygone du cadastre.
        const char *inputPath = input.c_str();
        GDALDataset *mDS = (GDALDataset *)GDALOpenEx(inputPath, GDAL_OF_VECTOR | GDAL_OF_READONLY, NULL, NULL, NULL);
        if (mDS != NULL)
        {
            // layer
            OGRLayer *lay = mDS->GetLayer(0);
            OGRFeature *poFeature;
            lay->ResetReading();
            OGRPoint pt(x, y);

            while ((poFeature = lay->GetNextFeature()) != NULL)
            {
                OGRGeometry *poGeom = poFeature->GetGeometryRef();
                poGeom->closeRings();
                poGeom->flattenTo2D();
                if (pt.Within(poGeom))
                {
                    // je refais le test de la surface, car c'est peut-être un polgone de commune ou de division qui sont trop grand du coup
                    int aSurfha = OGR_G_Area(poGeom) / 10000;
                    if (aSurfha < globSurfMaxOnePol)
                    {
                        computeStatAndVisuSelectedPol(poFeature->GetFID());
                    }
                    else
                    {
                        // message box
                        auto messageBox =
                            addChild(std::make_unique<Wt::WMessageBox>(
                                "Analyse surfacique",
                                tr("parcellaire.upload.size"),
                                Wt::Icon::Information,
                                Wt::StandardButton::Ok));

                        messageBox->setModal(true);
                        messageBox->buttonClicked().connect([=]
                                                            { removeChild(messageBox); });
                        messageBox->show();
                    }
                    break;
                }
            }
            GDALClose(mDS);
        }
        else
        {
            std::cout << "select Polygone mDS is null " << std::endl;
        }
    }
}

void parcellaire::polygoneCadastre(std::string aFileGeoJson, std::string aLabelName)
{
    mFullPath = aFileGeoJson.substr(0, aFileGeoJson.size() - 8);
    fs::path p(aFileGeoJson);
    mName = p.filename().stem().c_str();
    mLabelName = aLabelName;

    if (computeGlobalGeom("geojson", 0))
    {
        hasValidShp = true;
        downloadRasterBt->enable();
        anaOnAllPolygBt->enable();
        display();
        mGL->mMap->setToolTip(tr("tooltipMap2"));
    }
}

void parcellaire::doComputingTask()
{
    std::string uuid = boost::uuids::to_string(boost::uuids::random_generator()());
    std::string filenameOut = mDico->File("TMPDIR") + uuid + ".xml";

    std::string ressourcePath = "results/" + uuid + ".xml";
    Wt::Mail::Message mail = tools::createMail(
        "JO.Lisein@uliege.be",
        "Lisein Jonathan",
        m_app->getUser().email() == "" ? m_app->getUser().unverifiedEmail() : m_app->getUser().email(),
        Wt::WString::tr("mail.anasMulti.title").toUTF8(),
        Wt::WString::tr("mail.anasMulti.body").arg(m_app->getUser().identity(Wt::Auth::Identity::LoginName)).arg(mLabelName == "" ? mClientName : mLabelName).arg(ressourcePath).toUTF8());

    parcellaire::TaskComputing *analyseSurfacique = new parcellaire::TaskComputing(geoJsonName(), mGL, filenameOut, &m_app);
    analyseSurfacique->setCallbackAfter([this, mail]() -> void
                                        {
        tools::sendMail(mail);
        m_app->addLog("Surface analysis report sent to " + m_app->getUser().email(), typeLog::anasMulti); });

    pool->add(analyseSurfacique);
}

void parcellaire::TaskComputing::run()
{
    std::string input(geoJsonName);
    const char *inputPath = input.c_str();
    cout << input.c_str();
    GDALDataset *mDS = (GDALDataset *)GDALOpenEx(inputPath, GDAL_OF_VECTOR || GDAL_OF_READONLY, NULL, NULL, NULL);
    if (mDS != NULL)
    {
        OGRLayer *lay = mDS->GetLayer(0);
        mGL->computeStatAllPol(lay, path);
        GDALClose(mDS);
    }
    else
    {
        std::cout << "select dataset mDS is null " << std::endl;
    }
}

void parcellaire::anaAllPol()
{
    if (!m_app->isLoggedIn())
    {
        auto messageBox =
            addChild(std::make_unique<Wt::WMessageBox>(
                "Analyse surfacique",
                tr("parcellaire.anaAllPol.connect"),
                Wt::Icon::Information,
                Wt::StandardButton::Ok));

        messageBox->setModal(true);
        messageBox->buttonClicked().connect([=]
                                            {
            removeChild(messageBox);
            m_app->showDialogues(0); // les autres fenetres
            m_app->dialog_auth->show();
            m_app->dialog_auth->raiseToFront(); });
        messageBox->show();
        return;
    }
    else
    {
        auto messageBox =
            addChild(std::make_unique<Wt::WMessageBox>(
                "Analyse surfacique",
                tr("parcellaire.anaAllPol.envoiEmail"),
                Wt::Icon::Information,
                Wt::StandardButton::Ok));

        messageBox->setModal(true);
        messageBox->buttonClicked().connect([=]
                                            { removeChild(messageBox); });
        messageBox->show();
        this->doComputingTask(); // Demarre le threadpool
        return;
    }
}

// le nouveau toGeoJson ; effectue un changement de src
bool parcellaire::to31370AndGeoJson()
{
    bool aRes = true;
    std::cout << " parcellaire::toGeoJson() ... ";
    // 0) suppression des polygones foireux - radical mais c'est l'utilisateur qui doit gérer ses propres merdes
    bool testEPSG(1);
    GDALDataset *DS = (GDALDataset *)GDALOpenEx(fileName().c_str(), GDAL_OF_VECTOR | GDAL_OF_UPDATE, NULL, NULL, NULL);
    if (DS != NULL)
    {
        OGRLayer *lay = DS->GetLayer(0);
        OGRSpatialReference *oSRS = lay->GetSpatialRef();
        if (oSRS == NULL)
        {
            testEPSG = 0;
        }
        else
        {
            int EPSG = oSRS->GetEPSGGeogCS();
            std::cout << "EPSG : " << EPSG << std::endl;
            if (EPSG == -1)
            {
                testEPSG = 0;
            }
        }
        OGRFeature *poFeature;
        OGRGeometry *poGeom;

        while ((poFeature = lay->GetNextFeature()) != NULL)
        {
            poGeom = poFeature->GetGeometryRef();
            if (poGeom->IsValid() != 1)
            {
                std::cout << "géométrie feature " << poFeature->GetFID() << " is invalid" << std::endl;
                lay->DeleteFeature(poFeature->GetFID()); // on est en lecture seule, donc ça ne devrai rien faire je crois. inutile?  GDAL_OF_READONLY
            }
        }

        GDALClose(DS);
        if (!testEPSG)
        {

            // https://gdal-dev.osgeo.narkive.com/6bntm7AI/assigning-spatialreference-to-ogrlayer
            //  assez dificile de définir un spatial ref pour un shp existant...

            Wt::WDialog *dialogBox = addChild(std::make_unique<Wt::WDialog>("Système de projection"));
            dialogBox->setModal(true);
            dialogBox->contents()->setOverflow(Overflow::Scroll);
            dialogBox->setClosable(false);
            dialogBox->contents()->addNew<Wt::WText>(tr("parcellaire.loadshp.srcNotDefined"));
            Wt::WComboBox *epsgSel = dialogBox->contents()->addNew<Wt::WComboBox>();
            epsgSel->addItem("Belge Lambert 72");
            epsgSel->addItem("WGS 84");
            epsgSel->setCurrentIndex(0);
            Wt::WPushButton *ok =
                dialogBox->footer()->addNew<Wt::WPushButton>("OK");
            ok->setDefault(true);
            ok->clicked().connect([=]
                                  { dialogBox->accept(); });
            dialogBox->finished().connect([=]
                                          {
                                              int inputEPSG(31370);
                                              if (epsgSel->currentIndex() == 1)
                                              {
                                                  inputEPSG = 4326;
                                              }
                                              std::cout << "epsg input " << inputEPSG << std::endl;
                                              removeChild(dialogBox);
                                              // to31370AndGeoJsonGDAL(inputEPSG);
                                              OGRSpatialReference src;
                                              src.importFromEPSG(inputEPSG);

                                              if (mExtention == "shp")
                                              {
                                                  // création du prj file
                                                  std::ofstream out;
                                                  out.open(mFullPath + ".prj");
                                                  char *wkt;
                                                  src.exportToWkt(&wkt);
                                                  out << wkt << "\n";
                                                  out.close();
                                              }
                                              to31370AndGeoJsonGDAL();
                                              return aRes; });
            dialogBox->show();
            return aRes;
        }
        else
        {
            to31370AndGeoJsonGDAL();
            return aRes;
        }
    }
    else
    {
        std::cout << " pas possible de lire fichier " << fileName() << std::endl;
        return 0;
    }
    // check le code EPSG - sur serveur j'ai un probleme avec les couche qui sont en user Defined scr 100036, la reprojection en BL72 me met le shp en décalage de 100 m
}

void parcellaire::to31370AndGeoJsonGDAL()
{

    std::string output(geoJsonName().c_str());
    const char *outPath = output.c_str();

    GDALDatasetH hSrcDS = GDALOpenEx(fileName().c_str(), GDAL_OF_VECTOR | GDAL_OF_READONLY, NULL, NULL, NULL);
    char **papszArgv = nullptr;

    /*if (inputEPSG!=-1){
        std::string command ="EPSG:"+std::to_string(inputEPSG);
        papszArgv = CSLAddString(papszArgv, "-s_srs");
        papszArgv = CSLAddString(papszArgv, command.c_str());
        std::cout << "command " << command << std::endl;
    }*/

    papszArgv = CSLAddString(papszArgv, "-t_srs"); // target src with reprojection
    papszArgv = CSLAddString(papszArgv, "EPSG:31370");
    GDALVectorTranslateOptions *option = GDALVectorTranslateOptionsNew(papszArgv, nullptr);
    if (option)
    {
        // 1 on converti en geojson pour ol
        GDALDatasetH hOutDS = GDALVectorTranslate(outPath, nullptr, 1, &hSrcDS, option, nullptr);
        if (hOutDS)
        {
            GDALClose(hOutDS);
        }
        else
        {
            std::cout << "changement de src par GDALVectorTranslate vers geojson; echec" << std::endl;
        }
    }
    else
    {
        std::cout << "options shp to geojson pas correctement parsées " << std::endl;
    }
    GDALVectorTranslateOptionsFree(option);
    GDALClose(hSrcDS);
    // la source devient le geojson
    papszArgv = nullptr;
    hSrcDS = GDALOpenEx(outPath, GDAL_OF_VECTOR | GDAL_OF_READONLY, NULL, NULL, NULL);
    papszArgv = CSLAddString(papszArgv, "-overwrite");
    GDALVectorTranslateOptions *option2 = GDALVectorTranslateOptionsNew(papszArgv, nullptr);
    if (option2)
    {
        // 2) on s'assure que le shp soit en BL72
        GDALDatasetH hOutDS = GDALVectorTranslate(fileName().c_str(), nullptr, 1, &hSrcDS, option2, nullptr);
        if (hOutDS)
        {
            GDALClose(hOutDS);
        }
    }
    GDALVectorTranslateOptionsFree(option2);
    GDALClose(hSrcDS);

    mGL->m_app->addLog("upload a shp");
    if (computeGlobalGeom())
    {
        hasValidShp = true;
        downloadRasterBt->enable();
        anaOnAllPolygBt->enable();
        display();
        mGL->mMap->setToolTip(tr("tooltipMap2"));
    }
    msg->setText(tr("analyse.surf.msg.uploadOK"));
}
