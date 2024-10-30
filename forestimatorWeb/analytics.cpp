#include "analytics.h"

extern bool globTest;

Analytics::Analytics(std::string aFileDB) : session()
{

    auto sqlite3 = std::make_unique<dbo::backend::Sqlite3>(aFileDB);
    sqlite3->setProperty("show-queries", "false");

    session.setConnection(std::move(sqlite3));
    session.mapClass<Log>("log");

    try {
        session.createTables();
        std::cout << "Created analytics database." << std::endl;
    } catch (std::exception& e) {
        //std::cout << "Using existing analytics database...";
    }

}


void Analytics::addLog(const Wt::WEnvironment &env, int user_id, std::string page, int cat){

    if(env.agentIsSpiderBot())return;

    dbo::Transaction transaction{session};

    std::unique_ptr<Log> log{new Log()};
    log->datum = time(0);
    log->date = Wt::WDate::currentDate();
    log->ip = env.clientAddress();
    log->client = env.userAgent();
    log->ipath = page;
    log->id_user = user_id;
    log->categorie = cat;

    //std::cout << log->datum << log->client << log->ipath << std::endl;

    addLogApache(env,page);

    session.add(std::move(log));
}

void Analytics::addLogApache(const Wt::WEnvironment &env, std::string page){
    time_t timestamp=time(0);
    struct tm * timeinfo = localtime(&timestamp);
    char buffer[80];
    strftime(buffer,sizeof(buffer),"%d/%b/%Y:%H:%M:%S",timeinfo);
    std::string str(buffer);

    std::ofstream out("log.txt", std::ios::app);
    out << env.clientAddress();
    out <<  " - - [";
    out << str;
    out << " +0100] \"GET " << page ;
    out << " HTTP/1.1\" 200 1000 \"-\" \"" << env.userAgent() << "\"\n";
}

bool Analytics::logExist(const Wt::WEnvironment &env, std::string page, typeLog cat){
    bool aRes(1);

    dbo::Transaction transaction{session};

    int c=session.query<int>("select count(1) from Log").where("date = ?").bind(Wt::WDate::currentDate().toString("yyyy-MM-dd")).where("ipath = ?").bind(page).where("ip = ?").bind(env.clientAddress()).where("cat = ?").bind((int (cat)));
    if (c==0){
        aRes=0;
    } else{
        // log brut data anyway
        addLogApache(env,page);
        //if (globTest){std::cout << " le log existe déjà pour cet utilisateur !" << std::endl;}
    }
    return aRes;
}


PageAnalytics::PageAnalytics(const Wt::WEnvironment& env, std::string aFileDB) : Wt::WApplication(env),
    session()
{
    messageResourceBundle().use(docRoot() + "/forestimator");
    messageResourceBundle().use(docRoot() + "/forestimator-documentation");

    auto sqlite3 = std::make_unique<dbo::backend::Sqlite3>(aFileDB);
    sqlite3->setProperty("show-queries", "false");

    session.setConnection(std::move(sqlite3));
    session.mapClass<Log>("log");
    if (globTest){std::cout << " statistiques brutes : début de transaction avec la BD analytics " << std::endl;}
    dbo::Transaction transaction{session};
    if (globTest){std::cout << " done " << std::endl;}
    typedef dbo::collection< dbo::ptr<Log> > Logs;

    // probablement plus simple de faire une selection sur base de datum (integer) que de date (texte puis date)
    /*time_t timeLim;
    struct tm tA, *tptr;
    time(&timeLim);
    tptr = localtime(&timeLim);
    tA = *tptr;
    tA.tm_mday = dateLim.day();
    tA.tm_mon = dateLim.month();
    tA.tm_year = dateLim.year()-1900;// time.h compte les années à partir de 1900.
    timeLim = mktime(&tA);*/

    //Logs logsGraph = session.find<Log>().where("datum > "+std::to_string(timeLim));
    //Logs logs = session.find<Log>().orderBy("datum DESC").limit(100);
    //std::cout << " date limite : " << dateLim.toString() << ", date ajd " << curDate.toString() << " en time_t ça donne " << timeLim << std::endl;
    setTitle("Forestimator - Stats");

    root()->setMargin(0);
    root()->setPadding(0);
    root()->setOverflow(Wt::Overflow::Scroll);


    /* nbMonthSelection_  =layout->addWidget(std::make_unique<Wt::WComboBox>());
    for (int c(0);c<5;c++){
        nbMonthSelection_->addItem(std::to_string(1+c*3));
    }
    nbMonthSelection_->changed().connect(std::bind(&PageAnalytics::changeGraph,this));*/

    //Wt::WContainerWidget * contentChart = layout->addWidget(std::make_unique<Wt::WContainerWidget>(),0);
    //mChart = layout->addWidget(std::make_unique<Chart::WCartesianChart>());
    mChart = root()->addWidget(std::make_unique<Chart::WCartesianChart>());


    // ok alors je sais pas trop pourquoi mais une fois que j'ai fini cette boucle je ne peux plus en refaire une autre. L'objet logsGraph est vide par après...
    // for (const dbo::ptr<Log> &log : logsGraph){
    // je pourrais aussi boucler sur les jours et refaire une query pour compter le nombre de logs indépendamment pour chaque jours!
    // ici si un logs dans la db a un identifiant NULL (si on a oublié de coché "auto-increment" pour colonne id - on se retrouve avec un déréférencement.
    //time_t t=time(0);
    model = std::make_shared<WStandardItemModel>();

    mChart->resize(800, 500);
    mChart->setMinimumSize(800, 500);
    mChart->setMargin(20, Side::Top | Side::Bottom);
    mChart->setMargin(50, Side::Left | Side::Right);

    mChart->setModel(model);
    mChart->setXSeriesColumn(0);
    mChart->setLegendEnabled(true);
    mChart->setType(Chart::ChartType::Scatter);
    mChart->axis(Chart::Axis::X).setScale(Chart::AxisScale::Date);

    //std::cout << "PageAnalytics::setChart " << std::endl;
    Wt::WDate curDate=Wt::WDate::currentDate();
    Wt::WDate dateLim=curDate.addMonths(-3);
    int nbd=dateLim.daysTo(curDate);

    // Configure the header.
    model->insertColumns(model->columnCount(), 2);
    // Set data in the model.
    model->insertRows(model->rowCount(), nbd);
    int row = 0;
    for (int d(0);d<nbd+1;d++){
        Wt::WDate ad=dateLim.addDays(d);
        //int count = session.query<int>("select count(1) from log").where("date = ?").bind(ad).groupBy("ip");
        int count = session.query<int>("SELECT COUNT(*) AS col0 FROM (SELECT DISTINCT ip, date FROM log)  where (date = '"+ad.toString("yyyy-MM-dd").toUTF8()+"')");
        model->setData(row, 0, ad);
        model->setData(row, 1, count);
        row++;
    }

    /*
     * Add the first column as line series.
     *  */
    auto s = std::make_unique<Chart::WDataSeries>(1, Chart::SeriesType::Line);
    s->setShadow(WShadow(3, 3, WColor(0, 0, 0, 127), 3));
    mChart->addSeries(std::move(s));

    // tableau de synthèse
    Wt::WContainerWidget * content2 = root()->addWidget(std::make_unique<Wt::WContainerWidget>());
    content2->setOverflow(Wt::Overflow::Scroll);
    content2->addNew<Wt::WText>(Wt::WText::tr("analytic.tab1"));
    auto table2 = content2->addWidget(std::make_unique<Wt::WTable>());
    table2->setHeaderCount(1);
    table2->setWidth(Wt::WLength("100%"));
    table2->toggleStyleClass("table-striped",true);
    table2->elementAt(0, 0)->addNew<Wt::WText>("Categorie");
    table2->elementAt(0, 1)->addNew<Wt::WText>("Nombre de consultation");

    //0 page,extend,danap,anas,dsingle,dmulti,danas,dsingleRW;
    // Sélection par catégorie de log
   // std::string q="SELECT COUNT(*)as nb, cat FROM log  WHERE ip != '127.0.0.1' AND ip NOT LIKE '%139.165%' GROUP BY cat;";
    for (int cat(1);cat <10;cat++){
        int nb=session.query<int>("SELECT COUNT(*) FROM log  WHERE ip != '127.0.0.1' AND ip NOT LIKE '%139.165.%' AND cat="+std::to_string(cat)+" GROUP BY cat");
        table2->elementAt(cat,0)->addWidget(std::make_unique<Wt::WText>(getCat(cat)));
        table2->elementAt(cat,0)->setContentAlignment(AlignmentFlag::Right);
        table2->elementAt(cat,1)->addWidget(std::make_unique<Wt::WText>(std::to_string(nb)));
        table2->elementAt(cat,1)->setContentAlignment(AlignmentFlag::Center);
    }

    content2->addNew<Wt::WText>("tableau de synthèse (utilisation via réseau de l'Ulg) :");
    auto table3 = content2->addWidget(std::make_unique<Wt::WTable>());
    table3->setHeaderCount(1);
    table3->setWidth(Wt::WLength("100%"));
    table3->toggleStyleClass("table-striped",true);
    table3->elementAt(0, 0)->addNew<Wt::WText>("Categorie");
    table3->elementAt(0, 1)->addNew<Wt::WText>("Nombre de consultation");

    //0 page,extend,danap,anas,dsingle,dmulti,danas,dsingleRW;
    // connexion par wifi depuis Ulg : donne l'IP 127.0.0.1
    // Sélection par catégorie de log
    // q="SELECT COUNT(*)as nb, cat FROM log  WHERE ip != '127.0.0.1' AND ip LIKE '%139.165%' GROUP BY cat;";
    for (int cat(1);cat <10;cat++){
        int nb=session.query<int>("SELECT COUNT(*) FROM log  WHERE ip != '127.0.0.1' AND ip LIKE '%139.165.%' AND cat="+std::to_string(cat)+" GROUP BY cat");
        table3->elementAt(cat,0)->addWidget(std::make_unique<Wt::WText>(getCat(cat)));
        table3->elementAt(cat,0)->setContentAlignment(AlignmentFlag::Right);
        table3->elementAt(cat,1)->addWidget(std::make_unique<Wt::WText>(std::to_string(nb)));
         table3->elementAt(cat,1)->setContentAlignment(AlignmentFlag::Center);
    }


    content2->addNew<Wt::WText>(Wt::WText::tr("analytic.tabUser"));
    auto table4 = content2->addWidget(std::make_unique<Wt::WTable>());
    table4->setHeaderCount(1);
    table4->setWidth(Wt::WLength("100%"));
    table4->toggleStyleClass("table-striped",true);
    table4->elementAt(0, 0)->addNew<Wt::WText>("année et mois");
    table4->elementAt(0, 1)->addNew<Wt::WText>("Nombre d'utilisateur");

    //q="SELECT COUNT(*)as nb FROM (SELECT COUNT(*)as nb FROM log  WHERE ip != '127.0.0.1' AND ip NOT LIKE '%139.165%' AND date LIKE '%2022-02%' GROUP BY ip);";
    row=1;
    for (int y(2022);y <2025;y++){
    for (int m(1);m <13;m++){
        std::string month = std::to_string(m);
        if (month.size()==1){month="0"+ month;}

        int nb=session.query<int>("SELECT COUNT(*) as nb FROM (SELECT COUNT(*) as nb FROM log  WHERE ip != '127.0.0.1' AND ip NOT LIKE '%139.165%' AND date LIKE '%"+std::to_string(y)+"-"+month+"%' GROUP BY ip)");
        table4->elementAt(row,0)->addWidget(Wt::cpp14::make_unique<Wt::WText>(month+"-"+std::to_string(y)));
        table4->elementAt(row,0)->setContentAlignment(AlignmentFlag::Right);
        table4->elementAt(row,1)->addWidget(Wt::cpp14::make_unique<Wt::WText>(std::to_string(nb)));
        table4->elementAt(row,1)->setContentAlignment(AlignmentFlag::Center);
        row++;
    }
    
    }

    // tableau brut des 100 derniers logs
    Wt::WContainerWidget * content = root()->addWidget(std::make_unique<Wt::WContainerWidget>());
    content->setOverflow(Wt::Overflow::Scroll);
    content->addNew<Wt::WText>(WText::tr("analytic.rawData"));
    auto table = content->addWidget(std::make_unique<Wt::WTable>());
    table->setHeaderCount(1);
    table->setWidth(Wt::WLength("100%"));
    table->toggleStyleClass("table-striped",true);
    table->elementAt(0, 0)->addNew<Wt::WText>("Numéro");
    table->elementAt(0, 1)->addNew<Wt::WText>("Date");
    table->elementAt(0, 2)->addNew<Wt::WText>("IP");
    table->elementAt(0, 3)->addNew<Wt::WText>("Client");
    table->elementAt(0, 4)->addNew<Wt::WText>("Page");
    table->elementAt(0, 5)->addNew<Wt::WText>("Categorie");

    Logs logs = session.find<Log>().orderBy("datum DESC").limit(100);
    int i=1;
    for (const dbo::ptr<Log> &log : logs){
        // ici si un logs dans la db a un identifiant NULL (si on a oublié de coché "auto-increment" pour colonne id - on se retrouve avec un déréférencement.
        if (log.get()!=nullptr){
            //if (globTest){std::cout << " log " << log->datum << " : " << log->ip << " : " << log->ipath << std::endl;}
            time_t now=log->datum;
            //tm *ltm = localtime(&now);
            table->elementAt(i,0)->addWidget(std::make_unique<Wt::WText>(std::to_string(i)));
            table->elementAt(i,1)->addWidget(std::make_unique<Wt::WText>(ctime(&now)));
            table->elementAt(i,2)->addWidget(std::make_unique<Wt::WText>(log->ip));
            table->elementAt(i,3)->addWidget(std::make_unique<Wt::WText>(log->client));
            table->elementAt(i,4)->addWidget(std::make_unique<Wt::WText>(log->ipath));
            table->elementAt(i,5)->addWidget(std::make_unique<Wt::WText>(log->getCat()));
            i++;
        }

    }

}
