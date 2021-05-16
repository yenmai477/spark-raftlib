//For raft
#include <raft>
#include <iostream>

//For socket
#include <stdio.h>      // standard input and output library
#include <stdlib.h>     // this includes functions regarding memory allocation
#include <string.h>     // contains string functions
#include <errno.h>      //It defines macros for reporting and retrieving error conditions through error codes
#include <time.h>       //contains various functions for manipulating date and time
#include <unistd.h>     //contains various constants
#include <sys/types.h>  //contains a number of basic derived types that should be used whenever appropriate
#include <arpa/inet.h>  // defines in_addr structure
#include <sys/socket.h> // for socket creation
#include <netinet/in.h> //contains constants and structures needed for internet domain addresses
#include <string>

//For string, split string
#include <fstream>
#include <vector>

using namespace std;

/**
 * Global Variable
 */
int clintListn = 0, clintConnt = 0;
struct sockaddr_in ipOfServer;

//Read string from file
string readInputFile(string path)
{
    string line;
    ifstream myfile(path);
    if (myfile.is_open())
    {
        string newLine = "";
        while (getline(myfile, newLine))
        {
            line += newLine + "\n";
        }
        myfile.close();
    }
    return line;
}

//Split string into pieces
vector<string> splitString(string input, string delimiter)
{
    vector<string> pieces;

    size_t pos = 0;
    string token;
    while ((pos = input.find(delimiter)) != string::npos)
    {
        token = input.substr(0, pos);
        pieces.push_back(token);
        input.erase(0, pos + delimiter.length());
    }

    return pieces;
}

/**
 * Producer: sends data
 */
class producer : public raft::kernel
{
private:
    int i = 0;
    string inputFile = "Hello World";
    vector<string> inputFilePieces;

public:
    producer() : raft::kernel()
    {
        cout << ">> Constructer of producer() run ... " << endl;

        output.addPort<string>("out");

        this->inputFile = readInputFile("input.txt");
        this->inputFilePieces = splitString(this->inputFile, "\n");

        cout << ">> Splitted size is " << this->inputFilePieces.size() << endl;
    }

    virtual raft::kstatus run()
    {
        if (i < this->inputFilePieces.size())
        {
            string outputString = this->inputFilePieces[i];
            cout << ">> Generate " << i << ": " << outputString << endl;
            i++;

            output["out"].push(outputString);

            //cout << ">> producer.run() return raft::proceed ... " << endl;
            return (raft::proceed);
        }
        else
        {
            //cout << ">> producer.run() return raft::stop ... " << endl;
            return (raft::stop);
        }
    };
};

/**
 * Consumer: takes the number from input and dumps it to the socket
 */
class consumer : public raft::kernel
{
public:
    consumer() : raft::kernel()
    {
        cout << ">> Constructer of consumer() run ... " << endl;

        input.addPort<string>("in");
    }

    virtual raft::kstatus run()
    {
        //cout << ">> Consumer receive data from producer and run ... " << endl;

        //Get the data from raft-port input
        string portInput = input["in"].peek<string>();
        input["in"].recycle();

        /////////////////////////////////////////////////////////////////////
        //Deal with the socket

        cout << ">> Write: " << portInput << " ... " << endl;

        //clintConnt = accept(clintListn, (struct sockaddr *)NULL, NULL);

        //Write data to the socket
        portInput = portInput + "\r\n"; // Add "\r\n" for spark know
        write(clintConnt, portInput.c_str(), strlen(portInput.c_str()));
        
        //Wait a bit
        sleep(1);

        //close(clintConnt);
        /////////////////////////////////////////////////////////////////////

        //cout << ">> consumer.run() return raft::proceed ... " << endl;
        return (raft::proceed);
    }
};

int main()
{
    cout << ">> Start initialize Socket ... ";
    /////////////////////////////////////////////////////////////////////
    //Deal with the socket

    clintListn = socket(AF_INET, SOCK_STREAM, 0); // creating socket

    memset(&ipOfServer, '0', sizeof(ipOfServer));

    ipOfServer.sin_family = AF_INET;
    ipOfServer.sin_addr.s_addr = htonl(INADDR_ANY);
    ipOfServer.sin_port = htons(9999); // this is the port number of running server

    bind(clintListn, (struct sockaddr *)&ipOfServer, sizeof(ipOfServer));
    listen(clintListn, 20);

    clintConnt = accept(clintListn, (struct sockaddr *)NULL, NULL);
    /////////////////////////////////////////////////////////////////////
    cout << "Done" << endl;

    cout << ">> Start initialize process ... " << endl;
    producer a;
    consumer b;
    raft::map m;
    m += a >> b;
    cout << "Done" << endl;

    cout << ">> Execute process ... " << endl;
    m.exe();

    cout << ">> Execute done, quit ... ";
    return (EXIT_SUCCESS);
}
