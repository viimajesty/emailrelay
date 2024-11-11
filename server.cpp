#include <httplib.h>
#include <string>
#include <iostream>
#include <chrono>
#include <thread>
#include <memory>
#include <stdexcept>
#include <array>

using namespace std;

std::string exec(const char *cmd)
{
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe)
    {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr)
    {
        result += buffer.data();
    }
    std::cout << "test1" << std:: endl;
    std::cout << result.c_str() << std::endl;
    return result;
}

std::string sendmail(const char *email, const char *messagetosend, const char *messagesubject)
{
    const string command = "echo $'Subject: " + std::string(messagesubject) + "\\n\\n" + std::string(messagetosend) + "' | msmtp --from=test7@gmail.com --tls=on --tls-trust-file=/etc/ssl/cert.pem --auth=on --user=test7@gmail.com --passwordeval='echo passwordhere' --host=smtp.gmail.com --port=587 " + std::string(email);

    std::string result = exec(command.c_str());

    // Check for authentication error message
    if (result.find("authentication failed (method PLAIN)") != std::string::npos ||
        result.find("535-5.7.8 Username and Password not accepted") != std::string::npos)
    {
        return "res error wrong password";
    }

    return result;
}

int main()
{
    httplib::Server svr;

    svr.Post("/sendmail", [](const httplib::Request &req, httplib::Response &res)
             {
        auto email = req.get_param_value("email");
        auto message_text = req.get_param_value("message");
        auto subject = req.get_param_value("subject");


        std::cout << email + " " + message_text << std::endl;
        sendmail(email.c_str(), message_text.c_str(), subject.c_str());
        res.set_content("Email sent", "text/plain"); });

    svr.listen("0.0.0.0", 5555); // or your desired port
}
