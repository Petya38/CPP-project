#include <iostream>
#include <string>
#include <optional>
#include <json.hpp>
#include "portable-file-dialogs.h"
#include "ZIP_file_signing.h"
#include "zipSigner.hpp"

using json = nlohmann::json;

#if _WIN32
#define DEFAULT_PATH "C:\\"
#else
#define DEFAULT_PATH "/tmp"
#endif

namespace message {

json getMessage() {
    size_t length = 0;
    for (size_t i = 0; i < 4; i++) {
        unsigned char read_char = getchar();
        length = length | (read_char << i * 8);
    }

    std::string msg = "";
    for (size_t i = 0; i < length; i++) {
        msg += getchar();
    }
    return json::parse(msg);
}

void sendMessage(const json &json_msg) {
    std::string msg = json_msg.dump();
    size_t len = msg.length();
    for (size_t i = 0; i <= 24; i += 8) {
        std::cout << char(len >> i);
    }
    std::cout << msg << std::flush;
}

} // namespace message

namespace {

std::optional<std::string> openZip() {
    auto f = pfd::open_file("Choose files to read", DEFAULT_PATH,
                        { "Zip Files (.zip)", "*.zip",
                          "All Files", "*" },
                        true);
    if (f.result().empty()) {
        return std::nullopt;
    }
    return f.result()[0];
}

}

int main(){
    
    while (true) {
        json msg = message::getMessage();
        std::cerr << msg.dump() << std::endl;
        if (msg["request"] == "Check certificate in file" ||
                msg["request"] == "Open and check certificate") {

            std::optional<std::string> filepath = std::nullopt;
            if (msg["request"] == "Check certificate in file") {
                if (msg.find("filepath") != msg.end()) {
                    filepath = msg["filepath"];
                }
            }
            else {
                filepath = openZip();
            }
            json j;
            if (!filepath) {
                j["Error"] = "No file found";
            }
            else {
                j["ArchiveName"] = *filepath;
                //j["ArchiveFiles"] = {"1.txt", "2.txt"}; 
                ZipSigner zipSigner;
                ZIP_file_signing::ZIP_file zip_file(filepath->c_str(), zipSigner);
                j["Verified"] = zip_file.check_sign() ? "OK" : "FAILED";
            }
            //string part1 = 
            // auto part2 = (R"({"Verified": "OK",
            //     "ArchiveFiles" : ["1.txt", "2.txt"]})"_json);
            message::sendMessage(j);
        }
        else if (msg["request"] == "Open and sign certificate") {
            std::optional<std::string> filepath = openZip();
            json j;
            if (!filepath) {
                j["Error"] = "No file found";
                
            }
            else if (msg.find("privateKey") == msg.end() || msg["privateKey"].size() == 0) {
                j["Error"] = "No private key found";
            }
            else {
                j["ArchiveName"] = *filepath;
                j["ArchiveFiles"] = {"1.txt", "2.txt"}; 
                ZipSigner zipSigner(msg["privateKey"]);
                ZIP_file_signing::ZIP_file zip_file(filepath->c_str(), zipSigner);
                zip_file.signing();
            }
            message::sendMessage(j);
        }
    }
    return 0;
}