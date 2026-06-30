#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cstdlib>
#include <fstream>
#include <regex>
#include "json.hpp"

std::string GAME_PATH = "";

namespace fs = std::filesystem;

std::string GetSteamPathFromRegistry() {
	return "C:\\Program Files (x86)\\Steam";
}

std::vector<std::string> GetSteamLibraries(const std::string& steamPath) {
	std::vector<std::string> libraries;
	std::string vdfPath = steamPath + "\\steamapps\\libraryfolders.vdf";
	
	if (!fs::exists(vdfPath)) {
		return libraries;
	}

	std::ifstream file(vdfPath);
	std::string line;
	std::regex pathRegex(R"(\"path\"\s+\"([^\"]+)\")");
	std::smatch match;

	while (std::getline(file, line)) {
		if (std::regex_search(line, match, pathRegex)) {
			std::string path = match[1].str();
			std::replace(path.begin(), path.end(), '/', '\\');
			libraries.push_back(path);
		}
	}

	if (libraries.empty()) {
		libraries.push_back(steamPath);
	}
	return libraries;
}

void saveConfig(const std::string& path) {
	nlohmann::json configData;
	configData["game_path"] = path;

	std::ofstream file("config.json");
	if (file.is_open()) {
		file << configData.dump(4);
		file.close();
	}
}

std::string loadConfig() {
	if (fs::exists("config.json")) {
		std::ifstream file("config.json");
		nlohmann::json configData;
		try {
			file >> configData;
			file.close();

			if (configData.contains("game_path")) {
				std::string path = configData["game_path"].get<std::string>();
				if (fs::exists(path)) {
					return path;
				}
			}
		} catch (...) {
			file.close();
		}
	}
	return "";
}


std::vector<std::string> getIpFromAWS(std::string targetRegion) {
	std::vector<std::string> filteredIps;
	std::ifstream file("ip-ranges.json");

	std::vector<std::string> whitelist = {
		"104.18.124.108", // Cloudflare/Store
		"3.222.111.104",
		"98.86.51.169", // Master Server
		"44.219.16.220",
		"44.195.230.128",
		"44.221.180.77" // Login Server
	};

	if (!file.is_open()) {
		std:: cout << "ip-ranges.json not found." << std::endl;
		return filteredIps;
	}
	nlohmann::json awsData;
	file >> awsData;

	for (const auto& item : awsData["prefixes"]) {
		if (item["region"] == targetRegion) {
			std::string currentIp = item["ip_prefix"].get<std::string>();

			bool isWhitelisted = false;
			for (const std::string& whiteIp : whitelist) {
				if (currentIp.find(whiteIp) != std::string::npos || whiteIp.find(currentIp) != std::string::npos) {
					isWhitelisted = true;
					break;
				}
			}
			if (!isWhitelisted) {
				filteredIps.push_back(currentIp);
			}
		}
	}
	file.close();
	return filteredIps;
}

void blockServer(const std::vector<std::string>& Ips) {
	if (Ips.empty()) {
		std::cout << "No IPs found for this region." << std::endl;
		return;
	}

	const size_t chunkSize = 80;
	for (size_t i = 0; i < Ips.size(); i += chunkSize) {

		std::stringstream ss;
		ss << Ips[i];
		for (size_t j = i + 1; j < Ips.size() && j < i + chunkSize; j++) {
			ss << "," << Ips[j];
		}
		std::string allIp = ss.str();
		std::string command = "netsh advfirewall firewall add rule name=\"BlockServer\" dir=out action=block program=\"" + GAME_PATH +"\" remoteip=" + allIp;
		int sReturn = system(command.c_str());
		if (sReturn == 0) {
			std::cout << "Block Server Complete!";
		}
	}
}
static std::string chooseServerToBlock(const int& numberToSelect) {
	std::string regionName;
	std::string clearCommand = "netsh advfirewall firewall delete rule name=\"BlockServer\" program=\"" + GAME_PATH + "\"";
	switch (numberToSelect) {
	case 1: regionName = "ap-southeast-1"; break;
	case 2: regionName = "ap-northeast-1"; break;
	case 3: regionName = "ap-northeast-2"; break;
	case 4: regionName = "ap-east-1"; break;
	case 5: regionName = "ap-southeast-2"; break;
	case 6: regionName = "ap-south-1"; break;
	//case 7: regionName = "us-east-1"; break;
	case 7: system(clearCommand.c_str());
		std::cout << "Firewall rules cleared." << std::endl;
		break;
	}
	return regionName;
}

int main() {
	std::string rel_game_path = "\\steamapps\\common\\Dead by Daylight\\DeadByDaylight\\Binaries\\Win64\\DeadByDaylight-Win64-Shipping.exe";
	std::string final_path = "";

	std::cout << "[System] Scanning for Dead by Daylight..." << std::endl;

	final_path = loadConfig();

	if (!final_path.empty()) {
		std::cout << "[Success] Loaded game path from config.json" << std::endl;
	}
	else {
		std::cout << "[System] Config not found. Scanning for Dead by Daylight..." << std::endl;

		std::string steamPath = GetSteamPathFromRegistry();
		std::vector<std::string> libraries = GetSteamLibraries(steamPath);

		for (const auto& lib : libraries) {
			std::string check_path = lib + rel_game_path;
			if (fs::exists(check_path)) {
				final_path = check_path;
				break;
			}
		}
		if (!final_path.empty()) {
			std::cout << "[Success] Found game automatically!" << std::endl;
			saveConfig(final_path);
		}
	}

	if (!final_path.empty()) {
		GAME_PATH = final_path;
		std::cout << "Path: " << GAME_PATH << "\n\nPress Enter to continue to menu...";
		std::cin.get();
	}
	else {
		std::cout << "[Warning] Could not detect the game automatically." << std::endl;
		std::cout << "Please drag and drop your game folder (or path to EXE) here:\n-> ";

		std::getline(std::cin, GAME_PATH);

		if (!GAME_PATH.empty() && GAME_PATH.front() == '"' && GAME_PATH.back() == '"') {
			GAME_PATH = GAME_PATH.substr(1, GAME_PATH.length() - 2);
		}

		if (fs::exists(GAME_PATH) && !GAME_PATH.ends_with(".exe")) {
			GAME_PATH += rel_game_path;
		}

		if (fs::exists(GAME_PATH)) {
			saveConfig(GAME_PATH);
			std::cout << "[Success] Path saved!" << std::endl;
		}
		else {
			std::cout << "[Error] Invalid path. Program might not work properly." << std::endl;
			std::cin.get();
		}
	}

	int input;
	while (true) {
		system("cls");
		std::cout << "--- DBD Server Selector ---" << std::endl;
		std::cout << "1. Singapore (ap-southeast-1)" << std::endl;
		std::cout << "2. Tokyo (ap-northeast-1)" << std::endl;
		std::cout << "3. Seoul [South Korea] (ap-northeast-2)" << std::endl;
		std::cout << "4. Hong Kong (ap-east-1)" << std::endl;
		std::cout << "5. Sydney (ap-southeast-2)" << std::endl;
		std::cout << "6. Mumbai [india] (ap-south-1)" << std::endl;
		//std::cout << "7. US East (us-east-1)" << std::endl;
		std::cout << "7. Unblock All (Delete Rule)" << std::endl;
		std::cout << "---------------------------" << std::endl;
		std::cout << "Select option: ";

		if (!(std::cin >> input)) {
			std::cin.clear();
			std::cin.ignore(1000, '\n');
			continue;
		}
		if (input == 9) { 
			break;
		}
		std::string serverToblock = chooseServerToBlock(input);
		if (!serverToblock.empty()) {
			std::vector <std::string> Ips = getIpFromAWS(serverToblock);
			blockServer(Ips);
			std::cout << "\nProcess finished. Press Enter to return to menu...";
			std::cin.ignore();
			std::cin.get();
		}
	}
	return 0;
}