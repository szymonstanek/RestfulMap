#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <csignal>
#include "json.hpp"
#include "server.h"
#include "globals.h"

std::string storagePath;

using json = nlohmann::json;
using namespace std;

//does not work in production
string getExecutablePath() {
    char path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
    std::string pathStr = std::string(path, (count > 0) ? count : 0);
    return pathStr;
}

bool loadConfig() {
    string tempPath;
    tempPath = getExecutablePath();
    string shortenedText = tempPath.substr(0, tempPath.size() - 10);
    shortenedText = shortenedText + "config.json";
    //use manual path in production for testing
    //tempPath = "/home/stanio/CLionProjects/RestfulMap/config.json";
    ifstream configFile(shortenedText);
    if (!configFile.is_open()) {
        cerr << "Failed to open config file." << endl;
        cerr << shortenedText << endl;
        return false;
    }

    json configJson;
    configFile >> configJson;
    logPath = configJson["log_path"];
    storagePath = configJson["storage_path"];
    return true;
}

void saveState() {
    json state;
    //state["data"] = "Current server data";
    ofstream outFile(storagePath);
    outFile << state.dump(4);
}

void signalHandler(int signum) {
    cout << "Interrupt signal (" << signum << ") received.\n";
    saveState();
    // Cleanup and close up stuff here
    exit(signum);
}

void displayAsciiArt() {
    cout << R"(


  _____           _    __       _ __  __
 |  __ \         | |  / _|     | |  \/  |
 | |__) |___  ___| |_| |_ _   _| | \  / | __ _ _ __
 |  _  // _ \/ __| __|  _| | | | | |\/| |/ _` | '_ \
 | | \ \  __/\__ \ |_| | | |_| | | |  | | (_| | |_) |
 |_|  \_\___||___/\__|_|  \__,_|_|_|  |_|\__,_| .__/
                                              | |
                                              |_|


Press "x" to exit, this ensures saving to storage.json
    )" << endl;
}

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void displayLastThreeLogs() {
    ifstream logFile(logPath);
    if (!logFile.is_open()) {
        cerr << "Failed to open log file." << endl;
        return;
    }

    vector<string> logLines;
    string line;
    while (getline(logFile, line)) {
        logLines.push_back(line);
    }
    logFile.close();

    int start_index = max(0, static_cast<int>(logLines.size()) - 3);
    for (int i = start_index; i < logLines.size(); ++i) {
        cout << logLines[i] << endl;
    }
}

void displayLogsContinuously() {
    while (true) {
        //clearScreen() does not work in production
        clearScreen();
        //cout << "\033[2J\033[1;1H";
        displayAsciiArt();
        displayLastThreeLogs();
        this_thread::sleep_for(chrono::milliseconds(1000));
    }
}

void listenForShutdown(KeyValueApi& server) {
    char command;
    while (cin >> command) {
        if (command == 'X' || command == 'x') {
            cout << "Shutdown command received." << endl;
            server.stop();
            server.logEvent("INFO","Server shutdown by user command.");
            exit(0);
        }
    }
}



int main() {
    if (!loadConfig()) {
        return 1;
    }
    KeyValueApi server(Pistache::Address("*:9080"));
    server.init();
    server.logEvent("INFO", "Server initialized and started");

    // Start the server in a separate thread to keep the main thread free for listening to the shutdown command
    thread serverThread([&server](){
        server.start();
    });

    thread shutdownThread(listenForShutdown, ref(server));

    // Create a thread for displaying logs continuously
    thread logThread(displayLogsContinuously);

    // Wait for all threads to complete
    serverThread.join();
    shutdownThread.join();
    logThread.join();  // Ensure the log thread is also joined

    return 0;
}

