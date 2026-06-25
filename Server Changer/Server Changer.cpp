#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cstdlib>
#include <fstream>
#include "json.hpp"

const std::string GAME_PATH = "G:\\Program Files (x86)\\Steam\\steamapps\\common\\Dead by Daylight\\DeadByDaylight\\Binaries\\Win64\\DeadByDaylight-Win64-Shipping.exe";

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
	case 7: regionName = "us-east-1"; break;
	case 8: system(clearCommand.c_str());
		std::cout << "Firewall rules cleared." << std::endl;
		break;
	}
	return regionName;
}

int main() {
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
		std::cout << "7. US East (us-east-1)" << std::endl;
		std::cout << "8. Unblock All (Delete Rule)" << std::endl;
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