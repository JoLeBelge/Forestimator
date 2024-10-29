#include "desserteForest.h"

extern bool globTest;

formDesserteForest::formDesserteForest(const WEnvironment &env, cDicoApt *dico, std::string aFileDB) : Wt::WApplication(env),
    session(),mDico(dico),polygValid(0),mBDFile(aFileDB)
{

    auto sqlite3 = std::make_unique<dbo::backend::Sqlite3>(aFileDB);
    sqlite3->setProperty("show-queries", "false");
    session.setConnection(std::move(sqlite3));
    session.mapClass<desserte>("desserte");
    //std::cout << session.tableCreationSql() << std::endl; pour debug ça fonctionne bien ça..
    try {
        session.createTables();
        std::cout << "Created desserte database table." << std::endl;
    } catch (Wt::Dbo::Exception e){
        //std::cout << "table creation failed"<< e.code() << std::endl;
    }

    loadStyles();
    WLabel *label;

    messageResourceBundle().use(docRoot() + "/encodageDesserte");
    setTitle(WString("encodage Desserte Forestière"));
    Wt::WTemplate * tpl = root()->addWidget(cpp14::make_unique<Wt::WTemplate>(WString::tr("template")));
    WContainerWidget * cont = tpl->bindWidget("contTitre", std::make_unique<WContainerWidget>());
    cont->addNew<Wt::WText>(WString::tr("titre"));

    cont = tpl->bindWidget("contContact", std::make_unique<WContainerWidget>());
    cont->addNew<Wt::WText>(WString::tr("sectionEncoder"));

    WTable * table= cont->addNew<WTable>();
    int row=0;

    nom = table->elementAt(row,1)->addWidget(std::make_unique<WLineEdit>());
    table->elementAt(row,1)->setMinimumSize("300px", "300px");
    label = table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("nom")));
    label->setBuddy(nom);
    nom->setPlaceholderText(WString::tr("nom.ph"));
    ++row;
    prenom = table->elementAt(row,1)->addWidget(std::make_unique<WLineEdit>());
    label = table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("prenom")));
    label->setBuddy(prenom);
    prenom->setPlaceholderText(WString::tr("prenom.ph"));
    ++row;
    mail= table->elementAt(row,1)->addWidget(std::make_unique<WLineEdit>());
    label = table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("contact")));
    mail->setPlaceholderText(WString::tr("contact.ph"));
    mail->setValidator(std::make_shared<WValidator>(true));
    mail->validator()->setInvalidBlankText("Nous avons besoin de pouvoir vous contacter");
    mail->addStyleClass("textEdit");
    ++row;
    tel= table->elementAt(row,1)->addWidget(std::make_unique<WLineEdit>());
    label = table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("contact.gsm")));
    tel->setPlaceholderText(WString::tr("contact.gsm.ph"));
    ++row;

    typeContact= table->elementAt(row,1)->addWidget(std::make_unique<WComboBox>());
    label = table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("typeEncoder")));

    typeContact->addItem(WString::tr("typeEncoder0"));
    typeContact->addItem(WString::tr("typeEncoder1"));
    typeContact->addItem(WString::tr("typeEncoder2"));
    typeContact->addItem(WString::tr("typeEncoder3"));
    typeContact->addItem(WString::tr("typeEncoder4"));
    typeContact->addItem(WString::tr("typeEncoder5"));

    row++;
    contactPresicion= table->elementAt(row,1)->addWidget(std::make_unique<WLineEdit>());
    table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("contact.precision")));

    cont = tpl->bindWidget("contAmenagement", std::make_unique<WContainerWidget>());
    cont->addNew<Wt::WText>(WString::tr("sectionAme"));

    table= cont->addNew<WTable>();
    row=0;
    typeAM= table->elementAt(row,1)->addWidget(std::make_unique<WComboBox>());
    table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("type.am")));
    typeAM->addItem(WString::tr("type.am0"));
    typeAM->addItem(WString::tr("type.am1"));
    typeAM->addItem(WString::tr("type.am2"));
    typeAM->addItem(WString::tr("type.am3"));
    typeAM->addItem(WString::tr("type.am4"));
    typeAM->addItem(WString::tr("type.am5"));
    typeAM->addItem(WString::tr("type.am6"));
    row++;
    typeProprio= table->elementAt(row,1)->addWidget(std::make_unique<WComboBox>());
    table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("proprio.am")));
    typeProprio->addItem(WString::tr("proprio.am0"));
    typeProprio->addItem(WString::tr("proprio.am1"));
    typeProprio->addItem(WString::tr("proprio.am2"));
    typeProprio->addItem(WString::tr("proprio.am3"));
    ++row;
    checkRepresentant= table->elementAt(row,1)->addWidget(std::make_unique<WCheckBox>());
    checkRepresentant->setMinimumSize("30px","30px");
    label = table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("checkRepresentant")));

    cont = tpl->bindWidget("contLoca", std::make_unique<WContainerWidget>());
    cont->addStyleClass("encodage");

    WVBoxLayout * la = cont->setLayout(Wt::cpp14::make_unique<Wt::WVBoxLayout>());
    cont= la->addWidget(std::make_unique<WContainerWidget>());
    cont->addWidget(std::make_unique<Wt::WText>(WString::tr("titreLocalisation")));

    // choix de la géométrie
    cont->addWidget(std::make_unique<Wt::WText>(WString::tr("choix.am")));
    choixAM = cont->addWidget(std::make_unique<Wt::WComboBox>());
    choixAM->addItem(WString::tr("choix.am0"));
    choixAM->addItem(WString::tr("choix.am1"));
    choixAM->addItem(WString::tr("choix.am2"));
    choixAM->changed().connect(std::bind(&formDesserteForest::changeGeom, this));

    cont= la->addWidget(std::make_unique<WContainerWidget>());
    WHBoxLayout * layoutH = cont->setLayout(Wt::cpp14::make_unique<Wt::WHBoxLayout>());
    auto smart_map = std::make_unique<Wol>();
    Wol * map = smart_map.get();
    std::ifstream t(mDico->File("docroot")+"/js/initOL_desserteForest.js");
    std::stringstream ss;
    ss << t.rdbuf();
    doJavaScript(ss.str());
    layoutH->addWidget(std::move(smart_map));

    cont= layoutH->addWidget(std::make_unique<WContainerWidget>());
    cont->addNew<Wt::WText>(WString::tr("titreCommune"));
    commune_ = cont->addWidget(std::make_unique<WComboBox>());
    aMLabelCom.emplace(std::make_pair(0,0));
    commune_->addItem(WString::tr("cadastre.choisir"));
    int c(1);
    for (auto & l : mDico->mCadastre->getCommuneLabel()){
        // sert à avoir directement le lien entre l'index de la combobox commune et le code postal
        aMLabelCom.emplace(std::make_pair(c,l.first));
        commune_->addItem(l.second);
        c++;
    }
    commune_->changed().connect(std::bind(&formDesserteForest::displayCommune, this));

    cont->addNew<Wt::WText>(WString::tr("titreCatalogue"));
    std::vector<std::string> vLs={"IGN", "ortho2020", "ortho2023", "Cadastre"};
    for (std::string code : vLs){
        std::shared_ptr<layerBase> aL1=mDico->getLayerBase(code);
        WPushButton * but = cont->addNew<Wt::WPushButton>(aL1->Nom());
        but->addStyleClass("mybtn2");
        cont->addNew<WBreak>();
        but->clicked().connect([=] {this->displayLayer(aL1->Code());});
    }
    cont->addNew<WBreak>();
    cont->addNew<Wt::WText>(WString::tr("btnPolyg"));
    bCancel =cont->addNew<WPushButton>(WString::tr("initPol"));
    bCancel->setMinimumSize("100%"," ");
    bCancel->addStyleClass("btn btn-danger");
    bCancel->clicked().connect([=] {doJavaScript("acr_src.clear();map.addInteraction(draw);"); polygValid=0;});
    WPushButton * bUndo =cont->addNew<WPushButton>(WString::tr("cancelPoint"));
    bUndo->setMinimumSize("100%"," ");
    bUndo->clicked().connect([=] {doJavaScript("draw.removeLastPoint();");});
    map->polygGeojson().connect(std::bind(&formDesserteForest::validDraw,this, std::placeholders::_1));

    cont = tpl->bindWidget("contDesserte", std::make_unique<WContainerWidget>());
    table= cont->addNew<WTable>();
    table->addStyleClass("table-encodage");
    row=0;
    table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("desc.am")));
    row++;
    description= table->elementAt(row,0)->addWidget(std::make_unique<WTextArea>());
    description->setColumns(40);
    description->setRows(5);

    table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("secFinale.titre")));
    row=0;
    table= cont->addNew<WTable>();
    table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("desc.deposant")));
    row++;
    deposant = table->elementAt(row,0)->addWidget(std::make_unique<WComboBox>());
    deposant->addItem(WString::tr("desc.deposant0"));
    deposant->addItem(WString::tr("desc.deposant1"));
    deposant->addItem(WString::tr("desc.deposant2"));
    deposant->addItem(WString::tr("desc.deposant3"));
    row++;
    label = table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("acAdmin")));
    row++;
    acAdmin= table->elementAt(row,0)->addWidget(std::make_unique<WComboBox>());
    acAdmin->addItem(WString::tr("choix.acAdm0"));
    acAdmin->addItem(WString::tr("choix.acAdm1"));

    WPushButton * bSubmit = tpl->bindWidget("bsubmit", std::make_unique<WPushButton>(WString::tr("submit")));
    bSubmit->clicked().connect([=] {submit();});

    nom->enterPressed().connect(prenom, &WWidget::setFocus);
    prenom->enterPressed().connect(mail, &WWidget::setFocus);
    mail->enterPressed().connect(tel, &WWidget::setFocus);
    tel->enterPressed().connect(typeContact, &WWidget::setFocus);
    typeContact->enterPressed().connect(contactPresicion, &WWidget::setFocus);
    contactPresicion->enterPressed().connect(typeAM, &WWidget::setFocus);
    typeAM->enterPressed().connect(typeProprio, &WWidget::setFocus);
    typeProprio->enterPressed().connect(choixAM, &WWidget::setFocus);

}

void formDesserteForest::submit(){
    if (mail->validate()!=ValidationState::Valid){
        Wt::WMessageBox * messageBox = this->addChild(std::make_unique<Wt::WMessageBox>(
                                                          "Votre contact",
                                                          "Renseignez-nous un moyen pour vous contacter svp",
                                                          Wt::Icon::Information,
                                                          Wt::StandardButton::Ok));
        messageBox->setModal(true);
        messageBox->buttonClicked().connect([=] {
            this->removeChild(messageBox);
        });
        messageBox->show();
    } else if(!polygValid){
        Wt::WMessageBox * messageBox = this->addChild(std::make_unique<Wt::WMessageBox>(
                                                          Wt::WString::tr("dessinNonValid.t"),
                                                          Wt::WString::tr("dessinNonValid.c"),
                                                          Wt::Icon::Information,
                                                          Wt::StandardButton::Ok));
        messageBox->setModal(true);
        messageBox->buttonClicked().connect([=] {
            this->removeChild(messageBox);
        });
        messageBox->show();
    } else {
        // sauver la coupe rase dans la BD
        int rc;
        sqlite3 *db_;
        rc = sqlite3_open(mBDFile.c_str(), &db_);
        if( rc )  {} else {
            WLocalDateTime d= WLocalDateTime::currentDateTime();
            //std::cout << " sauve la coupe rase " << std::endl;
            sqlite3_stmt * stmt;
            SQLstring="INSERT INTO desserte (version,date,nom,prenom,mail,tel,typeContact,contactPrecision,checkRepresentant,typeAM,typeProprio,deposant,typeGeom,descr,acAdmin,geom) VALUES (0,'"
                    +d.toString().toUTF8()+"',"
                    +"'"+format4SQL(nom->valueText().toUTF8())+"',"
                    +"'"+format4SQL(prenom->valueText().toUTF8())+"',"
                    +"'"+format4SQL(mail->valueText().toUTF8())+"',"
                    +"'"+format4SQL(tel->valueText().toUTF8())+"',"
                    +"'"+format4SQL(typeContact->currentText().toUTF8())+"',"
                    +"'"+format4SQL(contactPresicion->valueText().toUTF8())+"',"
                    +std::to_string(checkRepresentant->isChecked())+","
                    +"'"+format4SQL(typeAM->currentText().toUTF8())+"',"
                    +"'"+format4SQL(typeProprio->currentText().toUTF8())+"',"
                    +"'"+format4SQL(deposant->currentText().toUTF8())+"',"
                    +std::to_string(choixAM->currentIndex()+1)+","
                    +"'"+format4SQL(description->valueText().toUTF8())+"',"
                    +"'"+format4SQL(acAdmin->valueText().toUTF8())+"',"
                    +"'"+aGeom+"');";
            //std::cout << "sql : " << SQLstring << std::endl;
            sqlite3_prepare_v2(db_, SQLstring.c_str(), -1, &stmt, NULL );
            // applique l'update
            sqlite3_step( stmt );
            sqlite3_finalize(stmt);
        }
        sqlite3_close(db_);
        sendSummaryMail();
        Wt::StandardButton answer = Wt::WMessageBox::show( WString::tr("saveACR.msg.titre"),
                                                           WString::tr("saveACR.msg.cont"),
                                                           Wt::StandardButton::Yes | Wt::StandardButton::No);
        if (answer == Wt::StandardButton::Yes){
            vider(0);
        } else {
            vider();
        }
    }
}

std::string formDesserteForest::format4SQL(std::string aString){
    boost::replace_all(aString,"'","\"");
    return aString;
}

void formDesserteForest::changeGeom(){
    std::string stgeom("LineString");
    switch (choixAM->currentIndex()) {
    case 0:
        stgeom="LineString";
        break;
    case 1:
        stgeom="Point";
        break;
    case 2:
        stgeom="Polygon";
        break;
    default:
        break;
    }
    std::string js ="map.removeInteraction(draw); draw = new ol.interaction.Draw({source: acr_src,type: '"+stgeom+"',}); acr_src.clear();map.addInteraction(draw);";
    doJavaScript(js);
    //std::cout << js << std::endl;
}

void formDesserteForest::vider(bool all){
    if(all){
        nom->setText("");
        prenom->setText("");
        mail->setText("");
        tel->setText("");
        typeContact->setCurrentIndex(0);
        contactPresicion->setText("");
        choixAM->setCurrentIndex(0);
        deposant->setCurrentIndex(0);
    }
    typeAM->setCurrentIndex(0);
    description->setText("");
    polygValid=0;
    aGeom="";
    doJavaScript("acr_src.clear();map.addInteraction(draw);");
    doJavaScript("document.body.scrollTop = 0; // For Safari");
    doJavaScript("document.documentElement.scrollTop = 0; // For Chrome, Firefox, IE and Opera");
}

void formDesserteForest::loadStyles(){
    std::shared_ptr<Wt::WBootstrap5Theme> theme = std::make_shared<Wt::WBootstrap5Theme>();
    setTheme(theme);
    useStyleSheet("resources/themes/default/wt.css");
    useStyleSheet("style/style.css");
    // init the OpenLayers javascript api
    require("jslib/v9.0.0-dist/ol.js");
    useStyleSheet("jslib/v9.0.0-dist/ol.css");
    require("jslib/proj4js-2.6.1/dist/proj4.js");
    require("jslib/proj4js-2.6.1/dist/proj4-src.js");
    enableUpdates();
}

void formDesserteForest::displayLayer(std::string aCode) {
    std::shared_ptr<layerBase>l= mDico->getLayerBase(aCode);
    std::string JScommand=std::string("activeLayer  = new ol.layer.Tile({")+
            "extent: extent,"+
            "title: 'MYTITLE',"+
            "source: new ol.source.TileWMS({"+
            "preload: Infinity,"+
            "title: 'MYTITLE',"+
            "url: 'MYURL',"+
            "crossOrigin: 'null',"+
            "attributions: 'MYATTRIBUTION',"+
            "params: {"+
            "'LAYERS': 'MYLAYER',"+
            "'TILED': false,"+
            //'TILED': false, // avant était à true mais ça faisait bugger cartoweb_topo
            "'FORMAT': 'image/png'"+
            "},"+
            "tileGrid: tileGrid,"+
            "projection: 'EPSG:31370',"+
            " }),"+
            " opacity: 1"+
            "});";

    if (l->mTypeWMS==TypeWMS::ArcGisRest){
        JScommand=std::string("activeLayer  = new ol.layer.Tile({")+
                "extent: extent,"+
                "title: 'MYTITLE',"+
                "  source: new ol.source.TileArcGISRest({"+
                "    attributions: 'MYATTRIBUTION',"+
                " url:'MYURL'"+
                "}),"+
                " opacity: 1"+
                "});";

    }

    JScommand+="activeLayers['MYCODE'] = activeLayer;updateGroupeLayers();";
    boost::replace_all(JScommand,"MYTITLE",l->getLegendLabel());
    boost::replace_all(JScommand,"MYLAYER",l->mWMSLayerName);
    boost::replace_all(JScommand,"MYURL",l->mUrl);
    boost::replace_all(JScommand,"MYATTRIBUTION",l->mWMSattribution);
    boost::replace_all(JScommand,"MYCODE",l->Code());
    doJavaScript(JScommand);
}

void formDesserteForest::displayCommune(){
    if (commune_->currentIndex()!=0){
        std::string nameFile=mDico->mCadastre->createPolygonCommune(aMLabelCom.at(commune_->currentIndex()));
        boost::filesystem::path p(nameFile);
        OGREnvelope env= computeGlobalGeom(nameFile);

        std::string JScommand=std::string("station = new ol.layer.Vector({")+
                "source: new ol.source.Vector({"+
                "format: new ol.format.GeoJSON(),"+
                "url: 'tmp/"+p.filename().c_str()+"'}),"+
                "style:new ol.style.Style({"+
                "stroke: new ol.style.Stroke({"+
                "color: 'blue',"+
                "width: 2})"+
                "  }),"+
                "extent: ["+std::to_string(env.MinX) + ","+std::to_string(env.MinY)+ ","+std::to_string(env.MaxX)+ ","+std::to_string(env.MaxY)+"],"+
                "});"+
                "updateGroupeLayers();"+
                "map.getView().fit(station.getExtent());" ;
        doJavaScript(JScommand);
    }
}

OGREnvelope formDesserteForest::computeGlobalGeom(std::string aFile){
    //std::cout << "computeGlobalGeom " << std::endl;
    OGREnvelope env= OGREnvelope();
    env.MinX =42247;
    env.MaxX = 295176;
    env.MinY = 21148;
    env.MaxY = 167719;

    const char *inputPath=aFile.c_str();
    GDALDataset * DS =  (GDALDataset*) GDALOpenEx( inputPath, GDAL_OF_VECTOR | GDAL_OF_READONLY, NULL, NULL, NULL );
    if( DS != NULL )
    {
        OGRLayer * lay = DS->GetLayer(0);
        // union de tout les polygones du shp
        OGRFeature *poFeature;
        OGRGeometry * poGeom;
        OGRGeometry * poGeom2;
        std::unique_ptr<OGRMultiPolygon> multi = std::make_unique<OGRMultiPolygon>();
        OGRErr err;
        OGRMultiPolygon *poGeomM;

        int nbValidPol(0);
        while( (poFeature = lay->GetNextFeature()) != NULL )
        {
            poFeature->GetGeometryRef()->flattenTo2D();
            switch (poFeature->GetGeometryRef()->getGeometryType()){
            case (wkbPolygon):
            {
                poGeom=poFeature->GetGeometryRef();
                poGeom->closeRings();
                poGeom = poGeom->Buffer(0.0);
                //poGeom->Simplify(1.0);
                err = multi->addGeometry(poGeom);
                if (err==OGRERR_NONE) nbValidPol++;
                break;
            }

            case wkbMultiPolygon:
            {
                poGeomM= poFeature->GetGeometryRef()->toMultiPolygon();
                int n(poGeomM->getNumGeometries());
                for (int i(0);i<n;i++){
                    poGeom=poGeomM->getGeometryRef(i);
                    poGeom->closeRings();
                    poGeom = poGeom->Buffer(0.0);
                    err = multi->addGeometry(poGeom);
                    if (err==OGRERR_NONE) nbValidPol++;
                }
                break;
            }
            default:
                std::cout << "Geometrie " << poFeature->GetFID() << ", type de geometrie non pris en charge ; " << poFeature->GetGeometryRef()->getGeometryName() << ", id " << poFeature->GetGeometryRef()->getGeometryType()<< std::endl;

                break;
            }
            if (err!=OGRERR_NONE){
                std::cout << "problem avec ajout de la geometrie " << poFeature->GetFID() << ", erreur : " << err <<  std::endl;
            }
            OGRFeature::DestroyFeature(poFeature);
        }
        if (nbValidPol>0){poGeom2 = multi->UnionCascaded();
            poGeom2 =poGeom2->Buffer(1.0);
            OGRGeometry * geomGlob = poGeom2->Simplify(1.0);
            geomGlob->getEnvelope(&env);
        }
    }else {
        std::cout << "computeGlobalGeom : je n'arrive pas à ouvrir " << aFile<< std::endl;
    }
    GDALClose(DS);
    return env;
}

void formDesserteForest::validDraw(std::string geojson){
    //std::cout << "valid draw \n" << geojson << std::endl;
    std::string name0 = std::tmpnam(nullptr);
    std::string name1 = name0.substr(5,name0.size()-5);
    std::string aOut = mDico->File("TMPDIR")+"/"+name1+".geojson";
    std::ofstream ofs (aOut, std::ofstream::out);
    ofs << geojson;
    ofs.close();
    // lecture avec gdal
    GDALDataset * DS =  (GDALDataset*) GDALOpenEx( aOut.c_str(), GDAL_OF_VECTOR | GDAL_OF_READONLY, NULL, NULL, NULL );
    if( DS != NULL )
    {
        OGRLayer * lay = DS->GetLayer(0);

        OGRFeature *poFeature;
        while( (poFeature = lay->GetNextFeature()) != NULL )
        {
            //surf=OGR_G_Area(poFeature->GetGeometryRef())/10000.0;
            //std::cout << "surface " << surf<< std::endl;
            polygValid=1;
            aGeom=poFeature->GetGeometryRef()->exportToJson();
            geom = poFeature->GetGeometryRef();
        }
        doJavaScript("map.removeInteraction(draw);");
    }else {
        std::cout << "validDraw : je n'arrive pas à ouvrir " << aOut<< std::endl;
    }
    GDALClose(DS);
}

void formDesserteForest::sendSummaryMail(){
    Wt::Mail::Message amail =Wt::Mail::Message();
    //mail.addHeader();
    //mail.setFrom(Wt::Mail::Mailbox("JO.Lisein@uliege.be", "Lisein Jonathan"));
    //amail.setFrom(Wt::Mail::Mailbox("v.colson@filiereboiswallonie.be","Vincent Colson"));

    amail.setFrom(Wt::Mail::Mailbox("louanne.collin@srfb-kbbm.be","Louanne Collin"));

    amail.setBody(
                "\nVos données ont été encodée.\n"+
                Wt::WString::tr("mail.contact").toUTF8()+"\n--------------------------\n"+
                nom->valueText().toUTF8()+"\n"+
                prenom->valueText().toUTF8()+"\n"+
                mail->valueText().toUTF8()+"\n"+
                tel->valueText().toUTF8()+"\n"+
                typeContact->valueText().toUTF8()+"\n\n"+
                contactPresicion->valueText().toUTF8()+"\n\n"+
                Wt::WString::tr("mail.acr").toUTF8()+"\n\n"+
                "type d'aménagement:"+typeAM->valueText().toUTF8()+"\n\n"+
                "Propriétaire:"+typeProprio->valueText().toUTF8()+"\n\n"+
                "Qui dépose le projet:"+deposant->valueText().toUTF8()+"\n\n"+
                "Description de l'aménagement:"+description->valueText().toUTF8()+"\n\n"+
                "Accompagnement administratif SRFB :"+acAdmin->valueText().toUTF8()+"\n\n"
                );
    amail.setSubject(Wt::WString::tr("mail.titre").toUTF8());
    amail.addRecipient(Wt::Mail::RecipientType::To,Mail::Mailbox(mail->valueText().toUTF8(),nom->valueText().toUTF8()) );

    // création de la figure de localisation
    std::ifstream in;
    if(1){ staticMap sm(mDico->getLayerBase("IGN"),geom);
    in.open(sm.getFileName(), std::ios::in);
    amail.addAttachment("image/png","localisationDesserte.png",&in);}
    Mail::Client client;
    client.connect();
    client.send(amail);
    in.close();// après l'envoi!! car le addAttachement pointe ver le ifstream!

    // mail confirmation encodage requete SQL
    amail =Wt::Mail::Message();
    amail.addRecipient(Wt::Mail::RecipientType::To,Wt::Mail::Mailbox("louanne.collin@srfb-kbbm.be","Louanne Collin"));
    //amail.addRecipient(Wt::Mail::RecipientType::To,Mail::Mailbox("v.colson@filiereboiswallonie.be","Vincent Colson"));
    amail.setFrom(Wt::Mail::Mailbox("JO.Lisein@uliege.be", "Lisein Jonathan"));
    amail.setSubject("Desserte Forestière - encodage");
    amail.setBody(SQLstring);
    client.send(amail);
}
