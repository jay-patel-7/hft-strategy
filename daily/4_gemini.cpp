#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <vector>
#include <chrono>
#include <ctime>
#include <cstring>
#include <iomanip>

#define ll long long

using namespace std;

struct Trade {
    double entryVal, exitVal;
    string entryOHLCDataTime, exitOHLCDataTime;
};

struct OHLCData {
    string ticker, OHLCDataTime;
    double open, high, low, close;
    ll weekNo, timestamp; // Use timestamp for accurate sorting
};

// Robust date parsing for Windows/Mac compatibility
time_t dateToTimestamp(string dateStr) {
    struct tm tm = {0};
    int d, m, y;
    // Standard format: DD-MM-YYYY
    if (sscanf(dateStr.c_str(), "%d-%m-%Y", &d, &m, &y) == 3) {
        tm.tm_mday = d;
        tm.tm_mon = m - 1;
        tm.tm_year = y - 1900;
        tm.tm_isdst = -1; 
        return mktime(&tm);
    }
    return -1;
}

bool sorting_function(const OHLCData& lhs, const OHLCData& rhs) {
    return lhs.timestamp < rhs.timestamp;
}

void addWeekno(vector<OHLCData>& dataList) {
    for (auto& i : dataList) {
        // Remove potential whitespace
        i.OHLCDataTime.erase(remove_if(i.OHLCDataTime.begin(), i.OHLCDataTime.end(), ::isspace), i.OHLCDataTime.end());

        time_t rawTime = dateToTimestamp(i.OHLCDataTime);
        if (rawTime == -1) {
            cerr << "Conversion failed for: " << i.OHLCDataTime << endl;
            continue;
        }

        i.timestamp = (ll)rawTime;
        // Linear Week Number: Seconds since 1970 divided by seconds in a week
        // 60s * 60m * 24h * 7d = 604800
        i.weekNo = (ll)rawTime / 604800;
    }
}

int main() {
    string file_path = "nifty_50_formatted.csv";
    ifstream file(file_path);

    if (!file.is_open()) {
        cerr << "Could not open file" << endl;
        return 1;
    }

    string line, word;
    vector<OHLCData> dataList;

    getline(file, line); // Skip header

    while (getline(file, line)) {
        stringstream ss(line);
        OHLCData row;

        getline(ss, row.ticker, ',');
        getline(ss, row.OHLCDataTime, ',');
        getline(ss, word, ','); row.open = stod(word);
        getline(ss, word, ','); row.high = stod(word);
        getline(ss, word, ','); row.low = stod(word);
        getline(ss, word, ','); row.close = stod(word);

        dataList.push_back(row);
    }
    file.close();

    // Process dates and week numbers
    addWeekno(dataList);

    // Sort chronologically
    sort(dataList.begin(), dataList.end(), sorting_function);

    // Trade Logic
    ll count = 0;
    ll consecutiveRed = 3;
    bool hasPosition = false;
    Trade temp;
    vector<Trade> ans;

    for (size_t i = 0; i < dataList.size(); i++) {
        if (hasPosition) {
            // Exit at the first upclose (Close > Open)
            if (dataList[i].close > dataList[i].open) {
                temp.exitVal = dataList[i].close;
                temp.exitOHLCDataTime = dataList[i].OHLCDataTime;
                ans.push_back(temp);
                hasPosition = false;
                count = 0; // Reset count for next entry search
            }
        } else {
            // Entry logic: 3rd consecutive red candle (Close < Open)
            if (dataList[i].close < dataList[i].open) {
                count++;
                if (count == consecutiveRed) {
                    temp.entryVal = dataList[i].close;
                    temp.entryOHLCDataTime = dataList[i].OHLCDataTime;
                    hasPosition = true;
                }
            } else {
                count = 0; // Reset if we see a green candle before reaching 3
            }
        }
    }

    // cout << "Trades Executed: " << ans.size() << endl;
    // cout << "Entry Date | Exit Date | Entry Price | Exit Price" << endl;
    // for (auto& t : ans) {
    //     cout << t.entryOHLCDataTime << " | " << t.exitOHLCDataTime 
    //          << " | " << t.entryVal << " | " << t.exitVal << endl;
    // }

    ofstream outFile("gemini_trades.txt");

    if(!outFile.is_open()){
        cerr << "Error opening trades.txt for writing\n";
    }
    else{
        outFile << "EntryDate,EntryPrice,ExitDate,ExitPrice\n";

        for(auto& i: ans){
            // console print
            cout << i.entryOHLCDataTime
                << " -> "
                << i.exitOHLCDataTime << endl;

            // file write
            outFile << i.entryOHLCDataTime << ","
                    << i.entryVal << ","
                    << i.exitOHLCDataTime << ","
                    << i.exitVal << "\n";
        }

        outFile.close();
        cout << "\nTrades saved to trades.txt\n";
    }

    return 0;
}