#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <vector>
#include <ctime>

#define ll long long

using namespace std;

struct Trade{
    double entryVal, exitVal;
    string entryOHLCDataTime, exitOHLCDataTime;
};

struct OHLCData{
    string ticker, OHLCDataTime;
    double open, high, low, close;
    ll weekNo, dayNo;
};

bool operator<(OHLCData const& lhs, OHLCData const& rhs) noexcept{
    if(lhs.weekNo < rhs.weekNo) return true;
    if(lhs.weekNo == rhs.weekNo) return lhs.dayNo < rhs.dayNo;
    return false;
}

bool sorting_function(OHLCData& lhs, OHLCData& rhs){
    return lhs < rhs;
}

void print_OHLCData(OHLCData& t){
    cout << t.ticker << " " << t.OHLCDataTime
         << " Week:" << t.weekNo
         << " Day:" << t.dayNo << endl;
}

// ✅ FIXED FUNCTION
void addWeeknoDayno(vector<OHLCData>& dataList){
    time_t base_time = 0;
    bool first = true;
    int offset = 0;

    for(auto& i : dataList){
        string date = i.OHLCDataTime;

        // trim leading spaces
        date.erase(date.begin(), std::find_if(date.begin(), date.end(),
            [](unsigned char ch) { return !std::isspace(ch); }));

        i.OHLCDataTime = date;

        // ✅ manual parsing (cross-platform safe)
        int day = stoi(date.substr(0, 2));
        int month = stoi(date.substr(3, 2));
        int year = stoi(date.substr(6, 4));

        std::tm then{};
        then.tm_mday = day;
        then.tm_mon = month - 1;
        then.tm_year = year - 1900;

        time_t current = mktime(&then);

        if(first){
            base_time = current;
            first = false;
            offset = (then.tm_wday == 0 ? 7 : then.tm_wday);
        }

        int days_diff = (current - base_time) / (60 * 60 * 24);
        int week = (days_diff + offset) / 7 + 1;

        // ✅ correct day of week (Mon=1 ... Sun=7)
        int dayNo = (then.tm_wday == 0 ? 7 : then.tm_wday);

        i.weekNo = week;
        i.dayNo = dayNo;
    }
}

int main(){

    string file_path = "nifty_50_formatted.csv";

    ifstream file(file_path);

    if(!file.is_open()){
        cerr << "Could not open file" << endl;
        return 1;
    } else {
        cout << "able to access the file" << endl;
    }

    string line, word;
    vector<OHLCData> dataList;

    // skip header
    getline(file, line);

    while(getline(file, line)){
        stringstream ss(line);
        OHLCData row;

        getline(ss, row.ticker, ',');
        getline(ss, row.OHLCDataTime, ',');

        try {
            getline(ss, word, ','); row.open = stod(word);
            getline(ss, word, ','); row.high = stod(word);
            getline(ss, word, ','); row.low = stod(word);
            getline(ss, word, ','); row.close = stod(word);
        } catch(...) {
            cout << "Skipping bad row: " << line << endl;
            continue;
        }

        dataList.push_back(row);
    }

    file.close();

    // ✅ add week + day info
    addWeeknoDayno(dataList);

    // ✅ sort
    sort(dataList.begin(), dataList.end(), sorting_function);

    // debug print
    // cout << "\n--- First 100 rows after sorting ---\n";
    // ll mn = min((ll)dataList.size(), 100LL);

    // for(ll i = 0; i < mn; i++){
    //     print_OHLCData(dataList[i]);
    // }

    // =========================
    // ✅ TRADING LOGIC (unchanged)
    // =========================

    ll count = 0, consecutiveRed = 3;
    ll dls = dataList.size();
    bool inTrade = false;

    Trade temp;
    vector<Trade> ans;

    for(ll i = 1; i < dls; i++){

        if(inTrade){
            // exit on next upclose
            if(dataList[i - 1].close < dataList[i].close){
                temp.exitVal = dataList[i].close;
                temp.exitOHLCDataTime = dataList[i].OHLCDataTime;
                inTrade = false;
                ans.push_back(temp);
            }
        }
        else{
            // check red candle
            if(dataList[i].open > dataList[i].close){
                count++;
                if(count == consecutiveRed){
                    inTrade = true;
                    temp.entryVal = dataList[i].close;
                    temp.entryOHLCDataTime = dataList[i].OHLCDataTime;
                    count = 0;
                }
            }
            else{
                count = 0;
            }
        }
    }

    // print trades
    // cout << "\n--- Trades ---\n";
    // for(auto& i: ans){
    //     cout << i.entryOHLCDataTime
    //          << " -> "
    //          << i.exitOHLCDataTime << endl;
    // }

    ofstream outFile("chatgpt_trades.txt");

    if(!outFile.is_open()){
        cerr << "Error opening trades.txt for writing\n";
    }
    else{
        outFile << "EntryDate,EntryPrice,ExitDate,ExitPrice\n";

        for(auto& i: ans){
            // console print
            // cout << i.entryOHLCDataTime
            //     << " -> "
            //     << i.exitOHLCDataTime << endl;

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