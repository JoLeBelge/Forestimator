#include "analytics.h"

extern bool globTest;

Analytics::Analytics(std::string aFileDB) : session()
{
    //auto sqlite3 = Wt::cpp14::make_unique<dbo::backend::Sqlite3>(sqliteDb);
    //auto sqlite3 = Wt::cpp14::make_unique<dbo::backend::Sqlite3>("/data1/Forestimator/data/analytics.db");
    auto sqlite3 = Wt::cpp14::make_unique<dbo::backend::Sqlite3>(aFileDB);
    sqlite3->setProperty("show-queries", "false");

    session.setConnection(std::move(sqlite3));
    session.mapClass<Log>("log");

    try {
        session.createTables();
        std::cout << "Created analytics database." << std::endl;
    } catch (std::exception& e) {
        std::cout << "Using existing analytics database...";
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

    /*dbo::ptr<Log> logPtr = session.find<Log>().where("date = ?").bind(Wt::WDate::currentDate().toString("yyyy-MM-dd")).where("ipath = ?").bind(page).where("ip = ?").bind(env.clientAddress());
        if (logPtr.get()==nullptr){aRes=0;} else{
         if (globTest){std::cout << " le log existe déjà pour cet utilisateur !" << std::endl;}
        }
        */

    int c=session.query<int>("select count(1) from Log").where("date = ?").bind(Wt::WDate::currentDate().toString("yyyy-MM-dd")).where("ipath = ?").bind(page).where("ip = ?").bind(env.clientAddress()).where("cat = ?").bind((int (cat)));
    if (c==0){
        aRes=0;
    } else{
        // log brut data anyway
        addLogApache(env,page);
        if (globTest){std::cout << " le log existe déjà pour cet utilisateur !" << std::endl;}
    }
    return aRes;
}


PageAnalytics::PageAnalytics(const Wt::WEnvironment& env, std::string aFileDB) : Wt::WApplication(env),
    session()
{
    auto sqlite3 = Wt::cpp14::make_unique<dbo::backend::Sqlite3>(aFileDB);
    sqlite3->setProperty("show-queries", "false");
    //sqlite3->setProperty("show-queries", "true");
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


    auto layout = root()->setLayout(Wt::cpp14::make_unique<Wt::WVBoxLayout>());
    root()->setMargin(0);
    root()->setPadding(0);

   /* nbMonthSelection_  =layout->addWidget(std::make_unique<Wt::WComboBox>());
    for (int c(0);c<5;c++){
        nbMonthSelection_->addItem(std::to_string(1+c*3));
    }
    nbMonthSelection_->changed().connect(std::bind(&PageAnalytics::changeGraph,this));*/

    mChart = layout->addWidget(cpp14::make_unique<Chart::WCartesianChart>());

    // ok alors je sais pas trop pourquoi mais une fois que j'ai fini cette boucle je ne peux plus en refaire une autre. L'objet logsGraph est vide par après...
    // for (const dbo::ptr<Log> &log : logsGraph){
    // je pourrais aussi boucler sur les jours et refaire une query pour compter le nombre de logs indépendamment pour chaque jours!
        // ici si un logs dans la db a un identifiant NULL (si on a oublié de coché "auto-increment" pour colonne id - on se retrouve avec un déréférencement.
    //time_t t=time(0);
    model = std::make_shared<WStandardItemModel>();

    mChart->resize(800, 500);    // WPaintedWidget must be given an explicit size.
    mChart->setMinimumSize(800, 500);
    mChart->setMargin(20, Side::Top | Side::Bottom); // Add margin vertically.
    //aChart->setMargin(WLength::Auto, Side::Left | Side::Right); // Center horizontally. il faut mettre des marges, qui sont comtpée au départ du cammembert, pour mettre les label
    mChart->setMargin(50, Side::Left | Side::Right);

    mChart->setModel(model);
    mChart->setXSeriesColumn(0);
    mChart->setLegendEnabled(true);
    mChart->setType(Chart::ChartType::Scatter);
    mChart->axis(Chart::Axis::X).setScale(Chart::AxisScale::Date);

    //setChart(3);

    std::cout << "PageAnalytics::setChart " << std::endl;
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
            model->setData(row, 1, count);// je met row à la place du nombre de vue pour l'instant
            row++;
    }

    /*
     * Add the first column as line series.
     *  */
    auto s = cpp14::make_unique<Chart::WDataSeries>(1, Chart::SeriesType::Line);
    s->setShadow(WShadow(3, 3, WColor(0, 0, 0, 127), 3));
    mChart->addSeries(std::move(s));

    // tableau brut des 100 derniers logs
    Wt::WContainerWidget * content = layout->addWidget(Wt::cpp14::make_unique<Wt::WContainerWidget>());
    content->setOverflow(Wt::Overflow::Scroll);
    content->addNew<Wt::WText>("Dernieres stats brutes :");

    auto table = content->addWidget(Wt::cpp14::make_unique<Wt::WTable>());
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
            table->elementAt(i,0)->addWidget(Wt::cpp14::make_unique<Wt::WText>(std::to_string(i)));
            table->elementAt(i,1)->addWidget(Wt::cpp14::make_unique<Wt::WText>(ctime(&now)));
            table->elementAt(i,2)->addWidget(Wt::cpp14::make_unique<Wt::WText>(log->ip));
            table->elementAt(i,3)->addWidget(Wt::cpp14::make_unique<Wt::WText>(log->client));
            table->elementAt(i,4)->addWidget(Wt::cpp14::make_unique<Wt::WText>(log->ipath));
            table->elementAt(i,5)->addWidget(Wt::cpp14::make_unique<Wt::WText>(log->getCat()));

            i++;
        }

    }

}

void PageAnalytics::setChart(int nbMonth){

    // on dirait pas que je puisse facilement changer l'entièretée du model, genre le vider des données et ensuite le reremplir..




}
