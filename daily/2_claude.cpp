#include<stdio.h>
#include<iostream>
#include<fstream>
#include<sstream>
#include<string>
#include<algorithm>
#include<vector>
#include<chrono>
#include<ctime>
#include<cstring>
#include<iomanip>

#define ll long long

using namespace std;

void flag(string s){
    cout << s << endl;
}

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
    if(lhs.weekNo < rhs.weekNo){
        return true;
    }
    else if(lhs.weekNo == rhs.weekNo){
        return lhs.dayNo < rhs.dayNo;
    }
    else{
        return false;
    }
}

bool sorting_function(OHLCData& lhs, OHLCData& rhs){
    return lhs < rhs;
}

void print_OHLCData(OHLCData& t){
    cout << t.ticker << " " << t.OHLCDataTime << " " << t.weekNo << " " << t.dayNo << endl;
}

// Parses "DD-MM-YYYY" and returns a time_t.
// Works cross-platform (no %V, no strftime quirks).
time_t parseDate(const string& date){
    std::tm then{};
    std::istringstream in(date);
    if(!(in >> std::get_time(&then, "%d-%m-%Y"))){
        std::cerr << "Conversion failed for date: " << date << "\n"; // FIX: was "std:cerr" (missing colon)
        return -1;
    }
    then.tm_isdst = -1; // let mktime figure out DST
    return mktime(&then);
}

void addWeeknoDayno(vector<OHLCData>& dataList){

    // First pass: parse all dates, find the earliest date's time_t
    // We use that as the epoch anchor so week numbers are GLOBAL (not per-year).
    // This avoids the %V ISO-week problem where Jan 1 2020 and Jan 1 2025 both == week 1.

    vector<time_t> times(dataList.size());
    time_t earliest = std::numeric_limits<time_t>::max();

    for(size_t idx = 0; idx < dataList.size(); idx++){
        string& date = dataList[idx].OHLCDataTime;

        // Trim leading whitespace (cross-platform safe)
        date.erase(date.begin(), std::find_if(date.begin(), date.end(), [](unsigned char ch){
            return !std::isspace(ch);
        }));

        time_t t = parseDate(date);
        times[idx] = t;

        if(t != -1 && t < earliest){
            earliest = t;
        }
    }

    // Seconds in one week
    const time_t SECS_PER_WEEK = 7 * 24 * 3600;

    // Second pass: assign global week index and day-of-month
    for(size_t idx = 0; idx < dataList.size(); idx++){
        time_t t = times[idx];

        if(t == -1){
            dataList[idx].weekNo = -1;
            dataList[idx].dayNo  = -1;
            continue;
        }

        // Global week number: 0-based from first date in dataset.
        // Both 01-01-2020 and 01-01-2025 will now get different week numbers
        // because we measure elapsed weeks from the dataset's own start date.
        dataList[idx].weekNo = (ll)((t - earliest) / SECS_PER_WEEK);

        // Day-of-month as the secondary sort key within a week
        dataList[idx].dayNo = stoi(dataList[idx].OHLCDataTime.substr(0, 2));
    }
}

int main(){

    string file_path = "nifty_50_formatted.csv";

    ifstream file(file_path);

    if(!file.is_open()){
        cerr << "Could not open file" << endl;
        return 1;
    }
    else{
        cout << "Able to access the file" << endl;
    }

    string line, word;
    vector<OHLCData> dataList;

    getline(file, line); // skip header

    while(getline(file, line)){
        stringstream ss(line);
        OHLCData row;

        getline(ss, row.ticker, ',');
        getline(ss, row.OHLCDataTime, ',');

        getline(ss, word, ','); row.open  = stod(word);
        getline(ss, word, ','); row.high  = stod(word);
        getline(ss, word, ','); row.low   = stod(word);
        getline(ss, word, ','); row.close = stod(word);

        dataList.push_back(row);
    }

    file.close();

    addWeeknoDayno(dataList);

    sort(dataList.begin(), dataList.end(), sorting_function);

    // ll mn = (ll)std::min(dataList.size(), (size_t)100); // FIX: cast to avoid signed/unsigned mismatch & conflict with custom min()

    // cout << "print_OHLCData (first " << mn << " rows)" << endl;
    // for(ll i = 0; i < mn; i++){
    //     print_OHLCData(dataList[i]);
    // }
    // cout << endl << endl;

    // --- Trading logic (daily data, upclose count = 1 for now) ---
    ll count = 0, consecutiveRed = 3;
    ll dls = dataList.size();
    bool B = 0;
    Trade temp;
    vector<Trade> ans;

    ll upcloseCount = 0;
    ll upcloseTarget = 1; // dynamic upclose: currently fixed at 1, will change later

    for(ll i = 0; i < dls; i++){
        if(B){
            if(dataList[i].close > dataList[i - 1].close){ // upclose candle
                upcloseCount++;
                if(upcloseCount == upcloseTarget){
                    temp.exitVal = dataList[i].close;
                    temp.exitOHLCDataTime = dataList[i].OHLCDataTime;
                    B = 0;
                    upcloseCount = 0;
                    ans.push_back(temp);
                }
            }
        }
        else{
            if(dataList[i].open > dataList[i].close){ // red candle
                count++;
                if(count == consecutiveRed){
                    B = 1;
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

    // cout << "Trades: " << endl;
    // for(auto& i: ans){
    //     cout << i.entryOHLCDataTime << " " << i.exitOHLCDataTime << endl;
    // }

    cout << "\n--- Trades ---\n";

    ofstream outFile("claude_trades.txt");

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