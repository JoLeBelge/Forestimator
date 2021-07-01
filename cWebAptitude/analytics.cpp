#include "analytics.h"






Analytics::Analytics() : session()
{
    //auto sqlite3 = Wt::cpp14::make_unique<dbo::backend::Sqlite3>(sqliteDb);
    auto sqlite3 = Wt::cpp14::make_unique<dbo::backend::Sqlite3>("/data1/Forestimator/data/analytics.db");
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

void Analytics::addLog(const Wt::WEnvironment& env, int user_id){

    dbo::Transaction transaction{session};

    std::unique_ptr<Log> log{new Log()};
    log->datum = time(0);
    log->ip = env.clientAddress();
    log->client = env.userAgent();
    log->ipath = env.internalPath();
    log->id_user = user_id;

    dbo::ptr<Log> logPtr = session.add(std::move(log));

}
