#include "analytics.h"






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

void Analytics::addLog(const Wt::WEnvironment &env, int user_id, std::string page){

    dbo::Transaction transaction{session};

    std::unique_ptr<Log> log{new Log()};
    log->datum = time(0);
    log->ip = env.clientAddress();
    log->client = env.userAgent();
    log->ipath = page;
    log->id_user = user_id;

    //std::cout << log->datum << log->client << log->ipath << std::endl;

    dbo::ptr<Log> logPtr = session.add(std::move(log));

}
