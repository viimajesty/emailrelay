#include <zmqpp/zmqpp.hpp>
#include <string>
#include <iostream>

using namespace std;

int main(int argc, char *argv[])
{
    const string endpoint = "tcp://localhost:5555";

    // initialize the 0MQ context
    zmqpp::context context;

    // generate a request socket
    zmqpp::socket_type type = zmqpp::socket_type::request;
    zmqpp::socket socket(context, type);

    // open the connection
    cout << "Connecting to hello world server…" << endl;
    socket.connect(endpoint);

    // Compose the message to send (email and message text)
    string email = "me@vivekkadre.com";
    string message_text = "Hello from the client!";

    // Prepare and send the two-part message (email and message text)
    zmqpp::message message;
    message << email << message_text;
    socket.send(message);

    // Receive the response from the server (optional)
    zmqpp::message reply;
    socket.receive(reply);
    string response;
    reply >> response;
    cout << "Server response: " << response << endl;

    return 0;
}
