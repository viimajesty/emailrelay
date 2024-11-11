//  Hello World server
// compile command g++ -std=c++11 -o hello_world_server main.cpp -lzmqpp -lzmq
#include <zmqpp/zmqpp.hpp>
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
  return result;
}

std::string sendmail(const char *email, const char *messagetosend)
{
  const string command = "echo " + std::string(messagetosend) + " | msmtp " + std::string(email);
  return exec(command.c_str());
}

int main(int argc, char *argv[])
{
  // execute sendmail and print to console
  // std::cout << sendmail() << std::endl;

  const string endpoint = "tcp://*:5555";

  // initialize the 0MQ context
  zmqpp::context context;

  // generate a pull socket
  zmqpp::socket_type type = zmqpp::socket_type::reply;
  zmqpp::socket socket(context, type);

  // bind to the socket
  socket.bind(endpoint);

  zmqpp::message message;
  socket.receive(message);

  // Extract email and message text from the two frames
  string email, message_text;
  message >> email >> message_text;

  std::cout << "Email: " << email << std::endl;
  std::cout << "Message: " << message_text << std::endl;

  // Send email
  sendmail(email.c_str(), message_text.c_str());
}