#include <tools/tools.hpp>

namespace tools
{
    Wt::Mail::Message createMail(
        const std::string &from,
        const std::string &fromName,
        const std::string &to,
        const std::string &subject,
        const std::string &body)
    {
        Wt::Mail::Message mail;
        mail.setFrom(Wt::Mail::Mailbox(from, fromName));
        mail.addRecipient(Wt::Mail::RecipientType::To, Wt::Mail::Mailbox(to));
        mail.setSubject(subject);
        mail.setBody(body);
        return mail;
    }

    bool sendMail(const Wt::Mail::Message &mail, bool useForestimatorDomain)
    {
        ifstream configFile("./forestimator.cfg");
        string lg = "";
        string pw = "";
        if (configFile.is_open())
        {
            getline(configFile, lg);
            getline(configFile, pw);
        }
        else
        {
            cout << "Error: Unable to open configMailAccount.txt" << endl;
            cout << "Mail not sent!" << endl;
        }

        Wt::Mail::Client client;
        client.enableAuthentication(lg, pw, Wt::Mail::AuthenticationMethod::Plain);
        client.setTransportEncryption(
            useForestimatorDomain 
            ? Wt::Mail::TransportEncryption::None 
            : Wt::Mail::TransportEncryption::TLS
        );
        client.setSslCertificateVerificationEnabled(true);
        try
        {
            useForestimatorDomain
                ? client.connect("smtp.ulg.ac.be", 25)
                : client.connect("smtp.ulg.ac.be", 465);
            client.send(mail);
            return true;
        }
        catch (const std::exception &e)
        {
            cerr << "Error sending email: " << e.what() << std::endl;
            return false;
        }
    }
}