#include <httplib.h>
#include <string>
#include <iostream>
#include <chrono>
#include <thread>
#include <memory>
#include <stdexcept>
#include <array>
#include <tuple>
#include <pstream.h>
#include <gpgme.h>
#include <locale.h>
#include <fstream>
#include "Base64.h"
#include "dotenv.h"
#include <pstream.h>

using namespace std;

std::string exec(std::string command)
{
    std::string res;
    // run a process and create a streambuf that reads its stdout and stderr
    redi::ipstream proc(command, redi::pstreams::pstdout | redi::pstreams::pstderr);
    std::string line;
    // read child's stdout
    while (std::getline(proc.out(), line))
    {
        std::cout << "stdout: " << line << '\n';
        res.append(line + "\n");
    }
    // if reading stdout stopped at EOF then reset the state:
    if (proc.eof() && proc.fail())
    {
        proc.clear();
    }
    // read child's stderr
    while (std::getline(proc.err(), line))
    {
        std::cout << "stderr: " << line << '\n';
        res.append(line + "\n");
    }
    return res;
}

void init_gpgme(void)
{
    setlocale(LC_ALL, "en_US.UTF-8");
    gpgme_set_engine_info(GPGME_PROTOCOL_OpenPGP, "/usr/local/bin/gpg", NULL);
    gpgme_check_version(NULL);
    gpgme_set_locale(NULL, LC_CTYPE, setlocale(LC_CTYPE, NULL));
#ifdef LC_MESSAGES
    gpgme_set_locale(NULL, LC_MESSAGES, setlocale(LC_MESSAGES, NULL));
#endif
}

bool is_file_readable(const char *filename)
{
    std::ifstream file(filename);
    return file.good();
}

std::tuple<int, std::string> decrypt(const char *encrypted_file)
{
    //     std::cout << encrypted_file << std::endl;
    // Ensure the buffer is not null and has data
    if (encrypted_file == nullptr || *encrypted_file == '\0')
    {
        std::cerr << "Error: Encrypted message is empty!" << std::endl;
        return std::make_tuple(1, "Encrypted message is empty!");
    }

    // Calculate the length of the encrypted file content correctly
    size_t cipher_size = strlen(encrypted_file); // Size of the encrypted file string (ensure it's valid)

    // Ensure non-zero length
    if (cipher_size == 0)
    {
        std::cerr << "Error: Encrypted message has no data!" << std::endl;
        return std::make_tuple(1, "Encrypted message has no data!");
    }

    // Create GPGME context
    gpgme_ctx_t ctx;
    gpgme_error_t err = gpgme_new(&ctx);
    if (err)
    {
        std::cerr << "Error creating GPGME context: " << gpgme_strerror(err) << std::endl;
        return std::make_tuple(1, gpgme_strerror(err));
    }

    // Create gpgme_data_t object for encrypted data from the file contents
    // convert encrypted_file into buffer
    gpgme_data_t cipher_data;
    err = gpgme_data_new_from_mem(&cipher_data, encrypted_file, cipher_size, 1); // 0 means no copy
    if (err)
    {
        std::cerr << "Error reading encrypted data: " << gpgme_strerror(err) << std::endl;
        gpgme_release(ctx);
        return std::make_tuple(1, gpgme_strerror(err));
    }

    // Create empty gpgme_data_t object for decrypted output
    gpgme_data_t plain_data;
    err = gpgme_data_new(&plain_data);
    if (err)
    {
        std::cerr << "Error creating plain data buffer: " << gpgme_strerror(err) << std::endl;
        gpgme_data_release(cipher_data);
        gpgme_release(ctx);
        return std::make_tuple(1, gpgme_strerror(err));
    }

    // Decrypt the ciphertext
    err = gpgme_op_decrypt(ctx, cipher_data, plain_data);
    if (err)
    {
        std::cerr << "Error decrypting message: " << gpgme_strerror(err) << std::endl;
        gpgme_data_release(cipher_data);
        gpgme_data_release(plain_data);
        gpgme_release(ctx);
        return std::make_tuple(1, gpgme_strerror(err));
    }

    // Check if the decrypted data has any content
    size_t plain_size = gpgme_data_seek(plain_data, 0, SEEK_END);
    gpgme_data_seek(plain_data, 0, SEEK_SET);
    std::cout << "Decrypted text size: " << plain_size << " bytes" << std::endl;

    std::string decrypted_text = "";
    if (plain_size == 0)
    {
        std::cerr << "Error: Decrypted text is empty!" << std::endl;
    }
    else
    {
        // Output the decrypted content
        char buffer[1024];
        size_t bytes_read;
        while ((bytes_read = gpgme_data_read(plain_data, buffer, sizeof(buffer))) > 0)
        {
            decrypted_text.append(buffer, bytes_read);
            // std::cout.write(buffer, bytes_read);
        }
    }

    // Clean up
    gpgme_data_release(cipher_data);
    gpgme_data_release(plain_data);
    gpgme_release(ctx);

    return std::make_tuple(0, decrypted_text);
}

int importKeys()
{
    init_gpgme();

    const char *private_key_file = std::getenv("PRIVATEKEYPATH"); // Path to private key file
    if (!is_file_readable(private_key_file))
    {
        std::cerr << "Error: File '" << private_key_file << "' is not readable!" << std::endl;
        return 1;
    }

    // Create GPGME context
    gpgme_ctx_t ctx;
    gpgme_error_t err = gpgme_new(&ctx);
    if (err)
    {
        std::cerr << "Error creating GPGME context: " << gpgme_strerror(err) << std::endl;
        return 1;
    }

    // Read private key from file
    gpgme_data_t private_key_data;
    err = gpgme_data_new_from_file(&private_key_data, private_key_file, 1); // Copy = 1
    if (err)
    {
        std::cerr << "Error reading private key file: " << gpgme_strerror(err) << std::endl;
        gpgme_release(ctx);
        return 1;
    }

    // Import the private key
    err = gpgme_op_import(ctx, private_key_data);
    if (err)
    {
        std::cerr << "Error importing private key: " << gpgme_strerror(err) << std::endl;
    }
    else
    {
        gpgme_import_result_t import_result = gpgme_op_import_result(ctx);
        if (import_result)
        {
            std::cout << "Imported Secret Keys: " << import_result->secret_imported << std::endl;
            std::cout << "Not Imported: " << import_result->not_imported << std::endl;

            gpgme_import_status_t import_status = import_result->imports;
            while (import_status)
            {
                std::cout << "Key ID: " << (import_status->fpr ? import_status->fpr : "Unknown") << std::endl;
                if (import_status->status)
                {
                    std::cerr << "Status: " << gpgme_strerror(import_status->status) << std::endl;
                }
                import_status = import_status->next;
            }
        }
    }
    gpgme_data_release(private_key_data);
    gpgme_release(ctx);

    return 0;
}

string sendmail(std::string &email, std::string &messagetosend, std::string &messagesubject, std::string &encpassword, std::string &sendfrom)
{
    std::string failure = "";
    // cout << encpassword << endl;
    std::string out;
    auto error = macaron::Base64::Decode(encpassword, out);
    if (!error.empty())
    {
        std::cout << "Error: " << error << std::endl;
    }
    std::cout << out << std::endl;

    auto passwordtupl = decrypt(out.c_str());
    std::string password = "";
    if (std::get<0>(passwordtupl) == 0)
    {
        // std::cout << std::get<1>(passwordtupl) << std::endl;
        password = std::get<1>(passwordtupl);
    }
    else
    {
        failure = std::get<1>(passwordtupl);
        return failure;
    }
    if (email.empty() || messagetosend.empty() || messagesubject.empty() || password.empty() || email.find("example.com") != std::string::npos || email.find("@") == std::string::npos || sendfrom.empty())
    {
        failure = "Bad request. Please check all input parameters. ";
        return failure;
    }
    const string command = "echo $'Subject: " + std::string(messagesubject) + "\\n\\n" + std::string(messagetosend) + "' | msmtp --from=" + std::string(sendfrom) + " --tls=on --tls-trust-file=/etc/ssl/cert.pem --auth=on --user=" + std::string(sendfrom) + " --passwordeval='echo " + std::string(password) + " ' --host=smtp.gmail.com --port=587 " + std::string(email);
    redi::ipstream proc(command, redi::pstreams::pstdout | redi::pstreams::pstderr);
    std::string line;
    // read child's stdout
    while (std::getline(proc.out(), line))
        std::cout << "stdout: " << line << '\n';
    // if reading stdout stopped at EOF then reset the state:
    if (proc.eof() && proc.fail())
        proc.clear();
    // read child's stderr
    while (std::getline(proc.err(), line))
    {
        std::cout << "stderr: " << line << '\n';
        failure.append(line);
    }

    if (failure == "")
    {
        failure = "message sent successfully.";
    }
    return failure;
}

int main()
{
    dotenv::init();
    int failure0 = importKeys();
    if (failure0 != 0)
    {
        std::cout << "fatal: error importing keys." << std::endl;
        return 1;
    }
    httplib::Server svr;

    svr.Post("/sendmail", [](const httplib::Request &req, httplib::Response &res)
             {
        auto email = req.get_param_value("email");
        auto message_text = req.get_param_value("message");
        auto subject = req.get_param_value("subject");
        auto password = req.get_param_value("password");
        auto sendfrom = req.get_param_value("from");

       // std::cout << email + " " + message_text + " " + subject + " " + password << std::endl;
        string result = sendmail(email, message_text, subject, password, sendfrom);
        res.set_content("response from server: " + result, "text/plain"); });

    svr.listen("0.0.0.0", 5555); // or your desired port
}
