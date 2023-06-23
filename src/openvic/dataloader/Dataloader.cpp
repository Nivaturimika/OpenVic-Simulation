#include "Dataloader.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include "../Types.hpp"

using namespace std;
using namespace std::filesystem;
using namespace OpenVic;


#include <map>
//WARNING!!
//The following functions are temporary and highly fragile
//If you have a weak constitution or have a heart condition, do not look at the following functions
namespace compatibility {
	//Helper function
	void eatWhitespaceComments(wifstream& stream) {
		while (stream && (
			stream.peek() == L' ' ||
			stream.peek() == L'\t' ||
			stream.peek() == L'\r' ||
			stream.peek() == L'\n' ||
			stream.peek() == L'#')) {

			if (stream.peek() == L'#') {
				stream.ignore(numeric_limits<streamsize>::max(), L'\n');
			}
			else {
				stream.ignore(1);
			}
		}
	}

	//Helper function
	wstring getNextWord(wifstream& stream) {
		std::wstringstream ss;
		wchar_t wc;

		eatWhitespaceComments(stream);
		while (stream &&
			stream.peek() != L' ' &&
			stream.peek() != L'\t' &&
			stream.peek() != L'\r' &&
			stream.peek() != L'\n' &&
			stream.peek() != L'#') {
			stream >> wc;
			ss << wc;
		}

		eatWhitespaceComments(stream);
		return ss.str();
	}

	using MapDefinitions = map<int, pair<colour_t, wstring>>;
	MapDefinitions readMapDefinitions(path vic2folder) {
		MapDefinitions mp;
		wifstream ws = wifstream(vic2folder/path("map")/path("definition.csv"), ios::binary);
		
		ws.ignore(numeric_limits<streamsize>::max(), L'\n');
		while (ws) {
			while (ws && ws.peek() == '#' || ws.peek() == ';' || ws.peek() == '\n' || ws.peek() == '\r') {
				ws.ignore(numeric_limits<streamsize>::max(), L'\n');
			}
			if (ws.eof()) break;

			int numericId;
			int r, g, b;
			wstring name;

			ws >> numericId; ws.ignore();
			ws >> r; ws.ignore();
			ws >> g; ws.ignore();
			ws >> b; ws.ignore();
			std::getline(ws, name, L';');
			ws.ignore(numeric_limits<streamsize>::max(), '\n');

			mp[numericId] = pair(rgba_to_colour(r, g, b), name);
		}
		return mp;
	}

	void readProvinceFile(path provinceFile) {
		if (provinceFile.extension() != ".txt") { return; }
		// cout << provinceFile.filename() << endl;
	}

	void readProvinceFiles(path vic2folder) {
		path provinceHistoryPath = vic2folder/path("history")/path("provinces");
		for (directory_entry const& di : recursive_directory_iterator(provinceHistoryPath)) {
			if (di.is_directory()) { continue; }
			// cout << di << endl;
			readProvinceFile(di.path());
		}
	}

	void loadGoodsFile(wifstream& file, OpenVic::Simulation& sim) {
		while (file) {
			wstring category = getNextWord(file); //Load a good category
			if (category == L"military_goods") {
				category = L"military";
			}
			if (category == L"raw_material_goods") {
				category = L"raw";
			}
			if (category == L"industrial_goods") {
				category = L"industrial";
			}
			if (category == L"consumer_goods") {
				category = L"consumer";
			}
			getNextWord(file); //Eat =
			getNextWord(file); //Eat {
			while (file.peek() != L'}') { //Load a good
				wstring goodid = getNextWord(file);
				price_t price = 0.0;
				bool availableAtStart = true;
				bool tradeable = true;
				bool currency = true;
				bool overseasMaintenance = false;
				colour_t colour;
				getNextWord(file); //Eat =
				getNextWord(file); //Eat {
				while (file.peek() != L'}') { //Load good attributes
					wstring attribute = getNextWord(file);
					getNextWord(file); //Eat =
					if (attribute == L"cost") {
						file >> price;
					}
					else if (attribute == L"color") {
						getNextWord(file); //Eat {
						int r, g, b;
						file >> r;
						file >> g;
						file >> b;
						colour = rgba_to_colour(r, g, b);
						
						getNextWord(file); //Eat }
					}
					else if (attribute == L"available_from_start") {
						availableAtStart = (getNextWord(file) == L"yes");
					}
					else if (attribute == L"tradeable") {
						tradeable = (getNextWord(file) == L"yes");
					}
					else if (attribute == L"money") {
						currency = (getNextWord(file) == L"yes");
					}
					else if (attribute == L"overseas_penalty") {
						overseasMaintenance = (getNextWord(file) == L"yes");
					}
				}
				getNextWord(file); //Eat }
				wcout << goodid <<" "<< category <<" 0x"<< HasColour::colour_to_hex_string(colour) <<" "<< price <<" "<< availableAtStart <<" "<< tradeable <<" "<< currency <<" "<< overseasMaintenance << endl;
				sim.goodManager.add_good(goodid, category, colour, price, availableAtStart, tradeable, currency, overseasMaintenance);
			}
			getNextWord(file); //Eat }
		}
	}
}




bool OpenVic::Dataloader::loadDir(std::filesystem::path rootDataFolder, Simulation& sim, LoadingMode loadMode) {
	if (loadMode == LoadingMode::DL_COMPATABILITY) {
		if (!is_directory(rootDataFolder)) {
			return false;
		}

		path goods = rootDataFolder/path("common")/path("goods.txt");
		if (!filesystem::exists(goods)) {
			wcerr << (goods.c_str()) << " does not exist" << endl;
			return false;
		}
		wifstream goodsf(goods);
		cout << "Start loading..." << endl;
		compatibility::loadGoodsFile(goodsf, sim);
		compatibility::readMapDefinitions(rootDataFolder);
		compatibility::readProvinceFiles(rootDataFolder);
		cout << "Done loading" << endl;


		return true;
	}
	return false;
}
